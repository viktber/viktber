/******************************************************************************

Welcome to GDB Online.
GDB online is an online compiler and debugger tool for C, C++, Python, Java, PHP, Ruby, Perl,
C#, OCaml, VB, Swift, Pascal, Fortran, Haskell, Objective-C, Assembly, HTML, CSS, JS, SQLite, Prolog.
Code, Compile, Run and Debug online from anywhere in world.

*******************************************************************************/
#include <iostream>
#include <chrono>
#include <atomic>
#include <set>
#include <unordered_map>
#include <optional>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <string>
#include <atomic>
#include <thread>
#include <random>
#include <memory>


using namespace std;

/*
Classes
TradeProcessor
Single consumer thread that processes TradeEvents.

ExchangeAdapter
Multiple producer threads for different exchanges producing TradeEvents.
ExchangeAdapter System
Goals:
Support 5 ExchangeAdapters simulating different exchanges producing TradeEvents.
Support additional MarketDataAdapter(s) producing market data events.
Each adapter runs in its own thread producing relevant events.
All push their TradeEvent into a shared TradeEventQueue


TradeEventQueue
Thread-safe queue holding TradeEvent instances.






OrderBook
Manages order matching, book state, and generates trade confirmations.

TradeEvent (std::variant)
Encapsulates all trade-related event types.

Order
OrderIdGenerator
BuyOrderComparator, SellOrderComparator
OrderBook

TradeEvent (with all event structs)
ThreadSafeQueue<TradeEvent>
Multiple producers (ExchangeAdapters) can safely push concurrently.
Single consumer (TradeProcessor) pops events.
Use mutex + condition_variable or a lock-free queue (mutex-based simpler and reliable for now).
Support push() and try_pop() or wait_and_pop() operations.
Use std::queue<TradeEvent> internally.


ExchangeAdapter

MarketDataAdapter

TradeProcessor
TradeProcessor
Consumes TradeEvent from tradeQueue.
Pushes trades into OrderBook, logs, etc.
Runs in its own thread.

MarketDataProcessor
Consumes MDataEvent from marketDataQueue.
Updates pricing models, notifies UI, etc.
Also runs in its own thread.

TradeProcessorManager
Owns both processors.
Responsible for:
Creating them (passing queues).
Starting both threads.
Stopping both threads.




Supported Trade Event Types
NewOrder	Submission of a new order.
CancelOrder	Request to cancel an existing order.
ModifyOrder	Request to modify an existing order.
TradeConfirmation	Notification of a successful trade/match.
RejectedOrder	Notification that an order was rejected.
OrderBookUpdate	Updates to the order book state (optional).
MarketDataUpdate	Market data events (price ticks, quotes).
SessionStatus	Exchange session status updates (open/close).
Heartbeat	Health-check or keepalive messages

*/

class OrderIdGenerator {
public:
    OrderIdGenerator() : id_(1) {}

    int getNextId() {
        return id_.fetch_add(1, std::memory_order_relaxed);
    }

private:
    std::atomic<int> id_;
};

enum class OrderType {
    Buy,
    Sell
};

enum class OrderStatus {
    NEW,
    MODIFIED,
    CANCELLED
};

inline std::string statusToString(OrderStatus status) {
    switch (status) {
        case OrderStatus::NEW: return "NEW";
        case OrderStatus::MODIFIED: return "MODIFIED";
        case OrderStatus::CANCELLED: return "CANCELLED";
    }
    return "UNKNOWN";
}

inline std::string orderTypeToString(OrderType t) {
    return t == OrderType::Buy ? "Buy" : "Sell";
}

struct Order {
private:    
    int orderId;
    std::string symbol;
    double price;
    int quantity;
    OrderType type;
    std::chrono::system_clock::time_point timestamp;
    OrderStatus status;
public:
//Move constructor/assignment
Order(Order&&) noexcept = default;
Order& operator=(Order&&) noexcept = default;

//copy constructor/assignment
Order(const Order&) = default;
Order& operator=(const Order&) = default;

    bool operator==(const Order& other) const {
        return orderId == other.orderId;
    }
    Order(int newId, const std::string& theSym, double thePrice, int qty,
          OrderType theType)
          : orderId(newId), symbol(theSym), 
          price(thePrice), quantity(qty), type(theType),
          timestamp(std::chrono::system_clock::now() - std::chrono::hours(4)),  //NYC
          status(OrderStatus::NEW) {}
          
    int getOrderId() const { return orderId; } 
    OrderType getOrderType() const { return type; }
    double getPrice() const { return price; }
    void setPrice(double thePrice) { price = thePrice; }
    int getQuantity() const { return quantity; }
    void setQuantity(int theQty) { quantity =theQty; } 
    
    std::string getSymbol() const { return symbol; }
    void setTimestamp (const std::chrono::system_clock::time_point& timeSt) {
        timestamp=timeSt;
    }
    std::chrono::system_clock::time_point getTimestamp() const { return timestamp; }
    
    void setStatus(OrderStatus theStatus) { status = theStatus; }
    OrderStatus getStatus() const { return status; }
    friend struct BuyOrderComparator;
    friend struct SellOrderComparator;  
    
    void printOrder() const {
        auto timestamp_t = std::chrono::system_clock::to_time_t(timestamp);
        
        std::cout << "Order ID: " << orderId << " "
                  << "Symbol: " << symbol << " "
                  << "Type: " << (type==OrderType::Buy? "Buy ":"Sell")
                  << std::fixed << std::setprecision(2)
                  << "Price: " << price << " "
                  << "Quantity: " << quantity << " "
                  << statusToString(status) << " "
                  << std::put_time(std::localtime(&timestamp_t), "%Y-%m-%d %H:%M:%S")
                  << "\n";
    
    }
};

//higher price first, earlier time first
struct BuyOrderComparator {
    bool operator()(const Order& lhs, const Order& rhs) const {
        if (lhs.price != rhs.price)
            return lhs.price > rhs.price;
        return lhs.timestamp < rhs.timestamp; // earlier is better
    }
};

//lower price first, earlier time first
struct SellOrderComparator {
    bool operator()(const Order& lhs, const Order& rhs) const {
        if (lhs.price != rhs.price)
            return lhs.price < rhs.price;
        return lhs.timestamp < rhs.timestamp; // earlier is better
    }
};

using setIterator=std::set<Order>::iterator;
struct OrderEntry {
    OrderType side;
    setIterator iterator;
};

class OrderBook {
public:
    OrderBook()  = default;
    ~OrderBook() = default;

     using mapIterator=std::unordered_map<int, OrderEntry>::iterator;
    // Add a new order to the book
    void addOrder(const Order& order);

    // Cancel an existing order by orderId
    bool cancelOrder(int orderId);

    // Modify price and/or quantity of an existing order by orderId
    bool modifyOrder(int orderId, double newPrice, int newQuantity);

    // (Optional) Match orders logic placeholder
    void matchOrders();
    
    std::pair<bool, mapIterator> findOrder(int orderId);
    
    void printOrderBook();

private:
    std::set<Order, BuyOrderComparator> buyOrders_;
    std::set<Order, SellOrderComparator> sellOrders_;
    //std::vector<Order> closedOrders_;
    // Map orderId to iterator in either buyOrders_ or sellOrders_

    std::unordered_map<int, OrderEntry> orderIdMap_;
};

std::pair<bool, OrderBook::mapIterator> OrderBook::findOrder(int orderId) {
    auto it = orderIdMap_.find(orderId);
    return {it !=orderIdMap_.end(), it} ;
}

void orderUpdate(Order& updatedOrder, double newPrice, int newQuantity) {
    updatedOrder.setPrice(newPrice);
    updatedOrder.setQuantity(newQuantity);
    updatedOrder.setTimestamp(std::chrono::system_clock::now() - std::chrono::hours(4));
    updatedOrder.setStatus(OrderStatus::MODIFIED);
}

bool OrderBook::modifyOrder(int orderId, double newPrice, int newQuantity) {
    auto result = findOrder(orderId);
    if (!result.first) {
        return false;
    }
    auto mapIt = result.second;
    auto& orderEntry = mapIt->second;

    if (orderEntry.side == OrderType::Buy) {
        auto nodeHandle = buyOrders_.extract(orderEntry.iterator);
        orderUpdate(nodeHandle.value(), newPrice, newQuantity);
        auto newIt = buyOrders_.insert(std::move(nodeHandle)).position;
        orderEntry.iterator = newIt;
    } else {
        auto nodeHandle = sellOrders_.extract(orderEntry.iterator);
        orderUpdate(nodeHandle.value(), newPrice, newQuantity);
        auto newIt = sellOrders_.insert(std::move(nodeHandle)).position;
        orderEntry.iterator = newIt;
    }
    return true;
}

bool OrderBook::cancelOrder(int orderId) {
    auto result=findOrder(orderId);
    if (!result.first) {
        return false;
    }
   
    auto mapIt = result.second;
    auto setIt = mapIt->second.iterator;
    
    //Save Order by *setIt  
    Order cancelOrder = *setIt;
    cancelOrder.setTimestamp(std::chrono::system_clock::now() - std::chrono::hours(4));
    cancelOrder.setStatus(OrderStatus::CANCELLED);
    //closedOrders_.push_back(cancelOrder);
    if (mapIt->second.side==OrderType::Buy) {
        buyOrders_.erase(setIt);;
    } else {
        sellOrders_.erase(setIt);
    }
    orderIdMap_.erase(mapIt);
    return true;
}

void OrderBook::addOrder(const Order& order) {
    auto orderId=order.getOrderId();
    if (orderIdMap_.count(orderId) !=0 )  //// Reject duplicate order ID
        return;
    if (order.getOrderType()==OrderType::Buy) {
        auto it = buyOrders_.insert(order).first;
        orderIdMap_[orderId]={OrderType::Buy, it};
    } else {
        auto it = sellOrders_.insert(order).first;
        orderIdMap_[orderId]={OrderType::Sell, it};        
    }
    
}

void OrderBook::matchOrders() {
    cout << "OrderBook Received matchOrders command\n";
    while (!buyOrders_.empty() && !sellOrders_.empty()) {
        auto buyIt = buyOrders_.begin();     //higher price first
        auto sellIt = sellOrders_.begin();   //lower price first

        if (buyIt->getPrice() < sellIt->getPrice()) { 
            break;  // No price match possible        // No match possible: best buy price 
        }                                             // is less than best sell price
        // Buy Price == SellPrice
        int buyQty = buyIt->getQuantity();
        int sellQty = sellIt->getQuantity();
        int matchedQty = std::min(buyQty, sellQty);

        // Update Buy order
        Order updatedBuy = *buyIt;
        orderUpdate(updatedBuy, updatedBuy.getPrice(), buyQty - matchedQty);

        // Update Sell order
        Order updatedSell = *sellIt;
        orderUpdate(updatedSell, updatedSell.getPrice(),sellQty - matchedQty);

        // Erase old orders and map entries
        int buyOrderId = buyIt->getOrderId();
        int sellOrderId = sellIt->getOrderId();

        buyOrders_.erase(buyIt);
        sellOrders_.erase(sellIt);
        orderIdMap_.erase(buyOrderId);
        orderIdMap_.erase(sellOrderId);

        // Re-insert partially filled orders if any
        if (updatedBuy.getQuantity() > 0) {
            auto it = buyOrders_.insert(updatedBuy).first;
            orderIdMap_[buyOrderId] = {OrderType::Buy, it};
        }
        if (updatedSell.getQuantity() > 0) {
            auto it = sellOrders_.insert(updatedSell).first;
            orderIdMap_[sellOrderId] = {OrderType::Sell, it};
        }
    }
}


void OrderBook::printOrderBook() {
    auto printOrd = [](const auto& container, const string& lable ) {
        if (container.size()) {
            cout << lable << " [" << container.size() << "]\n";
            for (const auto& el : container) {
                el.printOrder();
            }
        }
    };
    printOrd(buyOrders_, "Buy Orders");
    printOrd(sellOrders_,"Sell Orders");
//    printOrd(closedOrders_, "Closed Orders");
}

//TradeEvent type
using Timestamp = std::chrono::system_clock::time_point;

// New order submission
struct NewOrder {
    int orderId;
    std::string symbol;
    OrderType orderType;     // Buy or Sell
    int quantity;
    double price;
    Timestamp timestamp;
};

// Cancel order request
struct CancelOrder {
    int orderId;
    Timestamp timestamp;
};

// Modify order request
struct ModifyOrder {
    int orderId;
    int newQuantity;
    double newPrice;
    Timestamp timestamp;
};

// Trade confirmation (successful match)
struct TradeConfirmation {
    int tradeId;
    int buyOrderId;
    int sellOrderId;
    int quantity;
    double price;
    Timestamp timestamp;
};

// Rejected order notification
struct RejectedOrder {
    int orderId;
    std::string reason;      // e.g., "Insufficient funds"
    Timestamp timestamp;
};

// Optional: Order book update
struct OrderBookUpdate {
    std::string symbol;
    // You can add bids/asks snapshot here if needed
    Timestamp timestamp;
};

// Market data update (price ticks, quotes)
struct MarketDataUpdate {
    std::string symbol;
    double lastPrice;
    double bidPrice;
    int bidSize;
    double askPrice;
    int askSize;
    Timestamp timestamp;
};

// Session status (exchange open/close/halt)
enum class SessionState { Open, Close, Halt };

struct SessionStatus {
    SessionState state;
    std::string exchangeName;
    Timestamp timestamp;
};

// Heartbeat (keep-alive)
struct Heartbeat {
    std::string source;   // e.g., exchange name or adapter id
    Timestamp timestamp;
};

struct StopSignal {};

using TradeEvent = std::variant<
    NewOrder,
    CancelOrder,
    ModifyOrder,
    TradeConfirmation,
    RejectedOrder,
    OrderBookUpdate,
    SessionStatus,
    Heartbeat,
    StopSignal 
>;

using MDataEvent = std::variant<
   MarketDataUpdate,
   StopSignal
>;
template <typename T>
class ThreadSafeQueue {
public:
    ThreadSafeQueue() = default;

    // Disable copy & assignment
    ThreadSafeQueue(const ThreadSafeQueue&) = delete;
    ThreadSafeQueue& operator=(const ThreadSafeQueue&) = delete;

    // Push an element into the queue (thread-safe)
    void push(T item) {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(std::move(item));
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

/////////////////////////////
enum class AdapterState {
    Idle,
    Running,
    Stopped
};

/*
Thread is created once in constructor.
Wakeup using condition_variable.
Proper atomic load/store memory order guarantees.
Cleaner restart logic — same as start().

*/

enum class AdapterType {
    Exchange,
    MarketData
};


class IAdapter {
public:
    virtual ~IAdapter() = default;
    virtual void start(int n) = 0;
    virtual void stop() = 0;
    virtual void restart(int n) = 0;
    virtual bool isStopped() const = 0;
    virtual std::string status() const = 0;
    virtual AdapterType getType() const = 0;
    virtual void putOneMsg(const TradeEvent&) {}     
    virtual void putOneMsg(const MDataEvent&) {}
};

class ExchangeAdapter : public IAdapter {
public:
    ExchangeAdapter(const std::string& name, ThreadSafeQueue<TradeEvent>& queue)
        : name_(name), queue_(queue), eventCount_(0),
          stopRequested_(false), state_(AdapterState::Idle)
    {
        worker_ = std::thread(&ExchangeAdapter::produceEvents, this);
    }

    ~ExchangeAdapter() {
        stop();
        {
            std::lock_guard<std::mutex> lock(mtx_);
            terminateRequested_.store(true, std::memory_order_release);
        }
        cv_.notify_all();
        if (worker_.joinable()) {
            worker_.join();
        }
    }

    AdapterType getType() const override {
        return AdapterType::Exchange;
    }

    void start(int n) override {
        {
            std::lock_guard<std::mutex> lock(mtx_);
            if (state_ == AdapterState::Running) return;
            eventCount_.store(n, std::memory_order_release);
            stopRequested_.store(false, std::memory_order_release);
            state_.store(AdapterState::Running, std::memory_order_release);
            std::cout << "[" << name_ << "] Started N tasks " << n << "\n";
        }
        cv_.notify_one();
    }
    
    void putOneMsg(const TradeEvent& ev) override {
         std::lock_guard<std::mutex> lock(mtx_);
         queue_.push(ev);
    }

    void stop() override {
        {
            std::lock_guard<std::mutex> lock(mtx_);
            stopRequested_.store(true, std::memory_order_release);
        }
        cv_.notify_one();
    }

    void restart(int n) override {
        start(n); // same as start now — triggers the waiting loop
    }

    bool isStopped() const override {
        return state_.load(std::memory_order_acquire) == AdapterState::Idle;
    }

    std::string status() const override {
        auto s = state_.load(std::memory_order_acquire);
        switch (s) {
            case AdapterState::Idle: return name_ + " [Idle]";
            case AdapterState::Running: return name_ + " [Running]";
            case AdapterState::Stopped: return name_ + " [Stopped]";
        }
        return name_ + " [Unknown]";
    }

private:
    void produceEvents() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> qtyDist(1, 1000);
        std::uniform_real_distribution<> priceDist(100.0, 200.0);
        std::uniform_int_distribution<> orderTypeDist(0, 1);

        while (true) {
            std::unique_lock<std::mutex> lock(mtx_);
            
            cv_.wait(lock, [&] {
                return terminateRequested_.load(std::memory_order_acquire) ||
                   (eventCount_.load(std::memory_order_acquire) > 0 &&
                    !stopRequested_.load(std::memory_order_acquire));
            });

            if (terminateRequested_.load(std::memory_order_acquire)) {
                break; // Permanent shutdown
            }

            state_.store(AdapterState::Running, std::memory_order_release);

            while (true) {
                if (stopRequested_.load(std::memory_order_acquire)) {
                    break;
                }

                int remaining = eventCount_.fetch_sub(1, std::memory_order_acq_rel);
                if (remaining <= 0) {
                    break;
                }

                NewOrder order{
                    generateOrderId(),
                    "AAPL",
                    static_cast<OrderType>(orderTypeDist(gen)),
                    qtyDist(gen),
                    priceDist(gen),
                    std::chrono::system_clock::now()
                };

                queue_.push(order);
                std::cout << "[" << name_ << "] Produced order " << order.orderId <<
                " Remaining tasks " << remaining 
                << "\n";

                lock.unlock(); // unlock before sleep to avoid blocking others
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                lock.lock();   // re-lock
            }
            cout << "[" << name_ << "] " << "Done with tasks\n";
            if (stopRequested_.load(std::memory_order_acquire)) {
                state_.store(AdapterState::Stopped, std::memory_order_release);
            } else {
                state_.store(AdapterState::Idle, std::memory_order_release);
            }
        }
    }

    int generateOrderId() {
        return orderIdGen_.getNextId();
    }

private:
    std::string name_;  //Echange name
    ThreadSafeQueue<TradeEvent>& queue_;
    std::atomic<int> eventCount_;
    std::atomic<bool> stopRequested_;
    std::atomic<AdapterState> state_;
    std::atomic<bool> terminateRequested_{false};
    std::thread worker_;
    OrderIdGenerator orderIdGen_;

    std::mutex mtx_;
    std::condition_variable cv_;
};

class MarketDataAdapter : public IAdapter {
public:
    MarketDataAdapter(const std::string& name, ThreadSafeQueue<MDataEvent>& queue)
        : name_(name), queue_(queue), eventCount_(0),
          stopRequested_(false), state_(AdapterState::Idle) {
        worker_ = std::thread(&MarketDataAdapter::produceEvents, this);
    }

    ~MarketDataAdapter() {
        stop();
        {
            std::lock_guard<std::mutex> lock(mtx_);
            terminateRequested_.store(true, std::memory_order_release);
        }
        cv_.notify_all();
        if (worker_.joinable()) {
            worker_.join();
        }
    }

    AdapterType getType() const override {
        return AdapterType::MarketData;
    }


    void start(int n) override {
        {
            std::lock_guard<std::mutex> lock(mtx_);
            if (state_ == AdapterState::Running) return;
            eventCount_.store(n, std::memory_order_release);
            stopRequested_.store(false, std::memory_order_release);
            state_.store(AdapterState::Running, std::memory_order_release);
            std::cout << "[" << name_ << "] Started N market data tasks " << n << "\n";
        }
        cv_.notify_one();
    }

    void stop() override {
        {
            std::lock_guard<std::mutex> lock(mtx_);
            stopRequested_.store(true, std::memory_order_release);
        }
        cv_.notify_one();
    }

    void restart(int n) override {
        start(n);
    }

    bool isStopped() const override {
        return state_.load(std::memory_order_acquire) == AdapterState::Idle;
    }

    std::string status() const override {
        auto s = state_.load(std::memory_order_acquire);
        switch (s) {
            case AdapterState::Idle: return name_ + " [Idle]";
            case AdapterState::Running: return name_ + " [Running]";
            case AdapterState::Stopped: return name_ + " [Stopped]";
        }
        return name_ + " [Unknown]";
    }
    
    void putOneMsg(const MDataEvent& ev) override {
         std::lock_guard<std::mutex> lock(mtx_);
         queue_.push(ev);
    }
private:
    void produceEvents() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> qtyDist(100, 1000);
        std::uniform_real_distribution<> priceDist(100.0, 200.0);

        while (true) {
            std::unique_lock<std::mutex> lock(mtx_);
            
            cv_.wait(lock, [&] {
                return terminateRequested_.load(std::memory_order_acquire) ||
                   (eventCount_.load(std::memory_order_acquire) > 0 &&
                    !stopRequested_.load(std::memory_order_acquire));
            });

            if (terminateRequested_.load(std::memory_order_acquire)) {
                break;
            }

            state_.store(AdapterState::Running, std::memory_order_release);

            while (true) {
                if (stopRequested_.load(std::memory_order_acquire)) {
                    break;
                }

                int remaining = eventCount_.fetch_sub(1, std::memory_order_acq_rel);
                if (remaining <= 0) {
                    break;
                }

                double lastPr = priceDist(gen);
                MarketDataUpdate mData{
                    "AAPL",
                    lastPr,
                    lastPr + 10.0,
                    qtyDist(gen),
                    lastPr + 50.0,
                    qtyDist(gen),
                    std::chrono::system_clock::now()
                };

                queue_.push(mData);
                std::cout << "[" << name_ << "] MarketData: " << lastPr << " Rem: " << remaining << "\n";

                lock.unlock();
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                lock.lock();
            }

            std::cout << "[" << name_ << "] DONE with task\n";

            if (stopRequested_.load(std::memory_order_acquire)) {
                state_.store(AdapterState::Stopped, std::memory_order_release);
            } else {
                state_.store(AdapterState::Idle, std::memory_order_release);
            }
        }
    }

private:
    std::string name_;
    ThreadSafeQueue<MDataEvent>& queue_;
    std::atomic<int> eventCount_;
    std::atomic<bool> stopRequested_;
    std::atomic<AdapterState> state_;
    std::atomic<bool> terminateRequested_{false};
    std::thread worker_;

    std::mutex mtx_;
    std::condition_variable cv_;
};


class AdapterFactory {
public:
    static std::unique_ptr<IAdapter> createAdapter(
        const std::string& name,
        ThreadSafeQueue<TradeEvent>& queue
    ) {
        return std::make_unique<ExchangeAdapter>(name, queue);
    }

    static std::unique_ptr<IAdapter> createAdapter(
        const std::string& name,
        ThreadSafeQueue<MDataEvent>& queue
    ) {
        return std::make_unique<MarketDataAdapter>(name, queue);
    }
};


class AdapterManager {
public:
    bool registerAdapter(const std::string& name, std::unique_ptr<IAdapter> adapter) {
        if (adapter) {
            adapters_[name] = std::move(adapter);
            return true;
        }
        return false;
    }

    void startAdapter(const std::string& name, int n) {
        if (auto it = adapters_.find(name); it != adapters_.end()) {
            it->second->start(n);
        }
    }

    void restartAdapter(const std::string& name, int n) {
        if (auto it = adapters_.find(name); it != adapters_.end()) {
            it->second->restart(n);
        }
    }

    void stopAdapter(const std::string& name) {
        if (auto it = adapters_.find(name); it != adapters_.end()) {
            it->second->stop();
        }
    }


    void startAll(int n) {
        for (auto& [_,adapter] : adapters_) {
            adapter->start(n);
        }
    }

    void restartAll(int n) {
        for (auto& [_, adapter] : adapters_) {
            adapter->restart(n);
        }
    }

    void stopAll() {
        for (auto& [_, adapter] : adapters_) {
            adapter->stop();
        }
    }
 
    void doneForDay() {
        for (auto& [_, adapter] : adapters_) {
            StopSignal stopEvent;

            if (adapter->getType() == AdapterType::Exchange) {
                adapter->putOneMsg(static_cast<const TradeEvent&>(stopEvent));
            } else {
                adapter->putOneMsg(static_cast<const MDataEvent&>(stopEvent));
            }
            adapter->stop();
        }
    }
    

    bool allIdleOrStopped() const {
        for (const auto& [_, adapter] : adapters_) {
            if (!adapter->isStopped()) {
                return false;
            }
        }
        return true;
    }

    std::string statusReport() const {
        std::ostringstream oss;
        for (const auto& [_, adapter] : adapters_) {
            oss << adapter->status() << "\n";
        }
        return oss.str();
    }

private:
    std::unordered_map<std::string, std::unique_ptr<IAdapter>> adapters_;
};

void testBook() {
    OrderBook ob;
    Order od(1, "MSFT", 10.0, 10,
          OrderType::Buy);
    ob.addOrder(od);
    ob.printOrderBook();
    ob.modifyOrder(od.getOrderId(), 200.0, 100);
    ob.printOrderBook();    
    ob.cancelOrder(od.getOrderId());
    ob.printOrderBook();
    Order od2(2, "MSFT", 10.0, 100,
          OrderType::Buy);
    Order od3(3, "MSFT", 10.0, 30,
          OrderType::Sell);
    Order od4(4, "MSFT", 10.0, 60,
          OrderType::Sell);
    ob.addOrder(od2);
    ob.addOrder(od3);
    ob.addOrder(od4);    
    ob.printOrderBook();  
    ob.matchOrders();
    ob.printOrderBook();      
}

class TradeProcessor {
public:
    TradeProcessor(ThreadSafeQueue<TradeEvent>& queue)
        : queue_(queue), stopRequested_(false),
        orderBook_(nullptr)
        {}

    void start() {
        stopRequested_ = false;
        worker_ = std::thread(&TradeProcessor::run, this);
    }

    void stop() {
        stopRequested_ = true;
        if (worker_.joinable()) {
            worker_.join();
        }
    }

    std::string status() const {
        return stopRequested_ ? "Stopped" : "Running";
    }
    void setOrderBook(OrderBook* obPtr) {
        orderBook_=obPtr;
    }
    OrderBook* getOrderBook() { return orderBook_; }
private:
    void run() {
        while (!stopRequested_) {
            TradeEvent event = queue_.wait_and_pop();

            // Simulate processing
            std::visit([this](auto&& event) {
                using T = std::decay_t<decltype(event)>;
                if constexpr (std::is_same_v<T, NewOrder>) {
                std::cout << "[TradeProcessor] Processing NewOrder: " << event.orderId << "\n";
                if (orderBook_) {

                    Order ord(event.orderId, event.symbol, event.price, event.quantity,event.orderType);
                    orderBook_->addOrder(ord);
                }
                
                } else if constexpr (std::is_same_v<T, CancelOrder>) {
                    std::cout << "Processing CancelOrder: " << event.orderId << "\n";
                } else if constexpr (std::is_same_v<T, Heartbeat>) {
                    std::cout << "Received Heartbeat\n";
                } else if constexpr (std::is_same_v<T, StopSignal>) {
                    std::cout << "[TradeProcessor] Received StopSignal. Exiting.\n";
                    stopRequested_ = true;
                }
            }, event);
        }
    }

    ThreadSafeQueue<TradeEvent>& queue_;
    std::atomic<bool> stopRequested_;
    std::thread worker_;
    OrderBook* orderBook_;
};

class MarketDataProcessor {
public:
    MarketDataProcessor(ThreadSafeQueue<MDataEvent>& queue)
        : queue_(queue), stopRequested_(false) {}

    void start() {
        stopRequested_ = false;
        worker_ = std::thread(&MarketDataProcessor::run, this);
    }

    void stop() {
        stopRequested_ = true;
        if (worker_.joinable()) {
            worker_.join();
        }
    }

    std::string status() const {
        return stopRequested_ ? "Stopped" : "Running";
    }

private:
    void run() {
        while (!stopRequested_) {
            MDataEvent event = queue_.wait_and_pop();
                        // Simulate processing
            std::visit([this](auto&& event) {
                using T = std::decay_t<decltype(event)>;
                if constexpr (std::is_same_v<T, MarketDataUpdate>) {
                    std::cout << "[MarketDataProcessor] Processed MarketData: "
                    << event.lastPrice << "\n";
                } else if constexpr (std::is_same_v<T, StopSignal>) {
                    std::cout << "[MarketDataProcessor] Received StopSignal. Exiting.\n";
                    stopRequested_ = true;
                }    
            }, event);

        }
    }

    ThreadSafeQueue<MDataEvent>& queue_;
    std::atomic<bool> stopRequested_;
    std::thread worker_;
};

class TradeProcessorManager {
public:
    TradeProcessorManager(ThreadSafeQueue<TradeEvent>& tradeQueue,
                          ThreadSafeQueue<MDataEvent>& mdQueue)
        : tradeProcessor_(tradeQueue), mdProcessor_(mdQueue) {}

    void start() {
        tradeProcessor_.start();
        mdProcessor_.start();
    }

    void stop() {
        tradeProcessor_.stop();
        mdProcessor_.stop();
    }
    void setOrderBook(OrderBook* obPtr) {
        tradeProcessor_.setOrderBook(obPtr);
    }
private:
    TradeProcessor tradeProcessor_;
    MarketDataProcessor mdProcessor_;
};


void whileRunning(AdapterManager& manager) {
    // Loop while not all adapters are Idle or Stopped
    while (!manager.allIdleOrStopped()) {
        std::cout << manager.statusReport() << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void testAdapterManager() {
    ThreadSafeQueue<TradeEvent> tradeQueue;
    ThreadSafeQueue<MDataEvent> mdQueue;
    
    TradeProcessorManager tradeProcessor(tradeQueue,mdQueue);
    OrderBook ob;
    tradeProcessor.setOrderBook(&ob);
    
    tradeProcessor.start();
    
    AdapterManager manager;
    string theName="NYSE";
    
    manager.registerAdapter(theName, AdapterFactory::createAdapter(
            theName, tradeQueue));
    manager.startAdapter("NYSE",10);
    
    whileRunning(manager);

    manager.registerAdapter("NASDAQ_MD", AdapterFactory::createAdapter(
            "NASDAQ_MD", mdQueue));
    manager.startAll(7);
    whileRunning(manager);
    cout << "queue size() tradeQueue " << tradeQueue.getSize() << 
    " mdQueue " << mdQueue.getSize() << "\n";
    manager.doneForDay();
    manager.statusReport();    
    ob.printOrderBook();
    ob.matchOrders();
    ob.printOrderBook();
    tradeProcessor.stop();

}

int main()
{
    std::cout<<"Hello World\n";
    //testBook();
    testAdapterManager();
    return 0;
}