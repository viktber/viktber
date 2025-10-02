/******************************************************************************

Welcome to GDB Online.
GDB online is an online compiler and debugger tool for C, C++, Python, Java, PHP, Ruby, Perl,
C#, OCaml, VB, Swift, Pascal, Fortran, Haskell, Objective-C, Assembly, HTML, CSS, JS, SQLite, Prolog.
Code, Compile, Run and Debug online from anywhere in world.

*******************************************************************************/
#include <iostream>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>


using namespace std;

enum class Side : uint8_t { 
    Buy = 0, 
    Sell = 1 
};

using OrderId = uint64_t;
using Price   = int64_t;   // in ticks Price 5000.00 tick = 0.25 
                           // price in integer ticks = price / tick =5000/0.25 20000
using Qty     = uint64_t;

using Index   = int64_t;

struct Order {
    OrderId id;
    Side side;
    Price price;
    Qty qty;
    std::string symbol;

    // Intrusive links for FIFO
    Order* next{nullptr};
    Order* prev{nullptr};

    bool is_active() const { return qty > 0; }
    void printOrder() const {
        std::cout << "Order ID: " << id << " "
        << "Side: " << (side==Side::Buy? "Buy":"Sell") 
        << " Price: " << price << " "
        << "Quantity: " << qty << " "
        << "\n";
    }     
};

// head --> Order1 <--> Order2 <--> OrderN <-- tail
class PriceLevel {  //List of Orders
public:
    PriceLevel() = default;
    explicit PriceLevel(Price p) : price_(p) {}

    // Add order to the back (newest) add to tail, new tail
    void push_back(Order* o) {
        o->next = nullptr;
        o->prev = tail_;
        if (!tail_) {
            //empty list``
            head_ = o;
        }    
        else {
            tail_->next = o;
        }    
        tail_ = o;  // new tail
        ++size_;
    }

    void setPrice(Price p) { price_ = p; }
    Price getPrice() const { return price_; }
    // Remove order from the front (oldest)  head remove
    /*
    void pop_frontP() {
        if (!head_) return;
        Order* old = head_;
        head_ = head_->next;
        if (head_) 
            head_->prev = nullptr;
        else 
            tail_ = nullptr; //empty List
        old->next = old->prev = nullptr;
        --size_;
    }
    */
    // Remove an arbitrary order (for cancel)
    void remove(Order* o) {
        if (o->prev) o->prev->next = o->next;
        else head_ = o->next;
        if (o->next) o->next->prev = o->prev;
        else tail_ = o->prev;
        o->next = o->prev = nullptr;
        --size_;
    }

    bool empty() const { return size_ == 0; }
    size_t size() const { return size_; }
    Price price() const { return price_; }
    Order* head() const { return head_; }
    Order* tail() const { return tail_; }

private:
    Price price_{0};
    Order* head_{nullptr};
    Order* tail_{nullptr};
    size_t size_{0};
};

class OrderPool { //with Arbitrary reclaim by Order id
                  //Allocate use free list if not empty, else head
                  //Reclaim push slot to free list
public:
    explicit OrderPool(size_t capacity)
        : capacity_(capacity), pool_(capacity), head_(0), count_(0)
    {
        free_list_.reserve(capacity);
    }

    // Allocate a new order
    Order* allocate() {
        Order* o = nullptr;
        if (!free_list_.empty()) {
            // reuse a freed slot (LIFO)
            size_t idx = free_list_.back();
            free_list_.pop_back();
            o = &pool_[idx];
        } else if (head_ < capacity_) {
            // use a fresh slot
            o = &pool_[head_++];
        } else {
            return nullptr; // pool full
        }
        ++count_;
        // Reset fields
        o->id = 0;
        o->qty = 0;
        o->price = 0;
        o->symbol.clear();
        o->next = o->prev = nullptr;
        return o;
    }

    // Reclaim an order (arbitrary order supported)
    void reclaim(Order* o) {
        // compute index and push to free list
        size_t idx = static_cast<size_t>(o - &pool_[0]);
        // ensure idx is in range
        if (idx >= capacity_) return;
        free_list_.push_back(idx);
        if (count_ > 0) --count_;
        // Reset links
        o->next = o->prev = nullptr;
    }

    bool empty() const { return count_ == 0; }
    bool full()  const { return count_ == capacity_; }
    size_t size() const { return count_; }

private:
    size_t capacity_;
    std::vector<Order> pool_;
    size_t head_;                   // next unused fresh slot
    size_t count_;                  // active orders
    std::vector<size_t> free_list_; // indices of freed slots (LIFO)
};



/*
class OrderPool { //this is only FIFO pool, doesnt support reclame by Order id
                  //Allocate use head, Reclaim use tail, count_++ / count_--
public:
    explicit OrderPool(size_t capacity)
        : capacity_(capacity), pool_(capacity), head_(0), tail_(0), count_(0) {}

    // Allocate a new order
    Order* allocate() {
        if (count_ == capacity_) return nullptr; // pool full
        Order* o = &pool_[head_];
        head_ = (head_ + 1) % capacity_;
        ++count_;
        // Optional: reset fields
        o->id = 0;
        o->qty = 0;
        o->next = o->prev = nullptr;
        return o;
    }

    // Reclaim an order
    void reclaim(Order* o) {
        if (count_ == 0) return;
        // reset links to be safe
        o->next = o->prev = nullptr;
        tail_ = (tail_ + 1) % capacity_;
        --count_;
    }

    bool empty() const { return count_ == 0; }
    bool full() const { return count_ == capacity_; }

private:
    size_t capacity_;
    std::vector<Order> pool_;
    size_t head_;   // next slot to allocate
    size_t tail_;   // next slot to reclaim
    size_t count_;  // number of allocated orders
};
*/

/*
Represents one side of the order book (Buy or Sell) for a single symbol.
Holds all price levels in a fixed-size array for O(1) price lookup.
Preserves time priority within each price level using PriceLevel’s FIFO queue.
*/

class FixedBookSide {
public:
    FixedBookSide(Side side, Price min_price, Price max_price, Price tick_size)
        : side_(side),
          min_price_(min_price),
          max_price_(max_price),
          tick_size_(tick_size)
    {
        size_ = (max_price_ - min_price_) / tick_size_ + 1;
        levels_.resize(size_);   // Preallocate all price levels
        for (size_t i = 0; i < size_; ++i) {
            Price px = min_price_ + i * tick_size_;
            levels_[i].setPrice(px);
        }
    }

    // Insert order
    void insert_order(Order* o) {
        Index idx = price_to_index(o->price);
        auto& lvl = levels_[idx];
        lvl.push_back(o);
        orders_by_id_[o->id] = {o, idx};
    }

    // Cancel order
    Order* cancel_order(OrderId id) {
        auto it = orders_by_id_.find(id);
        if (it == orders_by_id_.end()) 
            return nullptr;
        auto [o, idx] = it->second;
        auto& lvl = levels_[idx];
        lvl.remove(o);
        orders_by_id_.erase(it);
        return o;
    }

    // Best price (depends on side)
    Price best_price() const {
        if (side_ == Side::Buy) {
            for (size_t i = size_; i-- > 0;) {
                if (!levels_[i].empty()) 
                    return levels_[i].getPrice();
            }
        } else { // Sell
            for (size_t i = 0; i < size_; ++i) {
                if (!levels_[i].empty()) 
                    return levels_[i].getPrice();
            }
        }
        return 0; // no liquidity
    }
    bool good_price(Price p) const {
        return (p>=min_price_ && p<=max_price_);
    }
    Index price_to_index(Price p) const {
        //if (!good_price(p)) return -1;
        return static_cast<Index>((p - min_price_) / tick_size_);
    }
    PriceLevel& getPriceLevel(size_t index) {
        return levels_[index];
    }
    const PriceLevel& getPriceLevel(size_t index) const {
        return levels_[index];
    }    
    size_t getSize() const{ return size_; }
    Side getSide() const { return side_; }
    
    Order* getOrder(OrderId id) {
        if ( auto it=orders_by_id_.find(id); it !=orders_by_id_.end() ) {
            return it->second.first;
        }
        return nullptr;
    }
    // Remove order and erase from orders_by_id_ (used when order fully filled)
    void remove_order (Order* o) {
        if (!o) return;
        if (auto it = orders_by_id_.find(o->id); it != orders_by_id_.end()) {
            size_t idx = it->second.second;
            // remove from level and erase from map
            levels_[idx].remove(o);
            orders_by_id_.erase(it);
        }    
    }
private:
    Side side_;
    Price min_price_, max_price_, tick_size_;
    size_t size_;
    std::vector<PriceLevel> levels_;  // no pointers, preallocated
    std::unordered_map<OrderId, std::pair<Order*, size_t>> orders_by_id_; //hash orderId pair Order*, idx for PriceLevel
};

enum class EventType : uint8_t {
    Accepted,   // order entered the book
    Fill,       // trade happened
    Cancel,     // order canceled
    Reject      // invalid order
};
inline std::string typeToString(EventType type) {
    using enum EventType;
    switch (type) {
        case Accepted: return "Accepted";
        case Fill: return "Fill";
        case Cancel: return "Cancel";
        case Reject: return "Reject";        
    }
    return "UNKNOWN";
}
struct OrderBookEvent {
    EventType type;
    OrderId orderId;   // affected order
    Qty qty{0};        // qty executed (only for Fill)
    Price price{0};    // price executed (only for Fill)
    OrderBookEvent(EventType t, OrderId id, Qty q=0, Price p=0)
        : type(t), orderId(id), qty(q), price(p) {}
    void printEvent() const {
        cout << typeToString(type) << " order Id " << orderId << " qty " << qty << " Price " << price << "\n";
    }
};

struct MessageHub { // OrderBookEvent Dispatcher event Observer
std::unordered_map<EventType, std::vector<std::function<void(const OrderBookEvent&)>>> subscribers;
MessageHub() {
    defaultSubscripion();
}
    void subscribe(EventType type, std::function<void(const OrderBookEvent&)> callback) {
        subscribers[type].push_back(callback);
    }

    void notify(const OrderBookEvent& event) const {
        auto it = subscribers.find(event.type);
        if (it != subscribers.end()) {
            for (const auto& cb : it->second) {
                cb(event);
            }
        }
    }
    void defaultSubscripion() {
        auto logger = [](const OrderBookEvent& e) {
            cout <<"MessageHub: ";
            e.printEvent();
        };
        auto sendMsg = [](const OrderBookEvent& e) {
            cout <<"MessageHub: Send message " << typeToString(e.type) << "\n";
        };
        subscribe(EventType::Accepted,logger);
        subscribe(EventType::Fill, logger);
        subscribe(EventType::Cancel, logger);
        subscribe(EventType::Reject, logger);
        subscribe(EventType::Accepted,sendMsg);
        subscribe(EventType::Fill, sendMsg);
        subscribe(EventType::Cancel, sendMsg);
        subscribe(EventType::Reject, sendMsg);
    }
    
};    


class MatchingEngine {
public:
    MatchingEngine(Price min_price, Price max_price, Price tick_size)
        : bids_(Side::Buy, min_price, max_price, tick_size),
          asks_(Side::Sell, min_price, max_price, tick_size),
          order_pool_({pool_capacity})
          {}
 
        OrderPool* getOrderPool_ptr() { return &order_pool_; }
        const OrderPool* getOrderPool_ptr() const { return &order_pool_; }
    
    void submit_new_order(Side n_side, Price n_price, Qty n_qty) {
        Order* newOrder = order_pool_.allocate();
        if (!newOrder) {
            // pool full -> reject order
            std::vector<OrderBookEvent> ev;
            ev.push_back({EventType::Reject, 0, n_qty, n_price});
            dispatch_book_event(ev);
            return;
        }
        newOrder->id=start_id++;
        newOrder->side=n_side;
        newOrder->price=n_price;
        newOrder->qty=n_qty;
        add_order(newOrder);
    }
    // Insert new order
    void add_order(Order* o) {
        if (o->side == Side::Buy) {
            //bids_.insert_order(o);
            dispatch_book_event(try_match(o, bids_, asks_));
        } else {
            //asks_.insert_order(o);
            dispatch_book_event(try_match(o, asks_, bids_));
        }
    }

    // Cancel order
    
    bool cancel_order(OrderId id) {
        if (Order* order=asks_.getOrder(id)) return cancel_order(id, Side::Sell);
        if (Order* order=bids_.getOrder(id)) return cancel_order(id, Side::Buy);
        return false;
    }
    
    bool cancel_order(OrderId id, Side side) {
        Order* clx_order;
        if (side == Side::Buy)
            clx_order=bids_.cancel_order(id);
        else
            clx_order=asks_.cancel_order(id);
        if (clx_order) {
            //clx_order->printOrder();
            std::vector<OrderBookEvent> events;
            events.emplace_back(EventType::Cancel, clx_order->id, 
            clx_order->qty, clx_order->price);
            dispatch_book_event(events);
            order_pool_.reclaim(clx_order);
            return true;
        }
        return false;
    }

    // Query best prices
    Price best_bid() const { return bids_.best_price(); }
    Price best_ask() const { return asks_.best_price(); }
    
void printBook() const {
    std::cout << "=== ORDER BOOK ===\n";
    // Asks (lowest to highest)
    std::cout << "ASKS:\n";
    for (size_t i = 0; i < asks_.getSize(); ++i) {
        const PriceLevel& lvl = asks_.getPriceLevel(i);
        if (!lvl.empty()) {
            std::cout << " Price " << lvl.getPrice() << " | ";
            Order* cur = lvl.head();
            while (cur) {
                std::cout << "[Id " << cur->id << " Qty " << cur->qty << "] ";
                cur = cur->next;
            }
            std::cout << "\n";
        }
    }
    // Bids (highest to lowest)
    std::cout << "BIDS:\n";
    for (size_t i = bids_.getSize(); i-- > 0;) { // from end to 0
        const PriceLevel& lvl = bids_.getPriceLevel(i);
        if (!lvl.empty()) {
            std::cout << " Price " << lvl.getPrice() << " | ";
            Order* cur = lvl.head();
            while (cur) {
                std::cout << "[Id " << cur->id << " Qty " << cur->qty << "] ";
                cur = cur->next;
            }
            std::cout << "\n";
        }
    }
    std::cout << "==================\n";
}    

private:
    // Core matching logic (to implement next)
    std::vector<OrderBookEvent> try_match(Order* newOrder,
                   FixedBookSide& own_side,
                   FixedBookSide& opp_side) {
        std::vector<OrderBookEvent> events;  //return result  
        //check own_side
        if (newOrder->price==0 || !own_side.good_price(newOrder->price)) { //outside price range
            events.push_back({EventType::Reject, newOrder->id, newOrder->qty, newOrder->price});
            order_pool_.reclaim(newOrder);
            return events;
        }
        //check opp_side best price
        Price best_price=opp_side.best_price();
        if (best_price==0 ) { //no oposite side 
            events.push_back({EventType::Accepted, newOrder->id, newOrder->qty, newOrder->price});
            own_side.insert_order(newOrder);
            return events;
        }
        Index index=opp_side.price_to_index(best_price);
        Qty remaining_qty=newOrder->qty;
        Qty match_qty=0;
        int step = (newOrder->side == Side::Buy) ? +1 : -1;
        // asks book at 150 => 151 => 152 sell side   
        // bids book at 160 => 155 => 150 buy side
        while (true) {
            if (index < opp_side.getSize() && index >=0) {
   
                PriceLevel& priceL = opp_side.getPriceLevel(index);
                if (priceL.empty()) {
                    index += step;
                    continue;
                }
                Price thrPrice = priceL.getPrice();
                if (newOrder->side== Side::Buy) {
                    if (newOrder->price<thrPrice) 
                        break;
                } else {
                    if (newOrder->price>thrPrice)
                        break;
                }
    
                while (!priceL.empty() && remaining_qty>0) {
                    Order* bookOrder=priceL.head();
                    match_qty=std::min(remaining_qty, bookOrder->qty);
                    bookOrder->qty-=match_qty;
                    events.push_back({EventType::Fill, bookOrder->id, match_qty, bookOrder->price});
                    remaining_qty-=match_qty;

                    events.push_back({EventType::Fill, newOrder->id, match_qty, bookOrder->price});
                    if (bookOrder->qty==0) {
                        //priceL.pop_front();
                        opp_side.remove_order(bookOrder);
                        order_pool_.reclaim(bookOrder);
                    }
                }
     
            //next PriceLevel
                index+=step;
            } else {
                break;
            }
        }
        if (remaining_qty>0) {
            newOrder->qty=remaining_qty;
            events.push_back({EventType::Accepted, newOrder->id, newOrder->qty, newOrder->price});
            own_side.insert_order(newOrder);
        } else {
            order_pool_.reclaim(newOrder);
        }
        return events;    
    }
    
    void dispatch_book_event(std::vector<OrderBookEvent> result) {
        for (const auto& event : result) {
            message_hub_.notify(event);
        }
    }
    
    FixedBookSide bids_;
    FixedBookSide asks_;
    size_t pool_capacity{1000};
    OrderPool order_pool_;
    OrderId start_id{1};
    MessageHub message_hub_;
    
};



int main()
{
    std::cout<<"Hello World\n";
    MatchingEngine engine(100, 200, 1); // prices from 100–200, tick size 1

    std::cout << "=== Test 1: Submit Buy 150 qty=10 ===\n";
    engine.submit_new_order(Side::Buy, 150, 10);

    std::cout << "=== Test 2: Submit Sell 160 qty=5 (no match, higher ask) ===\n";
    engine.submit_new_order(Side::Sell, 160, 5);

    std::cout << "=== Test 3: Submit Sell 150 qty=6 (matches with Buy 150) ===\n";
    engine.submit_new_order(Side::Sell, 150, 6);

    std::cout << "=== Test 4: Submit Buy 160 qty=10 (matches existing Sell 160 qty=5, rest accepted) ===\n";
    engine.submit_new_order(Side::Buy, 160, 10);

    std::cout << "=== Test 5: Cancel orderId=2 (Sell 160 that was partially matched) ===\n";
    engine.cancel_order(2, Side::Sell);
    //engine.cancel_order(4, Side::Buy);
    engine.cancel_order(4);
    std::cout << "=== Best Bid/Ask ===\n";
    std::cout << "Best Bid: " << engine.best_bid() << "\n";
    std::cout << "Best Ask: " << engine.best_ask() << "\n";
    engine.submit_new_order(Side::Sell, 1500, 6);
    engine.submit_new_order(Side::Buy, 0, 6);    
    
    engine.printBook();
    std::cout << "=== TEST 6: Pool Exhaustion ===\n";
    for (int i = 0; i < 1005; i++) {
        engine.submit_new_order(Side::Buy, 170, 1);
    }
    engine.printBook();
    return 0;
}