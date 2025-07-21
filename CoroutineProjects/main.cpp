/******************************************************************************

Welcome to GDB Online.
  GDB online is an online compiler and debugger tool for C, C++, Python, PHP, Ruby, 
  C#, OCaml, VB, Perl, Swift, Prolog, Javascript, Pascal, COBOL, HTML, CSS, JS
  Code, Compile, Run and Debug online from anywhere in world.

*******************************************************************************/
#include <iostream>
#include <utility> // Required for std::exchange
#include <coroutine>
#include <optional>
#include <chrono>
#include <queue>
#include <vector>
#include <atomic>
#include <future>
#include <memory>
#include <unordered_map>
#include <random>
#include <format>
#include <thread>
#include <condition_variable>
#include <iomanip>
#include <stop_token>
#include <sstream>
#include <syncstream>
#include <map>
#include <cassert> // assert macro
#include <atomic>
#include <condition_variable>
#include <queue>
#include <list>

using namespace std::chrono;
using namespace std::chrono_literals;
using TimePoint = steady_clock::time_point;

using namespace std;

#define TRACE_WAS(msg) std::cout << "[CoroutineQueue] " << msg << "\n";
//#define TRACE(msg) do { std::osyncstream(std::cout) << msg << "\n"; } while (false)

#define ENABLE_TRACE_LOGS 1
#if ENABLE_TRACE_LOGS
    #include <syncstream>
    #define TRACE(msg) do { std::osyncstream(std::cout) << msg << "\n"; } while (false)
#else
    #define TRACE(msg) do {} while (false)
#endif


/*
The CoroutineQueue<T> manages async delivery and backpressure.
The Task class owns/resumes coroutine handles.
CoroutineManager orchestrates multiple producers/consumers and lifecycle.

CoroutineQueue<T> Logic
send(value) (producer side)
When a producer co_await queue.send(val):

If the queue has space:
await_ready() returns true, or await_suspend() returns false.
deliver(val) sends value to waiting consumer or buffers it.
No suspension.

If queue is full:
await_suspend() pushes producer + value to prodWaiters_.
Coroutine suspends.
Later, a consumer's next() resumes it and deliver() pushes value.

next() (consumer side)
When a consumer co_await queue.next():
If buffer has data:
await_ready() returns true, value is extracted.
If queue is closed:
Returns std::nullopt to signal termination.
Else:
Suspends consumer handle in consWaiters_.
On resume:
Returns value or nothing.
If producers are waiting, wakes one producer via prod.h.resume().
=====================================
struct SendAwaitable Producer side
(Used in co_await queue.send(value))
Member variables
CoroutineQueue& q_;
T value_;
bool needDeliver_ = true;
q_: reference to the queue
value_: value the producer wants to send
needDeliver_: controls whether await_resume() should actually deliver the value or skip it (because it was already delivered earlier)

bool await_ready()
Checks if the queue has space.
If true, then the coroutine does not suspend (co_await completes immediately).
If false, coroutine goes to await_suspend()

bool await_suspend(std::coroutine_handle<> h)
Two cases:

Case 1: Queue has space
Call deliver(value) immediately.
Skip suspension (return false).

Case 2: Queue is full
Save the coroutine handle + value to prodWaiters_.
Return true to suspend the coroutine.
It will be resumed later by a consumer pulling a value.

void await_resume()
if (needDeliver_) {
    q_.deliver(std::move(value_));
}
If we didn’t deliver earlier (e.g., coroutine resumed later from prodWaiters_), 
then we now deliver the value to queue.

struct NextAwaitable Consumer side
(Used in auto val = co_await queue.next())
Member variables
CoroutineQueue& q_;
std::optional<T> slot_;
q_: queue reference

slot_: holds the value to be returned to the coroutine (optional)
Three cases:
Data is in buffer: Pull now → no suspend
Queue is closed: Nothing more will come → no suspend
Empty & open: Suspend the coroutine → go to await_suspend

void await_suspend(std::coroutine_handle<> h)
q_.consWaiters_.push_back(h);
Queue is empty → save the coroutine handle for future wake-up

std::optional<T> await_resume()
If slot_ was filled in await_ready(), return it.
If queue is closed, return std::nullopt (consumer should exit).
Else:
Pull from buffer.
Wake one producer (if waiting in prodWaiters_).
Return the value.


TimerAwaitable
Non-blocking async timer:
co_await TimerAwaitable(20ms, stopToken):
If deadline not reached, spins a detached thread that wakes coroutine after 5ms slices.
Uses stop_token for cancellation.

CoroutineManager<T>
CoroutineManager<int> mgr(8);
Creates a CoroutineQueue<int> of size 8.
Uses std::stop_source to signal stop.

Producers
co_await queue_.send(n);
co_await TimerAwaitable(20ms, stopToken_);
Loops while stop_requested() is false.
Sends integer values to queue.
Delays 20ms using timer.

Consumers
auto opt = co_await queue_.next();
if (!opt) break;
std::cout << tag << " got " << *opt << '\n';
Waits for next value or queue closure.
Exits cleanly if closed.

Shutdown Logic
requestStop()
Cancels producer timers (via stop_token).
Closes the queue (wakes all suspended consumers with .resume()).
Marks stopped_ = true.
wait()
Repeatedly calls tickOnce():
Resumes all still-alive coroutines.
Erases completed tasks.
Waits until all tasks are done.

Producer (send)
   ├─ If queue has room → deliver → skip suspend
   ├─ Else → save (value+handle) → suspend
   └─ await_resume → deliver if needed

Consumer (next)
   ├─ If queue has data → pop → return optional<T>
   ├─ If queue is closed → return nullopt
   ├─ Else → save handle → suspend
   └─ await_resume → pop value → maybe wake producer



*/

template <typename T>
class CoroutineQueue {
public:
    explicit CoroutineQueue(std::size_t max = 64) : maxSize_(max) {}
    /*───────────────────────────────────────────────────────────────
     *  Producer side  –  co_await queue.send(value);
     *───────────────────────────────────────────────────────────────*/
    struct SendAwaitable {
        CoroutineQueue& q_;
        T value_;
        bool needDeliver_ = true;

        template <typename U>
        SendAwaitable(CoroutineQueue& q, U&& v)
            : q_(q), value_(std::forward<U>(v)) {
                TRACE("SendAwaitable created with value: " << value_);
            }

        bool await_ready() const noexcept {
            bool ready = q_.buffer_.size() < q_.maxSize_;
            TRACE("SendAwaitable await_ready: " << std::boolalpha << ready
                << " (buffer size: " << q_.buffer_.size()
                << "/" << q_.maxSize_ << ")");
            return ready;
        }

        // returns true  → suspend
        // returns false → continue, but await_resume() will still run
        template<typename Promise>
        bool await_suspend(std::coroutine_handle<Promise> h) {
            if (q_.buffer_.size() < q_.maxSize_) {
                TRACE("SendAwaitable await_suspend: No suspend. Delivering value " 
                << value_ << " directly.");
                q_.deliver(std::move(value_));
                needDeliver_ = false;
                return false; // skip suspension
            }
            TRACE("SendAwaitable await_suspend: Queue full. Suspending producer. value " 
                << value_
                << " Prod Waiters: " << q_.prodWaiters_.size());
            q_.prodWaiters_.push_back({std::move(value_), h});
            needDeliver_ = false;                           // consumer will push    
            return true; // suspend producer
        }

        void await_resume() noexcept {
            TRACE("SendAwaitable await_resume: Producer resumed.");
            if (needDeliver_) {                             // fast‑path case
                q_.deliver(std::move(value_));
            }
        }
    };
    //Producer call co_await queue.send(42);
    //push
    template <typename U>
    [[nodiscard]] SendAwaitable send(U&& v) {
        return SendAwaitable{*this, std::forward<U>(v)};
    }
    /*───────────────────────────────────────────────────────────────
     *  Consumer side  –  T val = co_await queue.next();
     *───────────────────────────────────────────────────────────────*/
        struct NextAwaitable {
        CoroutineQueue& q_;
        std::optional<T> slot_;      // holds value if any

        //bool await_ready() const noexcept { return false; } // always suspend
       bool await_ready() noexcept {
           std::lock_guard lock(q_.mtx_);
            if (!q_.buffer_.empty()) {                 // data ready
                slot_.emplace(std::move(q_.buffer_.front()));
                q_.buffer_.pop_front();
                return true;
            }
            if (q_.closed_)            // nothing more will ever come No suspend
                return true;           // ready with empty optional
            return false;              // need to suspend
        }
        template<typename Promise>
        void await_suspend(std::coroutine_handle<Promise> h) {
            q_.consWaiters_.push_back(h);
            TRACE("NextAwaitable await_suspend: Consumer suspended. "
            << "Buffer size: " << q_.buffer_.size());
        }

        std::optional<T> await_resume() {
            std::lock_guard lock(q_.mtx_);
            if (slot_) return std::move(slot_);   // had data
            if (q_.closed_) return std::nullopt;  // closed 
            assert(!q_.buffer_.empty() && "Consumer CoroutineQueue::await_resume: buffer is empty unexpectedly!");

            slot_.emplace(std::move(q_.buffer_.front()));
            q_.buffer_.pop_front();

            if (!q_.prodWaiters_.empty()) {       // free one producer
                TRACE("NextAwaitable await_resume: Waking(resume) one Producer. "
                << "Remaining producers: " << q_.prodWaiters_.size());
                auto prod = std::move(q_.prodWaiters_.front());
                q_.prodWaiters_.pop_front();
                q_.deliver(std::move(prod.value));
                prod.h.resume();
            }
            return std::move(slot_);

        }
    };
    [[nodiscard]] NextAwaitable next() { return NextAwaitable{*this}; }

    // Status queries for inspection/logging
    std::size_t consWaitersSize() const { return consWaiters_.size(); }
    std::size_t prodWaitersSize() const { return prodWaiters_.size(); }
    std::size_t bufferSize() const { return buffer_.size(); }
        void close() {
            closed_ = true;
            TRACE("Closing queue " << "Remaining consumers: " << consWaiters_.size());
            while (!consWaiters_.empty()) {
                auto h = consWaiters_.front();
                consWaiters_.pop_front();
                h.resume();                     // wake every consumer
            }
            
        }
        bool closed_{false};    
private:
    struct PendingProd {
        T value;
        std::coroutine_handle<> h;
    };

    std::deque<T> buffer_;
    std::deque<std::coroutine_handle<>> consWaiters_;
    std::deque<PendingProd> prodWaiters_;
    const std::size_t maxSize_;
    mutable std::mutex mtx_;
    void deliver(T&& v) {
        std::unique_lock lock(mtx_);
        buffer_.push_back(std::move(v));
        if (!consWaiters_.empty()) {
            auto c = consWaiters_.front();
            consWaiters_.pop_front();
            cout << "[CoroutineQueue] deliver(): value pushed, resuming Consumer thread ID" 
            << std::this_thread::get_id() << "\n";
            lock.unlock();
            c.resume();
        } else {
            cout << "[CoroutineQueue] deliver(): value queued. No waiting consumers.\n";
        }
    }
};

//to trace status
enum class TaskState {
    Created,
    Running,
    Suspended,
    Done
};

inline const char* to_string(TaskState state) {
    using enum TaskState;
    switch (state) {
        case Created: return "Created";
        case Running: return "Running";
        case Suspended: return "Suspended";
        case Done:     return "Done";
    }
    return "Unknown";
}

struct TaskStatus {
    std::string tag;
    TaskState state;
    int resumes = 0;
    int suspends = 0;
    friend std::ostream& operator << (std::ostream& oss, const TaskStatus& s) {
        oss << "[" << s.tag << "] "
          << "State: " << to_string(s.state)
          << " Resumes: " << s.resumes
          << " Suspends: " << s.suspends << "\n";
        return oss;
    }
};

//Task is the handle-owner.
//handle that points to the coroutine frame
/* promise with using
    using MyPromise = Task::promise_type;
    std::coroutine_handle<MyPromise> h;
*/
struct Task {

    struct promise_type {
        std::string tag_;
        TaskState state_ = TaskState::Created;
        int resumeCount_ = 0;
        int suspendCount_ = 0;
        Task get_return_object() {
            return Task{
                std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        std::suspend_always initial_suspend() noexcept { 
            state_ = TaskState::Suspended;
            ++suspendCount_;            
            return {}; 
        }
        std::suspend_always final_suspend()   noexcept {
            state_ = TaskState::Done;
            ++suspendCount_;   
            return {}; 
        }
        void return_void()                    noexcept {}
        void unhandled_exception()            { std::terminate(); }
        void setTag(std::string tag) { tag_=tag; }
        const std::string& tag() const { return tag_; }
        TaskStatus getStatus() const {
            return TaskStatus{
                .tag = tag_,
                .state = state_,
                .resumes = resumeCount_,
                .suspends = suspendCount_
            };
        }
    };
    //using TaskPromise=promise_type;
    explicit Task(std::coroutine_handle<promise_type> h) : h_(h) {}
    Task(const Task&)            = delete;
    Task& operator=(const Task&) = delete;
    Task(Task&& o) noexcept : h_(o.h_) { o.h_ = {}; }

    Task& operator=(Task&& other) noexcept {
        if (this != &other) {
            if (h_) h_.destroy();      // ALWAYS destroy when we own a handle
            h_ = other.h_;
            other.h_ = nullptr;
        }
        return *this;
    }
    ~Task() { 
        if (h_) {
            std::cout << h_.promise().getStatus();
            std::cout <<h_.promise().tag_ << " [Task] Destroying coroutine\n";
            h_.destroy();
            h_ = {};
        } 
    }
    
    void start() {
        if (done()) {
            std::cerr << "[Task] Tried to start a completed coroutine\n";
            std::terminate();
        }
        resume(); 
    }  
    
    bool done() const { return !h_ || h_.done(); }
    void resume() { 
        if (!h_) return;         // Handle is null, do nothing
        if (h_.done()) {
            h_.destroy();
            h_ = {};             // Clean up if somehow missed
            return;
        }
        std::cout <<((h_.promise().tag_.empty())?"Initial suspend":h_.promise().tag_)
        << " [Task] Resuming coroutine\n";
        ++h_.promise().resumeCount_;
        h_.promise().state_ = TaskState::Running;
        h_.resume();
    }
    
    explicit operator bool() const noexcept { return h_ != nullptr; }
    TaskStatus status() const {
        return h_ ? h_.promise().getStatus() : TaskStatus{};
    }
    
private:
    std::coroutine_handle<promise_type> h_;
};

//available to pass tag to promise
template<typename Promise>
struct TagSetter {
    std::string tag;

    bool await_ready() const noexcept { return false; }
    
    //await_suspend(std::coroutine_handle<> h not work
    //need template TagSetter with typename Promise and pass Task on constructor 

    void await_suspend(std::coroutine_handle<Promise> h) const {
        auto& promise = h.promise();
        promise.setTag(tag);
    }

    void await_resume() const noexcept {}
};

struct TimerAwaitable {
    using clock = std::chrono::steady_clock;

    clock::time_point target_;
    std::stop_token   st_;                  // cancellation source
    std::string       tag_; 
    static constexpr auto kSlice = std::chrono::milliseconds{5};

    TimerAwaitable(std::chrono::milliseconds d, std::stop_token st, std::string tag = "" )
        : target_{clock::now() + d}, st_{st}, tag_(tag) {}

    bool await_ready() const noexcept {
        return clock::now() >= target_;
    }
    template<typename Promise>
    void await_suspend(std::coroutine_handle<Promise> h) const {
        /* The helper thread lives at most (delay + one slice) and
           exits early if stop is requested. */
        std::thread([h, tgt = target_, st = st_, tag = tag_] {
            const auto tid = std::this_thread::get_id();
            std::cout << "[Timer] Waiting on thread " << tid << " for tag: " << tag << "\n";
            for (;;) {
                if (st.stop_requested()) {          //   ancelled?
                    std::cout << "[Timer] STOP requested for tag: " << tag <<" on thread " << tid <<  "\n";
                    h.resume();
                    std::cout << "[Timer] Resumed coroutine for tag: " << tag
                          << " from thread " << tid << "\n";
                    std::this_thread::sleep_for(1ms);        
                    return;                        //    exit silently
                }
                auto now = clock::now();
                if (now >= tgt) {
                    std::cout << "[Timer] Resuming coroutine for tag: " << tag
                          << " from thread " << tid << "\n";
                    h.resume();                    // normal wake‑up
                    return;
                }
                auto remaining = tgt - now;         // clock::duration
                std::this_thread::sleep_for(
                    std::min<clock::duration>(kSlice, remaining));  //  slice sleep
            }
        }).detach();
    }
    void await_resume() const noexcept {}
};

template<typename T>
class CoroutineManager {
public:
    explicit CoroutineManager(std::size_t maxQ = 64)
        : queue_{maxQ} {}

    /*──────────────────── 1.  coroutine factories ───────────────────*/
    Task makeProducer(int first = 0, std::string tag = "") {
        std::cout <<"[CoroutineManager] "<< tag <<" Coroutine Started\n";
        co_await TagSetter<Task::promise_type>{tag};
        for (int n = first; !stopToken_.stop_requested(); ++n) {
            co_await queue_.send(n);
            TRACE(tag << " produced " << n);
            co_await TimerAwaitable(20ms, stopToken_, tag);          // non‑blocking delay
        }
       TRACE("Producer " << tag << " got  STOP requested Coroutine exiting...");
       co_return;
    }

    Task makeConsumer(std::string tag) {
        std::cout <<"[CoroutineManager] "<< tag <<" Coroutine Started\n";
        co_await TagSetter<Task::promise_type>{tag};
        while (true) {
            auto opt = co_await queue_.next();   // optional<T>
            if (!opt) {
                TRACE(tag << " exiting – queue closed");
                break;
            }
            std::cout << tag << " got " << *opt << '\n';
        }
        co_return;
    }

    /*──────────────────── 2.  registration API ─────────────────────*/
    void addProducer(int first = 0)  {
        producers_.emplace_back(makeProducer(first, "P" + std::to_string(first)));
        TRACE("Added Producer first " << first << " - P" << std::to_string(first));
    }
    void addConsumer(std::string tag) {
        consumers_.emplace_back(makeConsumer(tag));
        TRACE("Added Consumer tag " << tag);
    }

    /*──────────────────── 3.  lifecycle control ────────────────────*/
    void start() {
        for (auto& p : producers_)  p.start();   //first resume
        for (auto& c : consumers_)  c.start();
        started_ = true;
        for (auto& p : producers_)  p.resume();  //second resume
        for (auto& c : consumers_)  c.resume();        
    }

    void requestStop() {
        stopSrc_.request_stop();   // 1) tell coroutines to exit their loops
        queue_.close();            // 2) wake consumers that are waiting
        stopped_ = true;
    }

    void wait() {
        while (!allDone()) {
            std::this_thread::sleep_for(2ms);   // avoid spin
        }
    }

    /*──────────────────── 4.  diagnostics ──────────────────────────*/
    void status() const {
        std::cout << "Producers: " << producers_.size()
                  << " Consumers: " << consumers_.size()
                  << " Queue size: " << queue_.bufferSize()
                  << " Started: " << started_
                  << " Stopped: " << stopped_
                  << '\n';
    }

private:
    bool allDone() const {
        auto finished = [](auto const& vec) {
            return std::all_of(vec.begin(), vec.end(),
                               [](auto const& t){ return t.done(); });
        };
        return finished(producers_) && finished(consumers_);
    }

    /*──────────────────  data members  ───────────────────*/
    CoroutineQueue<T> queue_;
    std::stop_source  stopSrc_;
    std::stop_token   stopToken_ = stopSrc_.get_token();

    std::vector<Task> producers_;
    std::vector<Task> consumers_;

    bool started_{false};
    bool stopped_{false};
};

///////////////////////////////////////
using Timestamp = std::chrono::system_clock::time_point;

struct MarketDataUpdate {
    std::string symbol;
    double lastPrice;
    double bidPrice;
    int bidSize;
    double askPrice;
    int askSize;
    Timestamp timestamp;
    std::string quoteToStr() const {
        std::ostringstream oss;
        oss << "Quote: " << symbol << "\t" << lastPrice << "\tBid " << 
        bidPrice <<"[" << bidSize << "]  \tAsk " << askPrice <<"[" <<askSize <<
        "]" << "  \t" << printTime() ;
        return oss.str();
    }
    
    std::string printTime() const {
        auto ms = duration_cast<std::chrono::milliseconds>(timestamp.time_since_epoch()) % 1000;

        std::time_t tt = std::chrono::system_clock::to_time_t(timestamp);
        std::tm tm = *std::localtime(&tt);

        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
        oss << '.' << std::setfill('0') << std::setw(3) << ms.count();

        return oss.str();
    }
    friend std::osyncstream& operator<<(std::osyncstream& os, const MarketDataUpdate& md) {
    //friend std::ostream& operator << (std::ostream& os, const MarketDataUpdate& md) {
        os << md.quoteToStr();
        return os;
    }
    
};


class Info {
public:
    static void init(std::vector<std::string>&& sec) {
        std::call_once(initFlag_, [&]() {
            instance_ = std::make_unique<Info>(std::move(sec));
        });
    }

    static Info& getInstance() {
        if (!instance_) {
            throw std::runtime_error("Info::init() must be called before Info::instance()");
        }
        return *instance_;
    }
    
    struct alignas(64) LocalRNG {
        std::random_device rd;
        std::mt19937 gen;
        std::uniform_int_distribution<int> qtyDist;
        std::uniform_real_distribution<double> priceDist;
        std::uniform_int_distribution<int> secDist;
        
        // Initialize distributions in constructor
        LocalRNG(int minQty, int maxQty, double minPrice, double maxPrice, int secSize)
            : gen(rd()),
              qtyDist(minQty, maxQty),
              priceDist(minPrice, maxPrice),
              secDist(0, secSize - 1) {}  // random index for symbol Info
                                          // std::vector<std::string> secData
    };
    

    Info(const Info&) = delete;
    Info& operator=(const Info&) = delete;
    Info(Info&&) noexcept = default;
    Info& operator=(Info&&) noexcept = default; 

    ~Info() = default;
    void publish(const std::string& pName="publisher_1 ", int nMsg=20, int timeout=10) {
        thread_local LocalRNG rng(500, 1000, 100.0, 200.0, secData.size());

        for (int ii=0; ii < nMsg; ++ii) {
            const double lastPrice = rng.priceDist(rng.gen);
            MarketDataUpdate mdu{
            .symbol = secData[rng.secDist(rng.gen)],
            .lastPrice = lastPrice ,
            .bidPrice = lastPrice - 0.05,
            .bidSize = rng.qtyDist(rng.gen),
            .askPrice = lastPrice  + 0.10,
            .askSize = rng.qtyDist(rng.gen),
            .timestamp = std::chrono::system_clock::now() - std::chrono::hours(4)
            };
            std::osyncstream sync_out(std::cout);
            if(!pName.empty()) 
                sync_out << pName;
            sync_out << mdu << "\n";
            std::this_thread::sleep_for(milliseconds(timeout)); 
        }
    }
    //private:

    Info(std::vector<string>&& sec) :
    secData(std::move(sec))
    {}
    
    size_t symbolsSize() const { return secData.size(); }
    const std::vector<std::string>& symbols() const { return secData; }
private:
    static std::unique_ptr<Info> instance_;
    static std::once_flag initFlag_;

    std::vector<std::string> secData;
};


std::unique_ptr<Info> Info::instance_ = nullptr;
std::once_flag Info::initFlag_;

//free function prototype
Task mdProducer(CoroutineQueue<MarketDataUpdate>& queue, 
           const std::string& name = "publisher_1 ", int nMsg = 25, int timeoutMs = 10) {
    thread_local Info::LocalRNG rng(500, 1000, 100.0, 200.0, Info::getInstance().symbolsSize());

    for (int ii = 0; ii < nMsg; ++ii) {
        const double lastPrice = rng.priceDist(rng.gen);
        MarketDataUpdate mdu{
            .symbol = Info::getInstance().symbols()[rng.secDist(rng.gen)],
            .lastPrice = lastPrice,
            .bidPrice = lastPrice - 0.05,
            .bidSize = rng.qtyDist(rng.gen),
            .askPrice = lastPrice + 0.10,
            .askSize = rng.qtyDist(rng.gen),
            .timestamp = std::chrono::system_clock::now() - std::chrono::hours(4)
        };

        std::osyncstream sync_out(std::cout);
        if (!name.empty())
            sync_out << name;
        sync_out << mdu;

        co_await queue.send<MarketDataUpdate>(std::move(mdu));  // Push into queue

        //co_await TimerAwaitable(std::chrono::milliseconds(timeoutMs));  // throttle
    }
}


class ThreadPool {
public:
explicit ThreadPool(size_t num_threads=std::thread::hardware_concurrency()) 
    :Nthreads(num_threads) {
    TRACE("[ThreadPool] N threads "<< Nthreads);  
    for (size_t ii=0; ii <Nthreads; ++ii) {
        workers.emplace_back(
            [this, ii]() {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock lock(mtx_);
                        cv_.wait(lock, [this]() {
                            return !tasks.empty() || stop.load(std::memory_order_acquire) ;
                        });
                        if (stop.load(std::memory_order_acquire) )
                            break;
                        task=tasks.front();
                        tasks.pop();
                    }
                    n_tasks.fetch_add(1, std::memory_order_relaxed);
                    task();
                }
                TRACE("[ThreadPool] Thread N "<< ii+1 <<" " << std::this_thread::get_id() << " Stopped...");
                nDone.fetch_add(1, std::memory_order_relaxed);    
            });    
        TRACE("[ThreadPool] Thread N "<< ii+1 <<" created");
    }
}
~ThreadPool() {
    shutdown();
}

void shutdown() {

    if (stop.exchange(true, std::memory_order_release))
        return; // already stopped
    
    cv_.notify_all();
    for (auto& thread : workers) {
        if (thread.joinable()) thread.join();
    }
    workers.clear();
    TRACE("ThreadPool shutdown... processed "<< n_tasks << " tasks");
}

void addTask(std::function<void()> fn) {
    {
            std::lock_guard lock(mtx_);
            tasks.emplace(std::move(fn));
    }
    cv_.notify_one();
}

private:
    size_t Nthreads;
    atomic<bool> stop{false};    
    std::vector<std::thread> workers;
    atomic<int> nDone;
    std::queue<std::function<void()>> tasks;
    std::mutex mtx_;
    std::condition_variable cv_;
    atomic<int> n_tasks;
};

int aa=0;
void consolidate(const MarketDataUpdate md, const std::string tag) {
    aa++;
}

class Processor {
public:
    Processor() = default;
    ~Processor() {
        stop();
    }

    void add(const MarketDataUpdate& md) {
        checkSymbolMutex(md.symbol);
        
        std::lock_guard lock(*symbolMutex_[md.symbol]); //lock mutex by symbol
        
        //add MarketDataUpdate
        auto& sDl=symbolMap_[md.symbol];           //list MarketDataUpdate
        sDl.push_back(md);                        //return void deque
        auto itt=sDl.end();
        itt--;                                    //iterator on last element on list
        auto& tsMap = timestampIndex_[md.symbol]; //map[timestamp]
        tsMap[md.timestamp] = itt;    
        if (tsMap.size()>windowSize) {
           tsMap.erase(tsMap.begin());
        }
        
    }
    void start() {
        if (started_.exchange(true, std::memory_order_release))
            return;
        //start timerThread_
        timerThread_= std::jthread([this] (std::stop_token stopTok) {
           while (!stopTok.stop_requested()) {
               this_thread::sleep_for(sleepTime);
               ++tickCount_;
               onTimer();
               if (tickCount_%3==0) {
                   cleanup();
               }
           } 
        });
    }
    
    void stop() {
        if (avgReport.empty()) {
            TRACE("empty avgReport !!!!!!!");
        }
        TRACE("AvgReport ============================");
        for (const auto& st : avgReport)
            TRACE(st);
        TRACE("=======================================");   
    }

private:
    void checkSymbolMutex(const std::string& symbol) {
        std::lock_guard lock(globalInitMtx);
        if (!symbolMutex_.count(symbol)) {
            symbolMutex_[symbol] = std::make_unique<std::mutex>();
        }
    }

    void onTimer() {     // Called every 2s
        for (const auto& [symbol, _] : symbolMap_) {
            checkSymbolMutex(symbol);
            {
                std::lock_guard lock(*symbolMutex_[symbol]); //lock mutex by symbol
                processLastN(symbol);                
            }
        }
    }
    void cleanup() {   // Called every 6s (every 3rd tick)
    //std::cout << "[Processor] Running cleanup safely...\n";
        for (auto& [symbol, tsMap] : timestampIndex_) {
        checkSymbolMutex(symbol);
        std::lock_guard lock(*symbolMutex_[symbol]);
        if (!symbolMap_.count(symbol)) continue;
        auto& dl = symbolMap_[symbol];  // std::list
            for (auto it = dl.begin(); it != dl.end(); ) {
                if (tsMap.count(it->timestamp)==0) {
                    it = dl.erase(it);
                } else {
                    ++it;
                }
            }
        }
    }   
    
    void processLastN(const std::string& symbol) {  // Core consolidation
        const auto& tsMap = timestampIndex_[symbol];
        if (tsMap.empty()) return;

        double totalPrice = 0;
        int count = 0;

        for (const auto& [_, it] : tsMap) {
            totalPrice += it->lastPrice;
            ++count;
        }

        if (count > 0) {
            double avg = totalPrice / count;
            std::stringstream bufP;
            bufP << "[Processor] (tick "<< tickCount_ << ") " << symbol << " avg price over last "
                  << count << " ticks = " << avg;
            avgReport.push_back(std::move(bufP.str()));     
        }
    }

    using DL = std::list<MarketDataUpdate>;
    using TimestampMap = std::map<Timestamp, DL::iterator>;  // timestamp => iterator

    std::unordered_map<std::string, DL> symbolMap_;                 //{symbol std::list<MarketDataUpdate>{
    std::unordered_map<std::string, TimestampMap> timestampIndex_;  //{symbol, std::map<Timestamp, DQ::iterator>}

    //std::mutex mtx_;
    
    std::unordered_map<std::string, std::unique_ptr<std::mutex>> symbolMutex_;
    std::mutex globalInitMtx;
    
    std::jthread timerThread_;
    std::atomic<bool> started_ = false;
    std::chrono::seconds sleepTime = 2s;
    size_t windowSize{50};

    int tickCount_ = 0;
    std::vector<std::string> avgReport;
};

template<>
class CoroutineManager<MarketDataUpdate> {
public:
    using quePtr = std::shared_ptr<CoroutineQueue<MarketDataUpdate>>;
    explicit CoroutineManager(std::size_t maxQ = 100, int msgN=25)
        : maxQsize(maxQ), msgN_(msgN) {
            
        }

    /*──────────────────── 1.  coroutine factories ───────────────────*/
    Task makeProducer(CoroutineQueue<MarketDataUpdate>& queue, 
        std::string name, int nMsg , int timeoutMs = 10) {
        //thread_local Info::LocalRNG rng(500, 1000, 100.0, 200.0, Info::getInstance().symbolsSize());
        Info& info = Info::getInstance();
        Info::LocalRNG rng(500, 1000, 100.0, 200.0, info.symbolsSize());
        std::cout <<"[CoroutineManager makeProducer ] "<< name <<" Coroutine Started\n";
        co_await TagSetter<Task::promise_type>{name};
        int n_msg=0;
        for (int ii = 0; ii < nMsg && !stopToken_.stop_requested() ; ++ii) {
            const double lastPrice = rng.priceDist(rng.gen);
            MarketDataUpdate mdu{
                .symbol = Info::getInstance().symbols()[rng.secDist(rng.gen)],
                .lastPrice = lastPrice,
                .bidPrice = lastPrice - 0.05,
                .bidSize = rng.qtyDist(rng.gen),
                .askPrice = lastPrice + 0.10,
                .askSize = rng.qtyDist(rng.gen),
                .timestamp = std::chrono::system_clock::now() - std::chrono::hours(4)
            };
            /*
            std::osyncstream sync_out(std::cout);
            if (!name.empty())
                sync_out << name <<" ";
            sync_out << mdu;
            */
            if (n_msg++<nLogMsg)
                TRACE("[Producer " << (name.empty()?"":name+" ") << "] "<< mdu);
            co_await queue.send<MarketDataUpdate>(std::move(mdu));  // Push into queue
            co_await TimerAwaitable(std::chrono::milliseconds(timeoutMs), stopToken_, name);  // throttle
        }
        if (stopToken_.stop_requested()) {
            TRACE("Producer " << name << " got  STOP requested");
        }
        TRACE("Producer " << name << " Done, Queue size "<<queue.bufferSize() <<" Coroutine exiting...");
        status();
        queue.close();
        co_return;
    }

    Task makeConsumer(CoroutineQueue<MarketDataUpdate>& queue, std::string tag, int& nMes) {
        std::cout <<"[CoroutineManager makeConsumer ] "<< tag <<" Coroutine Started\n";
        co_await TagSetter<Task::promise_type>{tag};
        int n_msg=0;
        while (true) {
            auto opt = co_await queue.next();   // optional<T>
            if (!opt) {
                TRACE(tag << " exiting – queue closed");
                break;
            }
           nMes++;    
           if (n_msg++<nLogMsg)
                TRACE("[Consumer " << tag << "] got " << *opt  << "\t(" << nMes <<")");
           
           threadPool_->addTask([md = std::move(*opt), tag, this] { 
               processor_.add(md);
            });
           
        }
        co_return;
    }

    /*──────────────────── 2.  registration API ─────────────────────*/
    void addThreadPool(ThreadPool* tp) {
        threadPool_=tp;
    }
    void addProducer(std::string tag)  {
        if(quePool.count(tag)) {
           TRACE("Added Producer "<< tag << " failed, Producer already exist...");
           return;
        }
        quePtr qPtr= std::make_shared<CoroutineQueue<MarketDataUpdate>>(maxQsize);
        quePool[tag]=qPtr;
        producers_.emplace_back(makeProducer(*qPtr, tag, msgN_, throttle));
        TRACE("Added Producer " << tag );
    }
    void addConsumer(std::string producer, std::string tag) {
        auto it=quePool.find(producer);
        if (it==quePool.end()) {
            TRACE("Added Consumer "<< tag << " failed, no such Producer as " << producer << "\n");
            return;
        } 
        
        consumers_.emplace_back(makeConsumer(*it->second, tag, nConsData));
        TRACE("Added Consumer tag " << tag);
    }
    void createSystem(int nProd, int nCons) {
        if (!nCons or !nProd) {
            TRACE("createSystem not supporting nProd " << nProd << " nCons " << nCons );
            return;
        }
        TRACE("createSystem N Producers " << nProd << " N consumers by producer " << nCons);
        int jj=1;
        for (int i=1; i <=nProd; ++i) {
            std::string ptag="Prod_"+std::to_string(i);
            addProducer(ptag);
            for (int j=1; j<=nCons; ++j) {
                std::string ctag="C"+std::to_string(jj++);
                addConsumer(ptag,ctag);
            }
        }
    }    

    /*──────────────────── 3.  lifecycle control ────────────────────*/
    void start() {
        processor_.start();
        
        for (auto& p : producers_)  p.start();   //first resume
        for (auto& c : consumers_)  c.start();
        started_ = true;
        for (auto& p : producers_)  p.resume();  //second resume
        for (auto& c : consumers_)  c.resume();        
    }

    void requestStop() {
        stopSrc_.request_stop();   // 1) tell coroutines to exit their loops
        //queue_.close();            // 2) wake consumers that are waiting
        for (const auto& [_prod, qPtr] : quePool ){
            qPtr->close();
        }
        stopped_ = true;
    }

    void wait() {
        while (!allDone()) {
            std::this_thread::sleep_for(2ms);   // avoid spin
        }
    }

    /*──────────────────── 4.  diagnostics ──────────────────────────*/
    void status() const {
        TRACE("Status " << "Producers: " << producers_.size()
                  << " Consumers: " << consumers_.size()
//                  << " Queue size: " << queue_.bufferSize()
                  << " Started: " << started_
                  << " Stopped: " << stopped_
                  << " Consumed msgs " << nConsData
                  );
    }

private:
    size_t maxQsize;
    bool allDone() const {
        auto finished = [](auto const& vec) {
            return std::all_of(vec.begin(), vec.end(),
                               [](auto const& t){ return t.done(); });
        };
        return finished(producers_) && finished(consumers_);
    }

    /*──────────────────  data members  ───────────────────*/
    //CoroutineQueue<MarketDataUpdate> queue_;
    std::map<std::string, quePtr> quePool;
    std::stop_source  stopSrc_;
    std::stop_token   stopToken_ = stopSrc_.get_token();

    std::vector<Task> producers_;
    std::vector<Task> consumers_;

    bool started_{false};
    bool stopped_{false};
    int nConsData{0};
    int msgN_;
    int nLogMsg{10};
    int throttle{50};
    ThreadPool* threadPool_;
    Processor processor_;
};



int main()
{
    std::cout<<"Hello World\n";
    {
        //
        cout << "producer–consumer system using coroutines ===================\n";
        CoroutineManager<int> mgr(8);
        mgr.addProducer(0);
        mgr.addProducer(1000);
        mgr.addConsumer("C1");
        mgr.addConsumer("C2");

        mgr.start();
        std::this_thread::sleep_for(200ms);
        mgr.requestStop();
        mgr.wait();

        mgr.status();
    }
    {
        cout << "MarketDataUpdate !!!!! ================================\n";
        Info::init({"MSFT", "AAPL", "SPY", "TSLA", "NVDA", "DAL", "UAL", "SEDG"});
        auto& info = Info::getInstance();
        info.publish("Pub_1 ");
/*
        std::async does NOT use default arguments for member function calls. 
        need to explicitly pass them, if not all 3, it will fail to compile
        auto ff=std::async(std::launch::async, &Info::publish, &info, "p1 ", 5,10);
        ff.get();
*/

/*
        auto future = std::async(std::launch::async, []{
                Info::getInstance().publish("Pub_2 ", 10, 50);
            });        
        future.wait();
*/
        std::vector<future<void>> futures;
        int futSize=5;
        for (int ii=0; ii< futSize; ++ii) {
            futures.push_back(std::async(std::launch::async,
                [ii]() {
                  Info::getInstance().publish("Pub_"+std::to_string(ii+1)+" ",
                  50, 10+ii*5);
            }
            ));
        }
        // Wait for all
        for (auto& f : futures) {
            f.wait();
        }
        /*
        CoroutineQueue<MarketDataUpdate> queueMd(100);
        auto prodTask = mdProducer(queueMd);
        prodTask.resume();
        std::this_thread::sleep_for(500ms);
        */
        cout << "======================================================================\n";
        ThreadPool tp(6);
        CoroutineManager<MarketDataUpdate> mgr(50, 1000);
        /*std::string prName("Pr1");
        mgr.addProducer("Pr1");
        mgr.addConsumer("Pr1", "C1");
        mgr.addConsumer("Pr1", "C2");
        */
        mgr.addThreadPool(&tp);
        mgr.createSystem(5,2);
        mgr.start();
        mgr.wait();
        tp.shutdown();
    }

    return 0;
}

