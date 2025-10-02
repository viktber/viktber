/******************************************************************************

Welcome to GDB Online.
GDB online is an online compiler and debugger tool for C, C++, Python, Java, PHP, Ruby, Perl,
C#, OCaml, VB, Swift, Pascal, Fortran, Haskell, Objective-C, Assembly, HTML, CSS, JS, SQLite, Prolog.
Code, Compile, Run and Debug online from anywhere in world.

*******************************************************************************/
#include <iostream>
#include <string>
#include <unordered_map>
#include <queue>
#include <deque>
#include <map>
#include <vector>
#include <memory>
#include <random>
#include <iomanip>
#include <algorithm> // Required for std::find_if
#include <list>

using namespace std;


// Enum for Buy/Sell side
enum class Side {
    Buy,
    Sell
};

// Enum for the order lifecycle status
enum class Status {
    New,
    PartiallyFilled,
    Filled,
    Cancelled,
    Rejected
};

inline std::string statusToString(Status status) {
    using enum Status;
    switch (status) {
        case New: return "New";
        case PartiallyFilled: return "PartiallyFilled";
        case Filled: return "Filled";
        case Cancelled: return "Cancelled";
        case Rejected : return "Rejected";
    }
    return "UNKNOWN";
}
// Enum for the type of action on the order
enum class OrderType {
    Place,   // create/insert new order
    Pick,    // select/fulfill
    Cancel,  // cancel/remove
    Replace  // modify
};

inline std::string typeToString(OrderType type) {
    using enum OrderType;
    switch (type) {
        case Place: return "Place";
        case Pick: return "Pick";
        case Cancel: return "Cancel";
        case Replace: return "Replace";        
    }
    return "UNKNOWN";
}

// Order struct
struct Order {
    long id;              // unique order ID
    std::string symbol;   // instrument symbol
    Side side;            // Buy or Sell
    double price;         // limit price
    long qty;             // total quantity
    long remainingQty;    // qty - filledQty
    double averagePrice;  // AveragePrice = (AveragePrice * FilledQty + matchQty * matchPrice) / (FilledQty + matchQty)
                          // FilledQty += matchQty
    int priority;         // 0-10, higher = more urgent
    long timestamp;       // simple long integer for ordering
    Status status;        // current order lifecycle state
    OrderType type;       // action type
    void printOrder() const {
        std::cout << "Order ID: " << id << " "
                  << "Symbol: " << symbol << " "
                  << "Side: " << (side==Side::Buy? "Buy ":"Sell ") 
                  << std::fixed << std::setprecision(2)
                  << "Price: " << price << " "
                  << "Quantity: " << qty << " "
                  << "RemainingQty: " << remainingQty << " "
                  << "AveragePrice: " << averagePrice << " "
                  << "priority: " << priority << " "
                  << "timestamp: " << timestamp << " "
                  << statusToString(status) << " "
                  << typeToString(type) << " "
                  << "\n";
    }    
};

// Enum for processing outcome
enum class ProcessingAction {
    Dropped,       // invalid transition or unmatched
    Matched,       // successfully matched with counter-order
    Reported,      // sent to external system
    SentOutside    // forwarded to another facility/system
};

enum class EventType {
    Accepted,   // order entered book
    Fill,       // trade happened
    Cancel,     // order canceled
    Reject      // invalid order
};

struct OrderBookEvent {
    EventType type;
    long orderId;        // affected order
    long qty{0};            // qty executed (0 if not Fill)
    double price{0.00};        // price executed (0 if not Fill)
};

// Iterator into list instead of deque for a given order
  using ListIterator = std::list<Order>::iterator;
// OrderID lookup entry
struct OrderEntry {
    Side side;      // Buy or Sell
    ListIterator iterator;
};
class OrderBook {
public:
    std::vector<OrderBookEvent> processOrder (const Order& order) {
        if (order.type==OrderType::Place) {
            return matchOrders(order);
        }
        return {};
    }
    
    std::vector<OrderBookEvent> matchOrders(const Order& order ) {
        std::vector<OrderBookEvent> events;  //return result
        long remainingQty = order.remainingQty;
        long matchQty;

        if (order.side == Side::Buy) {  //new Buy order
            // check sell side
            auto it = sellLevels_.begin();  // start from lowest sell price first price first
            while (it != sellLevels_.end() && remainingQty > 0 && it->first <= order.price) {
                auto& sellList = it->second; //corresponding list of sell orders
                while (!sellList.empty() && remainingQty > 0) {
                    Order& sellOrder = sellList.front();
                    matchQty = std::min(remainingQty, sellOrder.remainingQty);

                    // Generate fill events
                    events.push_back({EventType::Fill, order.id, matchQty, sellOrder.price}); // new Buy Order
                    events.push_back({EventType::Fill, sellOrder.id, matchQty, sellOrder.price}); // book Sell Order 

                    sellOrder.remainingQty -= matchQty;
                    if (sellOrder.remainingQty == 0) {
                        // Remove fully filled order from book
                        orderIndex_.erase(sellOrder.id);
                        sellList.pop_front();
                    }                
                remainingQty -= matchQty;
                }
                // Remove empty price level
                if (sellList.empty())
                    it = sellLevels_.erase(it);
                else
                    ++it;
                // Go to next Sell Level    
            }
        } else { // new Sell Order
            auto it = buyLevels_.begin();  // Check Buy Side highest price first
            while (it != buyLevels_.end() && remainingQty > 0 && it->first >= order.price) {
                auto& buyList = it->second; //corresponding list of Buy orders
                while (!buyList.empty() && remainingQty > 0) {
                    Order& buyOrder = buyList.front();
                    matchQty = std::min(remainingQty, buyOrder.remainingQty);
                    // Generate fill events
                    events.push_back({EventType::Fill, order.id, matchQty, buyOrder.price});// new Sell Order
                    events.push_back({EventType::Fill, buyOrder.id, matchQty, buyOrder.price}); //book Buy Order
                    buyOrder.remainingQty -= matchQty;
                    remainingQty -= matchQty;
                    if (buyOrder.remainingQty == 0) {
                        // Remove fully filled order from book
                        orderIndex_.erase(buyOrder.id);
                        buyList.pop_front();
                    }
                }
                if (buyList.empty())
                    it = buyLevels_.erase(it);
                else
                    ++it;
                // Go to next Buy Level      
            }
        }
        // Add remaining qty to book
        if (remainingQty > 0) {
            Order newOrder = order;
            newOrder.remainingQty = remainingQty;
            if (newOrder.qty==newOrder.remainingQty) {
                events.push_back({EventType::Accepted, newOrder.id});
            }            
            addOrder(std::move(newOrder));
        }        
        return events;
    }   
    
    void addOrder(Order&& newOrder) {
        // listRef price-level list based on side
        auto& listRef = (newOrder.side == Side::Buy) ? 
                     buyLevels_[newOrder.price] : sellLevels_[newOrder.price];        
        // Find insertion point: higher priority first, then earlier timestamp
        auto it = std::find_if(listRef.begin(), listRef.end(),
            [&newOrder](const Order& bookOrder) {
                if (newOrder.priority > bookOrder.priority) return true;      // higher priority
                if (newOrder.priority == bookOrder.priority &&
                    newOrder.timestamp < bookOrder.timestamp) return true;   // earlier timestamp
            return false; // keep looking
            });
        // Capture id and side BEFORE moving newOrder on insert
        long id = newOrder.id;
        Side side = newOrder.side;
        // Insert order into the list
        auto listIterator = listRef.insert(it, std::move(newOrder));   // insert at found position
        orderIndex_[id] = {side, listIterator};
    }

    std::vector<OrderBookEvent> cancelOrder(const Order& order) {
  
        if (auto it = orderIndex_.find(order.id); it !=orderIndex_.end()) {
            OrderEntry& entry = it->second;
            double thePrice=entry.iterator->price;
            auto& listRef = (entry.side == Side::Buy) ? 
                  buyLevels_[thePrice] : sellLevels_[thePrice];
            listRef.erase(it->second.iterator);
            if (listRef.empty()) {
                if (entry.side == Side::Buy) {
                    buyLevels_.erase(thePrice);
                }  else {
                    sellLevels_.erase(thePrice);                    
                }  
            }    
            orderIndex_.erase(it);
            return {{EventType::Cancel, order.id}};
        }
        return {};        
    }   
private:
    // Buy side: price descending: best (highest) first
    std::map<double, std::list<Order>, std::greater<double>> buyLevels_;

    // Sell side: price ascending default Compare: best (lowest) first  
    std::map<double, std::list<Order>> sellLevels_;

    // Unified fast OrderID lookup
    std::unordered_map<long, OrderEntry> orderIndex_;
};

//Engine class
class Engine {
public:
    Engine() = default;
    Engine(const Engine& ) = delete;
    Engine& operator = (const Engine& ) = delete;
    Engine(Engine&&) = delete;
    Engine& operator = (Engine&&) = delete;
    // Generate a new order internally (randomly) and add to the queue
    void generateOrder() {
        int nOrder= 200;
        long orderId=1;
        long startTime=100;
        std::vector<std::string> symbols({"IBM", "MSFT", "SPY", "TESLA", "GE", "GOOGL", "AAPL"});
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<int> qtyDist(500, 1000);
        std::uniform_real_distribution<> priceDist(150.0, 170.0);
        std::uniform_int_distribution<int> secDist(0, symbols.size()-1);
        std::bernoulli_distribution sideDist(0.5);
        std::uniform_int_distribution<int> priorityDist(1,10);
        while (nOrder) {

            // Random generation logic here (symbol, side, price, qty, etc.)
            // Then emplace directly in the queue
            // Example placeholder:
            int randomQty=qtyDist(gen);
            orderQueue.emplace(
                orderId++,
                symbols[secDist(gen)],
                (sideDist(gen)? Side::Buy : Side::Sell), 
                priceDist(gen),
                randomQty, // qty
                randomQty, // remainingQty
                0.00,      // averagePrice
                priorityDist(gen), 
                startTime, 
                Status::New,
                OrderType::Place);
            startTime+=10;    
            if (nOrder%5==0) {
                Order orderH=orderQueue.front();
                orderH.id=orderId++;
                orderH.timestamp+=startTime;
                orderH.priority=10;
                startTime+=10;
                orderQueue.push(std::move(orderH));
            }
            nOrder--;
        }
        //Special orders
        /*
        for OrderType::Pick
        OrderType::Replace
        */
    }

    // Process orders from the queue
    void processOrders() {
        cout << "GOT HERE orderQueue size " << orderQueue.size() << "\n";
        static int debugPrint=0;
        while (!orderQueue.empty()) {
            Order next = std::move(orderQueue.front()); 
            orderQueue.pop();
            if (debugPrint<20) {
                debugPrint++;
                next.printOrder();
            }
            if (validOrder(next)) {
                if (next.type==OrderType::Place) {
                    allOrders_[next.id]=next; //add to allOrders_ map
                }
                
                //auto it = orderBooks_.try_emplace(next.symbol, std::make_unique<OrderBook>()).first;
                //auto result = it->second->processOrder(next);
                auto result = orderBooks_[next.symbol].processOrder(next);
                if (result.size()) {
                    dispatchOrderBookEvent(result);
                    processedN++;    
                } else {
                    next.status=Status::Rejected;
                    droppedN++;
                    cerr << "Attention Dropped Order: Order ID " << next.id << "\n"; 
                }
            }
        }
        cout << "orderQueue is empty N processed msgs " << processedN << "\n";
    }
    
    bool validOrder(const Order& ord) const {
        return true;
    }

    void dispatchOrderBookEvent(std::vector<OrderBookEvent>& result) {
        for (const auto& event : result) {
            switch (event.type) {
                case EventType::Fill: 
                    processFill(event);
                    break;
            }
        }    
    }
    void processFill(const OrderBookEvent& event) {
        if (auto itt=allOrders_.find(event.orderId); itt!=allOrders_.end()) {
            //   double averagePrice;  // AveragePrice = (AveragePrice * FilledQty + matchQty * matchPrice) / (FilledQty + matchQty)
            Order& order_= itt->second;
            long filledQty=order_.qty - order_.remainingQty;
            order_.averagePrice=(order_.averagePrice*filledQty+event.price*event.qty)/(filledQty+event.qty);
            order_.remainingQty-=event.qty;
            if (order_.remainingQty==0) {
                order_.status=Status::Filled;
                readyForReport.push_back(order_.id);
            } else {
                if (order_.qty!=order_.remainingQty) {
                    order_.status=Status::PartiallyFilled;
                }
            }
        }
    }
    void run() {
        generateOrder();
        processOrders();
        report();
    }
    void report() {
        cout << "Ready to report " << readyForReport.size() << " orders\n";
        for (int i=0; const auto& el : readyForReport) {
            Order& order_=(allOrders_.find(el))->second;
            if (i==0) {
                cout << "First matching order:\n";
                order_.printOrder();
            }
            cout << el << " ";  // reporting
            allOrders_.erase(el);
            i++;

            if (i%20==0) {
                cout << "\n";
            }
        } cout << "\n";
        cout << "Remaining active orders: " << allOrders_.size() << "\n" << 
        "N dropped orders: " << droppedN << "\n";
        readyForReport.clear();
        
    }
    
private:
    // Hash table of all orders by OrderID
    std::unordered_map<long, Order> allOrders_;

    // Active OrderBooks per symbol
    //std::unordered_map<std::string, std::unique_ptr<OrderBook>> orderBooks_;
    std::unordered_map<std::string, OrderBook> orderBooks_;  // Symbol, OrderBook
    // OrderID Ready for report
    std::vector<long> readyForReport;

    // Queue of Orders to be processed (in-place construction possible)
    std::queue<Order> orderQueue;
    long processedN{0};
    long droppedN{0};
};
///////////////////////////////////

struct SimpleOrderBookEvent {
    EventType type;
    uint64_t orderId;        // affected order
    uint32_t qty{0};            // qty executed (0 if not Fill)
    int64_t price{0};        // price executed (0 if not Fill)
};

struct SimpleOrder {
  enum class Side : char { Buy, Sell };
  enum class Status : char {
    Accepted,
    PartiallyFilled,
    Filled,
    Cancelled,
    Rejected
};
  uint64_t order_id_;  // Some globally unique identifier for this order, where it comes from is not important
  int64_t price_;      // Some normalized price type, details aren't important
  uint32_t quantity_;  // Number of shares to buy or sell
  uint32_t remaining_qty_;
  Side side_;          // Whether order is to buy or sell
  uint64_t timetamp;
  
};

using SimpleIterator = std::list<SimpleOrder>::iterator;
// OrderID lookup entry
struct SimpleOrderEntry {
    SimpleOrder::Side side;      // Buy or Sell
    SimpleIterator iterator;
};

struct Fill {
  // Define this class
  uint64_t order_id_;  
  int64_t price_;      
  uint32_t qty_;  
  Fill(const SimpleOrderBookEvent& event) 
  :order_id_(event.orderId),
  price_(event.price),
  qty_(event.qty)
  {
      cout << "Fill for Order " << order_id_ << " qty " << qty_ << " price " << price_ << "\n"; 
  }
};
struct Cancel {
  // Define this class
  uint64_t order_id_;
  Cancel(const SimpleOrderBookEvent& event) 
  :order_id_(event.orderId)
  {
       cout << "Cancel for Order " << order_id_ << "\n";       
  }
};
struct OrderAck {
  uint64_t order_id_;
  OrderAck(const SimpleOrderBookEvent& event) 
  :order_id_(event.orderId)
  {
      cout << "OrderAck for Order " << order_id_ << "\n"; 
  }
};

struct MessageHub {
  // You need to call these functions to notify the system of
  // fills, rejects, cancels, and order acknowledgements

  virtual void SendFill(
      Fill &) = 0; // Call twice per fill, once for each order that participated
                   // in the fill (i.e. the buy and sell orders).
  virtual void
  SendCancel(Cancel &) = 0; // Call when an order is successfully cancelled
  virtual void
  SendOrderAck(OrderAck &) = 0; // Call when a 'limit' order doesn't trade
                                // immediately, but is placed into the book
};

struct SystemMessageHub : public MessageHub {
  // You need to call these functions to notify the system of
  // fills, rejects, cancels, and order acknowledgements

  virtual void SendFill(
            Fill & fill) {
           cout << "MessageHub sent Fill\n";
      }
  virtual void SendCancel(Cancel & cancel) {
         cout << "MessageHub sent Cancel\n";   
  }
  virtual void
  SendOrderAck(OrderAck & ack) {
       cout << "MessageHub sent OrderAck\n";       
  }
};


class MatchingEngine {
  MessageHub *message_hub_;
    // Buy side: price descending: best (highest) first
    std::map<double, std::list<SimpleOrder>, std::greater<double>> buyLevels_;
    // Sell side: price ascending default Compare: best (lowest) first  
    std::map<double, std::list<SimpleOrder>> sellLevels_;
    // Unified fast OrderID lookup
    std::unordered_map<uint64_t, SimpleOrderEntry> orderIndex_;
    uint64_t start_timestamp{1};
public:
  MatchingEngine(MessageHub *message_hub)
      : message_hub_(message_hub)

  {}
  // Implement these functions, and any other supporting functions or classes
  // needed. You can assume these functions will be called by external code, and
  // that they will be used properly, i.e the order objects are valid and filled
  // out correctly. You should call the message_hub_ member to notify it of the
  // various events that happen as a result of matching orders and entering
  // orders into the order book.

  void submit_new_order(SimpleOrder &order) {
      order.remaining_qty_=order.quantity_;
      order.timetamp=start_timestamp++;
      auto result = processOrder(order);
      if (result.size()) {
          dipatch_result(result);
      }
  }
      std::vector<SimpleOrderBookEvent> processOrder(const SimpleOrder& order ) {
        std::vector<SimpleOrderBookEvent> events;  //return result
        uint32_t remainingQty = order.remaining_qty_;
        uint32_t matchQty;
        if (order.side_ == SimpleOrder::Side::Buy) {  //new Buy order
            // check sell side
            auto it = sellLevels_.begin();  // start from lowest sell price first price first
            while (it != sellLevels_.end() && remainingQty > 0 && it->first <= order.price_) {
                auto& sellList = it->second; //corresponding list of sell orders
                while (!sellList.empty() && remainingQty > 0) {
                    SimpleOrder& sellOrder = sellList.front();
                    matchQty = std::min(remainingQty, sellOrder.remaining_qty_);

                    // Generate fill events
                    events.push_back({EventType::Fill, order.order_id_, matchQty, sellOrder.price_}); // new Buy Order
                    events.push_back({EventType::Fill, sellOrder.order_id_, matchQty, sellOrder.price_}); // book Sell Order 

                    sellOrder.remaining_qty_ -= matchQty;
                    if (sellOrder.remaining_qty_ == 0) {
                        // Remove fully filled order from book
                        orderIndex_.erase(sellOrder.order_id_);
                        sellList.pop_front();
                    }                
                remainingQty -= matchQty;
                }
                // Remove empty price level
                if (sellList.empty())
                    it = sellLevels_.erase(it);
                else
                    ++it;
                // Go to next Sell Level    
            }
        }  else { // new Sell Order
            auto it = buyLevels_.begin();  // Check Buy Side highest price first
            while (it != buyLevels_.end() && remainingQty > 0 && it->first >= order.price_) {
                auto& buyList = it->second; //corresponding list of Buy orders
                while (!buyList.empty() && remainingQty > 0) {
                    SimpleOrder& buyOrder = buyList.front();
                    matchQty = std::min(remainingQty, buyOrder.remaining_qty_);
                    // Generate fill events
                    events.push_back({EventType::Fill, order.order_id_, matchQty, buyOrder.price_});// new Sell Order
                    events.push_back({EventType::Fill, buyOrder.order_id_, matchQty, buyOrder.price_}); //book Buy Order
                    buyOrder.remaining_qty_ -= matchQty;
                    remainingQty -= matchQty;
                    if (buyOrder.remaining_qty_ == 0) {
                        // Remove fully filled order from book
                        orderIndex_.erase(buyOrder.order_id_);
                        buyList.pop_front();
                    }
                }
                if (buyList.empty())
                    it = buyLevels_.erase(it);
                else
                    ++it;
                // Go to next Buy Level      
            }
        }
        // Add remaining qty to book
        if (remainingQty > 0) {
            SimpleOrder newOrder = order;
            newOrder.remaining_qty_ = remainingQty;
            events.push_back({EventType::Accepted, newOrder.order_id_});
            addOrder(std::move(newOrder));
        }  
        return events;
    }  
    
    void addOrder(SimpleOrder&& newOrder) {
        // listRef price-level list based on side
        auto& listRef = (newOrder.side_ == SimpleOrder::Side::Buy) ? 
                     buyLevels_[newOrder.price_] : sellLevels_[newOrder.price_];        
        // later timestamp to the end
        uint64_t id = newOrder.order_id_;
        SimpleOrder::Side side = newOrder.side_;
        auto itt =listRef.insert(listRef.end(),std::move(newOrder));
        orderIndex_[id] = {side, itt};
    }
    
    void dipatch_result(std::vector<SimpleOrderBookEvent> result) {
        for (const auto& event : result) {
            switch (event.type) {
                case EventType::Fill: {
                    Fill theFill(event);
                    message_hub_->SendFill(theFill);
                    break;
                }    
                case EventType::Cancel: { 
                    Cancel theCancel(event);
                    message_hub_->SendCancel(theCancel);
                    break;
                }    
                case EventType::Accepted: { 
                    OrderAck theAck(event);
                    message_hub_->SendOrderAck(theAck);
                    break;                 
                }    
            }
            
        } 
    }
    
    
  void cancel_existing_order(SimpleOrder &order) {
      auto result = cancelOrder(order);
      if (result.size()) {
          dipatch_result(result);
      }      
  }
  
    std::vector<SimpleOrderBookEvent> cancelOrder(const SimpleOrder& order) {
  
        if (auto it = orderIndex_.find(order.order_id_); it !=orderIndex_.end()) {
            SimpleOrderEntry& entry = it->second;
            auto thePrice =entry.iterator->price_;
            auto& listRef = (entry.side == SimpleOrder::Side::Buy) ? 
                  buyLevels_[thePrice] : sellLevels_[thePrice];
            listRef.erase(it->second.iterator);
            if (listRef.empty()) {
                if (entry.side == SimpleOrder::Side::Buy) {
                    buyLevels_.erase(thePrice);
                }  else {
                    sellLevels_.erase(thePrice);                    
                }  
            }    
            orderIndex_.erase(it);
            return {{EventType::Cancel, order.order_id_}};
        }
        return {};        
    } 

  // Add any additional methods you want
};


int main()
{
    {
        std::cout<<"Hello Matching Engine\n";
        Engine engine;
        engine.run();
    }
    {
        std::cout<<"Hello Simple Matching Engine\n";
        SystemMessageHub theSystem;
        MatchingEngine engine(&theSystem);
            SimpleOrder o1{1, 900, 100, 0, SimpleOrder::Side::Buy};
            SimpleOrder o2{2, 900, 50, 0, SimpleOrder::Side::Sell};
            SimpleOrder o3{3, 850, 70, 0, SimpleOrder::Side::Sell};
        engine.submit_new_order(o1);
        engine.submit_new_order(o2);
        engine.submit_new_order(o3);
        engine.cancel_existing_order(o3);
        
    }
    return 0;
}