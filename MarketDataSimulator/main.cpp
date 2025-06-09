/******************************************************************************

Welcome to GDB Online.
  GDB online is an online compiler and debugger tool for C, C++, Python, PHP, Ruby, 
  C#, OCaml, VB, Perl, Swift, Prolog, Javascript, Pascal, COBOL, HTML, CSS, JS
  Code, Compile, Run and Debug online from anywhere in world.

*******************************************************************************/
#include <stdio.h>
#include <iostream>
#include <vector>
#include <map>
#include <utility>
#include <sstream>
#include <string>
#include <limits> // For std::numeric_limits
#include <ios> 
#include <set>

#include <queue>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <memory>
//Coroutine
#include <generator>         // for std::generator
#include <variant>           // for std::variant
#include <random>
#include <ranges>
#include <unordered_map>

using namespace std;

struct CommitData {
    string path;
    string id;
};

struct QueryData {
    unsigned long long timestampStart;
    unsigned long long timestampEnd;
    CommitData data;
};

using CommitKey = pair<unsigned long long, unsigned long long>; // <timestamp, id>
using CommitMap = map<CommitKey, vector<CommitData>>;

void readCin(CommitMap& dataInfo, vector<QueryData>& queryData) {
    //input
    // N 6
    /*
    id 8 timestamp 200 quicksort.cpp 839ad0 mergesort.cpp 0cdde1 bubblesort.cpp 248dd1
    id 0 timestamp 500 array.h 163111 sequence.h 294def 
    id 6 timestamp 200 mergesort.cpp 0cdde1 bogosort.cpp 4213ff
    id 4 timestamp 1000 array.h 163111 vector.h fcc2af 
    id 2 timestamp 300 bubblesort.cpp 248dd1 bogosort.cpp 4213ff
    id 3 timestamp 300 bubblesort.cpp eaf888 bogosort.cpp 4f11aa
    */
    //N 4
    /*
    0 10000 quicksort.cpp 839ad0
    0 500 vector.h fcc2af
    0 100000 no_found.h empty_response
    100 200 bogosort.cpp 4213ff
    */
    map<string, string> fileIdMap; // ambiguous check path -> first opaque_id seen
    int N;
    cin >> N;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    //discard characters from the input buffer to process getLine next
    //Ensures the buffer is fully cleared, no matter how much junk is left.
    for (int i = 0; i < N; ++i) {
        string inputLine;
        std::getline(std::cin, inputLine);
        //cin >> inputLine;
        //cout << "inputLine " << inputLine << endl;
        istringstream isLine(inputLine);
        unsigned long long timestamp;
        unsigned long long id;
        CommitData data;
        string word;
        isLine >> word;
        isLine >> id;
        isLine >> word;
        isLine >> timestamp;
        vector<CommitData> newCommit;
        //cout << "Got here " << id << " " << timestamp << endl;
        
        while (isLine >> data.path >> data.id) {
        //   cout << "Got here 1111111111\n";
           auto it = fileIdMap.find(data.path);
           if (it==fileIdMap.end()) {
               fileIdMap.insert({data.path, data.id});
           } else {
               if (it->second != data.id) {
                  cout << "Ambiguous input; Skipping path " << data.path << " id " << data.id << endl;
                  continue; 
               }
           }
           newCommit.push_back(data);
        }
        if (newCommit.size()>0) 
           dataInfo[{timestamp,id}]=newCommit; 
    }
    cout << "Query input\n";
    cin >> N;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    //discard characters from the input buffer to process getLine next
    //Ensures the buffer is fully cleared, no matter how much junk is left.
    QueryData qd;
    for (int i = 0; i < N; ++i) {
        string inputLine;
        std::getline(std::cin, inputLine);
        istringstream isLine(inputLine);
        while (isLine >> qd.timestampStart >> qd.timestampEnd >> qd.data.path >> qd.data.id) {
            queryData.push_back(qd);
        }
    }    
}

//bool needInput=true;
bool needInput=false;

std::vector<unsigned long long> processQuery(const CommitMap& dataInfo,
                                    const QueryData& qd) {
    std::vector<unsigned long long> result;
    //read map create connectedPaths set
    std::set<std::string> connectedPaths; // all path in sutisfy commit 
    for (const auto& data : dataInfo) {
        auto timestamp=data.first.first;
        if (timestamp < qd.timestampStart) 
            continue;
        if (timestamp > qd.timestampEnd)
            break;
        for (const auto& commit :data.second) {
            if (commit.path==qd.data.path && commit.id==qd.data.id) {
                    for (const auto& c : data.second) {
                    connectedPaths.insert(c.path);
                    }
                break;
            }
        }
    }
    //read map again if path in connectedPaths add id to result
    for (const auto& data : dataInfo) {
        auto timestamp=data.first.first;
        if (timestamp < qd.timestampStart) 
            continue;
        if (timestamp > qd.timestampEnd)
            break;
        for (const auto& commit :data.second) {
            if (connectedPaths.count(commit.path)) {
                result.push_back(data.first.second);
                break;
            }   
        }
    }
    return result;
}
  
  
//Logger   
enum class LogLevel {
    INFO,
    WARN,
    ERROR,
    DEBUG
};   
    
struct LogEvent {
    std::string timestamp;
    std::thread::id threadId;
    LogLevel level;
    std::string message;
    /*
    ~LogEvent() {
        cout << "LogEvent Destructor called\n";
    }
    */
};

class Logger {
public:
    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }

    void start();      // Starts the logger thread
    
    void log(LogLevel level, const std::string& message);  // Producer method

    // Convenience shorthands
    void info(const std::string& msg)  { log(LogLevel::INFO, msg); }
    void warn(const std::string& msg)  { log(LogLevel::WARN, msg); }
    void error(const std::string& msg) { log(LogLevel::ERROR, msg); }
    void debug(const std::string& msg) { log(LogLevel::DEBUG, msg); }

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
private:
    void stop();       // Signals shutdown and joins thread

    Logger() = default;
    ~Logger() {
        cout << "Logger Destructor Called, stopping Loger\n";    
        stop(); 
    }

    void process();    // Consumer thread function
    std::string currentTimestamp();  // Timestamp helper
    std::string toString(LogLevel level);
    void log_event(const LogEvent& ev);

    std::queue<shared_ptr<LogEvent>> queue_;
    std::mutex queueMutex_;
    std::condition_variable condVar_;
    std::atomic<bool> running_{false};
    std::thread worker_;
    // Shared atomic flag for prin synchronization
    std::atomic_flag printLock = ATOMIC_FLAG_INIT;
};

std::string Logger::currentTimestamp() {
        using namespace std::chrono;

        auto now = system_clock::now() - std::chrono::hours(4);  //NYC
        auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;

        std::time_t tt = system_clock::to_time_t(now);
        std::tm tm = *std::localtime(&tt);

        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
        oss << '.' << std::setfill('0') << std::setw(3) << ms.count();

        return oss.str();
}

void Logger::log(LogLevel level, const std::string& message) {
/*
    LogEvent event{
        currentTimestamp(),
        std::this_thread::get_id(),
        level,
        message
    };
*/
    shared_ptr<LogEvent> event =std::make_shared<LogEvent>(
            currentTimestamp(),
            std::this_thread::get_id(),
            level,
            message
    );
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        queue_.push(std::move(event));
    }
    condVar_.notify_one();
}

void Logger::process() {
    while (true) {
        std::unique_lock<std::mutex> lock(queueMutex_);
        condVar_.wait(lock, [&] {
                return !running_.load(std::memory_order_acquire) ||
                   !queue_.empty();
            });
        if (!running_.load(std::memory_order_acquire)) {
            while (!queue_.empty()) {
                shared_ptr<LogEvent> event=std::move(queue_.front());
                queue_.pop();
                log_event(*event);
            }
            break;
        }    
        shared_ptr<LogEvent> event=std::move(queue_.front());
        queue_.pop();
        lock.unlock();
        std::this_thread::yield();
        log_event(*event);
    }
}

void Logger::log_event(const LogEvent& ev) {
    while (printLock.test_and_set(std::memory_order_acquire)) {
        // Busy-wait
        std::this_thread::yield();
    }
    std::cout << "[" << ev.timestamp << "] "
              << "[" << toString(ev.level) << "]:"
              << ev.threadId << " " 
              << ev.message << "\n";
    printLock.clear(std::memory_order_release);
}

std::string Logger::toString(LogLevel level) {
    switch (level) {
            case LogLevel::INFO:  return "INFO";
            case LogLevel::WARN:  return "WARN";
            case LogLevel::ERROR: return "ERROR";
            case LogLevel::DEBUG: return "DEBUG";
    }
    return "UNKNOWN";
}

void Logger::start() {
    if (running_.exchange(true)) return;  // already running

    worker_ = std::thread([this]() {
        this->process();
    });
}

void Logger::stop() {
    if (!running_.exchange(false)) return;  // already stopped
    condVar_.notify_all();  // Wake thread so it can exit

    if (worker_.joinable()) {
        worker_.join();
    }
}




void testLoger() {
    Logger::getInstance().start();
    int num_threads=10;
    std::vector<std::thread> threads;
    for (size_t i = 0; i < num_threads; ++i) {
        threads.emplace_back([i]() {
            for (size_t jj = 0; jj < 3; ++jj) {
                Logger::getInstance().info(string("From thread N ")+to_string(i+1) + 
                string(" Hello N ") + to_string(jj+1));
                
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }     
        });     
    }
    for (auto& th : threads) {
        th.join();  // Joins once all tasks are done
    }
    cout << "All Hello is done....\n";
}

///////////// Coroutine
/*
[ Coroutine MarketDataFeed ]
[ ThreadSafeQueue<MDataEvent> ]
[ TradeProcessor ]
[ Simulated Matching Engine ]
[ Executed Trades / Logs / Analytics ]


*/


using Timestamp = std::chrono::system_clock::time_point;

struct MarketDataUpdate {
    std::string symbol;
    double lastPrice;
    double bidPrice;
    int bidSize;
    double askPrice;
    int askSize;
    Timestamp timestamp;
    std::string quoteToStr() {
        std::ostringstream oss;
        oss << "Quote: " << symbol << "\t" << lastPrice << " Bid " << 
        bidPrice <<"[" << bidSize << "] Ask " << askPrice <<"[" <<askSize <<
        "]" << "\t" << printTime() ;
        return oss.str();
    }
    
    std::string printTime() {
        auto ms = duration_cast<std::chrono::milliseconds>(timestamp.time_since_epoch()) % 1000;

        std::time_t tt = std::chrono::system_clock::to_time_t(timestamp);
        std::tm tm = *std::localtime(&tt);

        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
        oss << '.' << std::setfill('0') << std::setw(3) << ms.count();

        return oss.str();
    }
};


struct StopSignal {};

using MDataEvent = std::variant<MarketDataUpdate, StopSignal>;

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
    
    string getSecurity() {
        return secData[secDist(gen)];
    }

    int getQuantity() {
        return qtyDist(gen);
    }

    double getPrice() {
        return priceDist(gen);
    }

    Info(const Info&) = delete;
    Info& operator=(const Info&) = delete;
    Info(Info&&) noexcept = default;
    Info& operator=(Info&&) noexcept = default; 

    ~Info() = default;
//private:

    Info(std::vector<string>&& sec) :
    gen(rd()), secData(std::move(sec)),
    qtyDist(500, 1000), priceDist(100.0, 200.0),
    secDist(0,sec.size()-1) {}

private:
    static std::unique_ptr<Info> instance_;
    static std::once_flag initFlag_;
    
    std::random_device rd;
    std::mt19937 gen;
    std::uniform_int_distribution<int> qtyDist;
    std::uniform_real_distribution<> priceDist;
    std::uniform_int_distribution<int> secDist;
    std::vector<std::string> secData;
};

std::unique_ptr<Info> Info::instance_ = nullptr;
std::once_flag Info::initFlag_;

template <typename T>
class ThreadSafeQueue {
public:
    ThreadSafeQueue() = default;

    // Disable copy & assignment
    ThreadSafeQueue(const ThreadSafeQueue&) = delete;
    ThreadSafeQueue& operator=(const ThreadSafeQueue&) = delete;

    // Push an element into the queue (thread-safe)
    void push(T item) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            queue_.push(std::move(item));
        }
        std::this_thread::yield();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        condVar_.notify_one();
    }

    // Wait and pop an element from the queue
    T wait_and_pop() {
        std::unique_lock<std::mutex> lock(mutex_);
        condVar_.wait(lock, [this]{ return !queue_.empty(); });
        T item = std::move(queue_.front());
        queue_.pop();
        return item;
    }

    // Try to pop an element, returns std::optional<T>
    std::optional<T> try_pop() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.empty())
            return std::nullopt;
        T item = std::move(queue_.front());
        queue_.pop();
        return item;
    }

    // Check if queue is empty
    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }
    size_t getSize() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }
private:
    mutable std::mutex mutex_;
    std::queue<T> queue_;
    std::condition_variable condVar_;
};


// Best Bid and Ask
struct QuoteBook {
    double bestBidPrice;
    int bidSize;
    double bestAskPrice;
    int askSize = 0;
    std::chrono::system_clock::time_point timestamp;
    QuoteBook() 
    : bestBidPrice(0.0),
      bidSize(0),
      bestAskPrice(std::numeric_limits<double>::max()),
      askSize(0),
      timestamp(std::chrono::system_clock::now() - std::chrono::hours(4)) {
      }
};

class IStrategy {
public:
    virtual bool onQuote(const MarketDataUpdate&, const QuoteBook& book) = 0;
    virtual ~IStrategy() = default;
};

class NarrowSpreadStrategy : public IStrategy {
public:
    NarrowSpreadStrategy() {}

    bool onQuote(const MarketDataUpdate& q, const QuoteBook& book) override {
        if (spreadLimits_.count(q.symbol) == 0) 
            return false;
        double threshold_=spreadLimits_[q.symbol];
        if ((book.bestAskPrice - book.bestBidPrice) <= threshold_) {
            std::ostringstream oss;
            oss << "[NarrowSpread] Signal on " <<
            q.symbol << " Create Limit order to buy on " << book.bestBidPrice << 
            " size " << book.bidSize;
            Logger::getInstance().info(oss.str()); 
            return true;
        }
        return false;
    }
private:

    std::unordered_map<std::string, double> spreadLimits_{
        {"AAPL", 0.8},
        {"SPY", 0.5},
        {"MSFT", 0.7}
    };
};

class PriceBreakoutStrategy : public IStrategy {
public:
    PriceBreakoutStrategy(size_t windowSize = 10) 
    :windowSize_(windowSize)
    {}
    bool onQuote(const MarketDataUpdate& q, const QuoteBook& book) override {
        auto& window = priceWindow_[q.symbol];
        
        window.push_back(q.lastPrice);
        if (window.size() > windowSize_) {
            window.pop_front(); // remove oldest
        }
        const auto& [low, high] = std::minmax_element(window.begin(),window.end());
        if (low == high)
            return false;
        std::ostringstream oss;
        if (q.lastPrice==*high) {
            oss<< "[PriceBreakoutStrategy] Signal on " << q.symbol << " lastPrice " << q.lastPrice <<
            " Create Limit order to sell on " << book.bestAskPrice << " size " << book.askSize;
            Logger::getInstance().info(oss.str());
            return true;
        }
        else if (q.lastPrice==*low) {
            oss << "[PriceBreakoutStrategy] Signal on " + q.symbol << " lastPrice " << q.lastPrice <<
            " Create Limit order to buy on " << book.bestBidPrice << " size " << book.bidSize;
            Logger::getInstance().info(oss.str()); 
            return true;
        }
        return false;
    }

private:
    size_t windowSize_;
    std::unordered_map<std::string, std::deque<double>> priceWindow_;
    //keep symbol deque of lastPrice size of windowSize_
};

enum class signalSide {
    Buy,
    Sell
}; 


class MovingAvCStrategy : public IStrategy {
public:
MovingAvCStrategy(size_t shortWindow = 3, size_t longWindow = 10)
    :
    shortWindow_(shortWindow),
    longWindow_(longWindow)
{
    
}

bool onQuote(const MarketDataUpdate& q, const QuoteBook& book) override {
        auto& window = priceHistory_[q.symbol];
        window.push_back(q.lastPrice);
        if (window.size() < longWindow_)
            return false;
        if (window.size() > longWindow_)
            window.pop_front(); 
        double longMA  = std::accumulate(window.begin(), window.end(), 0.0)/window.size();
        /*
        STL containers that support Random Access Iterators include: 
        std::vector
        std::deque
        std::string
        std::array
        
        support iterator arithmetic  
        */

        std::ostringstream oss;
        double shortMA  = std::accumulate(window.end()-shortWindow_, window.end(), 0.0)/shortWindow_;
        if (shortMA > longMA && (lastSignal_.count(q.symbol)==0 ||  lastSignal_[q.symbol]!=signalSide::Buy)){
            oss << "[MA Crossover] Signal on " + q.symbol << " lastPrice " << q.lastPrice <<
            " Create Limit order to buy on " << book.bestBidPrice << " size " << book.bidSize;
            Logger::getInstance().info(oss.str()); 
            lastSignal_[q.symbol] = signalSide::Buy;
            return true;
        }
        else if (shortMA < longMA &&  (lastSignal_.count(q.symbol)==0 ||  lastSignal_[q.symbol]!=signalSide::Sell)){
            oss<< "[MA Crossover] Signal on " << q.symbol << " lastPrice " << q.lastPrice <<
            " Create Limit order to sell on " << book.bestAskPrice << " size " << book.askSize;
            Logger::getInstance().info(oss.str());
            lastSignal_[q.symbol] = signalSide::Sell;
            return true;
        }
        return false;
} 

private:
    size_t shortWindow_;
    size_t longWindow_;
    std::unordered_map<std::string, std::deque<double>> priceHistory_; 
    std::unordered_map<std::string, signalSide> lastSignal_;

};



// 
class TradeProcessor {
public:
    static TradeProcessor& getInstance(std::shared_ptr<ThreadSafeQueue<MDataEvent>> queue) {
        std::call_once(initFlag_, [&]() {
            instance_.reset(new TradeProcessor(queue));
        });
        return *instance_;
    }

    void start();
    void stop();
    void join();
    std::string status() const; 
private:
    TradeProcessor(std::shared_ptr<ThreadSafeQueue<MDataEvent>> queue);

    void run();
    
    void updateMarket(const MarketDataUpdate& q); 
    void checkStrategy(const MarketDataUpdate& q, const QuoteBook& book);

    std::shared_ptr<ThreadSafeQueue<MDataEvent>> queue_;
    std::atomic<bool> running_{false};
    std::thread thread_;

    static std::once_flag initFlag_;
    static std::unique_ptr<TradeProcessor> instance_;
    
    std::unordered_map<std::string, QuoteBook> market_;  // symbol => best quote
    std::vector<std::unique_ptr<IStrategy>> strategies_; // plug-in strategy logic
};
std::once_flag TradeProcessor::initFlag_;
std::unique_ptr<TradeProcessor> TradeProcessor::instance_=nullptr;

TradeProcessor::TradeProcessor(std::shared_ptr<ThreadSafeQueue<MDataEvent>> queue)
    : queue_(queue) {
        strategies_.emplace_back(std::make_unique<NarrowSpreadStrategy>());
        strategies_.emplace_back(std::make_unique<PriceBreakoutStrategy>()); 
        strategies_.emplace_back(std::make_unique<MovingAvCStrategy>());
    }

void TradeProcessor::start() {
    running_ = true;
    cout << "TradeProcessor started\n";
    thread_ = std::thread(&TradeProcessor::run, this);
}

void TradeProcessor::stop() {
    running_ = false;
    join();
}

void TradeProcessor::join() {
    if (thread_.joinable()) {
        thread_.join();
    }
}

void TradeProcessor::checkStrategy(const MarketDataUpdate& q, const QuoteBook& book){
    for (const auto& strPtr : strategies_) {
        /*
        if (strPtr->onQuote(q, book)) 
            break;
        */
        strPtr->onQuote(q, book);
    }
}

void TradeProcessor::run() {
    while (running_ || !queue_->empty()) {
        if (auto ev=queue_->try_pop(); ev.has_value() ) {
            MDataEvent event=ev.value();
            std::visit([&](auto&& e) {
                using T = std::decay_t<decltype(e)>;
                if constexpr (std::is_same_v<T, MarketDataUpdate>) {
                    // Process the quote here
                    //std::cout << "[TradeProcessor] Got quote: " << e.quoteToStr() << "\n";
                    Logger::getInstance().info(std::string("TR_PROC ")+e.quoteToStr());
                    updateMarket(e);
                    checkStrategy(e, market_[e.symbol]);
                    // You could implement trade logic, signal generation, etc.
                } else if constexpr (std::is_same_v<T, StopSignal>) {
                    Logger::getInstance().info("[TradeProcessor] Received STOP signal");
                    running_ = false; // end loop
                }
            }, event);
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
    Logger::getInstance().info("TradeProcessor RUN exit");
}

void TradeProcessor::updateMarket(const MarketDataUpdate& q) {
    auto& book = market_[q.symbol];
    bool upd=false;

    // Keep best (highest) bid
    if (q.bidPrice > book.bestBidPrice) {
        book.bestBidPrice = q.bidPrice;
        book.bidSize  = q.bidSize;
        upd=true;
    }

    // Keep best (lowest) ask
    if (q.askPrice < book.bestAskPrice) {
        book.bestAskPrice = q.askPrice;
        book.askSize  = q.askSize;
        upd=true;
    }

    if (upd)
        book.timestamp = q.timestamp; // latest time
}


std::string TradeProcessor::status() const {
        return  running_.load(std::memory_order_acquire) ? "Running" : "Stopped";
}

class MarketData {
public:
static MarketData& getInstance(
    std::shared_ptr<ThreadSafeQueue<MDataEvent>> queue,
    std::vector<std::string> symbols) 
    {
        static MarketData instance(queue, std::move(symbols));
        return instance;
    }


    void start();        // starts thread that runs marketDataFeed
    void stop();         // sets internal flag and waits for thread to finish
    bool isRunning() const;
    std::string status() const; 
    void wait_till_done();
private:
//  Singleton
    MarketData(const MarketData&) = delete;
    MarketData& operator=(const MarketData&) = delete;
    MarketData(MarketData&&) = delete;
    MarketData& operator=(MarketData&&) = delete;
/////////////////////    
    MarketData(std::shared_ptr<ThreadSafeQueue<MDataEvent>> queue,
               std::vector<std::string> symbols);
               
    std::generator<MDataEvent> marketDataFeed(); // coroutine
    void run();          // internal thread logic
    void stopTradeProcessor();

    std::atomic<bool> running_{false};
    std::thread thread_;
    std::shared_ptr<ThreadSafeQueue<MDataEvent>> queue_;
    std::unique_ptr<Info> info_;
    std::atomic<bool> all_done_{false};
    std::mutex mutex;
    std::condition_variable cv_alldone;
};

MarketData::MarketData(std::shared_ptr<ThreadSafeQueue<MDataEvent>> queue,
               std::vector<std::string> symbols) 
               :queue_(queue)
{
    Info::init(std::move(symbols));               
}

void MarketData::start() {
    if (running_.exchange(true)) return;  // already running

    auto& trP=TradeProcessor::getInstance(queue_);
    trP.start();

    thread_ = std::thread([this]() {
        this->run();
    });
}

std::generator<MDataEvent> MarketData::marketDataFeed() {
    while (running_.load(std::memory_order_acquire)) {
        static int ii=0;  //debug limit
        if (ii>50)
            break;
        ii++;
        
        auto& info = Info::getInstance();
        double bidPrice_=info.getPrice() - 0.05;
        MarketDataUpdate mdu{
            .symbol = info.getSecurity(),
            .lastPrice = info.getPrice(),
            .bidPrice = bidPrice_,
            .bidSize = info.getQuantity(),
            .askPrice = bidPrice_ + 0.10,
            .askSize = info.getQuantity(),
            .timestamp = std::chrono::system_clock::now() - std::chrono::hours(4)
        };
        co_yield mdu;
        //std::this_thread::yield();
    }
    co_yield StopSignal{};  // yield the stop signal
    co_return;
}

bool MarketData::isRunning() const {
    return running_.load(std::memory_order_acquire);
}

std::string MarketData::status() const {
        return  running_.load(std::memory_order_acquire) ? "Running" : "Stopped";
}

void MarketData::stopTradeProcessor() {
    auto& trP=TradeProcessor::getInstance(queue_);
    trP.stop();
}

void MarketData::stop() {
    if (!running_.exchange(false)) return;  // already stopped
    stopTradeProcessor();
    if (thread_.joinable()) {
        thread_.join();
    }
}

void MarketData::wait_till_done() {
    std::unique_lock lock(mutex);
    cv_alldone.wait(lock, [this] { return all_done_.load(std::memory_order_acquire); });
    if (running_.load(std::memory_order_acquire)) { // still running
        stopTradeProcessor();
        if (thread_.joinable()) {
            thread_.join();
        }
    }
}

void MarketData::run() {
    for (auto&& event : MarketData::marketDataFeed()) {
        std::visit([this](auto&& e) {
            using T = std::decay_t<decltype(e)>;
            if constexpr (std::is_same_v<T, MarketDataUpdate>) {
                //std::cout << "QUOTE: " << e.symbol << " " << e.bidPrice << "/" << e.askPrice << "\n";
                //std::cout << e.quoteToStr() << "\n";
                Logger::getInstance().info(std::string("MD_FEED ")+e.quoteToStr());
                queue_->push(e);
                std::this_thread::yield();
            } else if constexpr (std::is_same_v<T, StopSignal>) {
                Logger::getInstance().info("[MarketData] Processing STOP signal");;
                queue_->push(e);
                all_done_.store(true, std::memory_order_release);
                cv_alldone.notify_one();
                return;
            }
        }, event);
    }
}

////////////////////////

int main()
{
    {
    CommitMap dataInfo;
    vector<QueryData> queryData;
    if (needInput) {
        readCin(dataInfo, queryData);
    } else {
        dataInfo = {
        {{200, 6}, {{"mergesort.cpp", "0cdde1"}, {"bogosort.cpp", "4213ff"}}},
        {{200, 8}, {{"quicksort.cpp", "839ad0"}, {"mergesort.cpp", "0cdde1"}, {"bubblesort.cpp", "248dd1"}}},
        {{300, 2}, {{"bubblesort.cpp", "248dd1"}, {"bogosort.cpp", "4213ff"}}},
        {{500, 0}, {{"array.h", "163111"}, {"sequence.h", "294def"}}},
        {{1000, 4}, {{"array.h", "163111"}, {"vector.h", "fcc2af"}}}
        };
        queryData = {
            {0, 10000, {"quicksort.cpp", "839ad0"}},
            {0, 500, {"vector.h", "fcc2af"}},
            {0, 100000, {"no_found.h", "empty_response"}},
            {100, 200, {"bogosort.cpp", "4213ff"}}
            };
    }

    //print
    cout << "Commit info\n";
    for (const auto& pair : dataInfo) {
        cout << "id " << pair.first.second << " timestamp " << pair.first.first;
        for (const auto& data : pair.second) {
            cout << " " << data.path << " " << data.id;
        }
        cout << "\n";
    }
    cout << "Query info\n";
    for (const auto& qu : queryData) {
        cout << "timestampStart " << qu.timestampStart << " timestampEnd " << qu.timestampEnd <<
        " path " << qu.data.path << " id " <<qu.data.id << endl;
        std::vector<unsigned long long> result = processQuery(dataInfo,qu);
        if (result.size()) {
            for (const auto& r : result) {
                cout << r << " ";
            } cout << "\n";
        } else {
            cout << "Nothing found\n";
        }
    }
    }
    {
        cout << "Log manager\n";
        testLoger();
    }
    {
        cout << "Coroutine MarketFeed => QuoteNormalizer => TradeMatcher\n";
        //Info  info(std::vector<std::string>({"MSFT", "AAPL", "SPY"}));
        /*
        Info::init({"MSFT", "AAPL", "SPY"});
        auto& info = Info::getInstance();
        for (int i : std::views::iota(0, 10)) {
            cout << info.getSecurity() << endl;
        }
        */
        MarketData& feed = MarketData::getInstance(std::make_shared<ThreadSafeQueue<MDataEvent>>(),
            std::vector<std::string>{"MSFT", "AAPL", "SPY"}
        );
        feed.start();
        feed.wait_till_done();

    }

    return 0;
}