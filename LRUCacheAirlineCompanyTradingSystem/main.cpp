/******************************************************************************

Welcome to GDB Online.
GDB online is an online compiler and debugger tool for C, C++, Python, Java, PHP, Ruby, Perl,
C#, OCaml, VB, Swift, Pascal, Fortran, Haskell, Objective-C, Assembly, HTML, CSS, JS, SQLite, Prolog.
Code, Compile, Run and Debug online from anywhere in world.

*******************************************************************************/
#include <iostream>
#include <list>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <tuple>
#include <memory>
#include <string>
#include <format>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <map>
#include <queue>


using namespace std;

/*
An LRU (Least Recently Used) cache is a type of cache that evicts the least 
recently accessed element when the cache is full. 
It is implemented using a combination of a hash map and a doubly linked list. 
The hash map provides O(1) access to the cache elements, while the doubly linked 
list maintains the order of access, allowing for efficient eviction of the 
least recently used element.
For LRU Cache, std::list remains the best choice since it allows O(1) insertions, 
deletions, and moving elements without invalidating iterators. 
recently used element move to begin by splice operations

class LRUCacheSet using std::set for cache
using Entry = std::tuple<int, int, int>; // (timestamp, key, value)
std::set<Entry> cache; // Ordered by timestamp (oldest first)
std::unordered_map<int, std::set<Entry>::iterator> key_map; // Key -> Set Iterator
int timestamp increasing on put and get; oldest element on cache.begin()

*/

class LRUCache {
private:
    int capacity;
    // List stores pairs of (key, value), ordered by MRU(Most Recently Used) 
    //to LRU(Least Recently Used)
    std::list<std::pair<int, int>> cacheList;
    // Hash map maps keys to iterators (pointers) in the list
    std::unordered_map<int, std::list<std::pair<int, int>>::iterator> cacheMap;
 
public:
    LRUCache(int capacity) : capacity(capacity) {}
    int get(int key) {
        if (cacheMap.find(key) == cacheMap.end()) return -1;

        // Move the accessed node to the front (MRU)
        auto it = cacheMap[key];
        //it move to begin
        cacheList.splice(cacheList.begin(), cacheList, it);
        return it->second;
    }
    void put(int key, int value) {
        if (cacheMap.find(key) != cacheMap.end()) {
            // Key exists: update value and move to front
            auto it = cacheMap[key];
            it->second = value;
            cacheList.splice(cacheList.begin(), cacheList, it);
        } else {
            // Key doesn't exist: check capacity
            if (cacheList.size() == capacity) {
                // Evict LRU (back of the list)
                int lruKey = cacheList.back().first;
                cacheMap.erase(lruKey);
                cacheList.pop_back();
            }
            // Insert new key-value at front
            cacheList.emplace_front(key, value);
            cacheMap[key] = cacheList.begin();
        }
    }
    void printCache() {
            for (const auto& pp : cacheList) {
                std::cout << "{"<< pp.first<< "," << pp.second<< "}" << " "; 
            }
            std::cout << endl;
    }
};

class LRUCacheSet {
private:
    using Entry = std::tuple<int, int, int>; // (timestamp, key, value)

    int capacity;
    int timestamp = 0; // Monotonically increasing timestamp

    std::set<Entry> cache; // Ordered by timestamp (oldest first)
    std::unordered_map<int, std::set<Entry>::iterator> key_map; // Key -> Set Iterator

public:
    LRUCacheSet(int cap) : capacity(cap) {}

    int get(int key) {
        if (key_map.find(key) == key_map.end()) return -1; // Not found

        // Extract value before erasing
        auto it = key_map[key];
        int value = std::get<2>(*it);  // get 3 tuple

        // Remove old entry from set 
        cache.erase(it);
        
        // Insert new entry with updated timestamp
        auto new_it = cache.insert({++timestamp, key, value}).first;
        key_map[key] = new_it;   // update map 

        return value;
    }

    void put(int key, int value) {
        if (key_map.find(key) != key_map.end()) {
            // Remove old entry before updating
            cache.erase(key_map[key]);
        } else if (cache.size() >= capacity) {
            // Remove the least recently used (oldest) entry
            auto lru = cache.begin();
            int lru_key = std::get<1>(*lru);
            
            // Remove from map and set
            //key_map.erase(lru_key); // no need do delete 
            cache.erase(lru);
        }

        // Insert new entry
        auto new_it = cache.insert({++timestamp, key, value}).first;
        key_map[key] = new_it;  // update key_map
    }

    void print() {
        cout << "LRUCacheSet size " << cache.size() << endl;
        for ( auto& x  : cache) {
            std::cout << "timestamp " << 
                std::get<0>(x) << " [" << std::get<1>(x) << "," << std::get<2>(x) << "]\n";
        }
    }
};

///////////// Airline Company Class Design
class Seat;
class Passenger;
class Airplane;
class CrewMember;
class Flight;
class AirlineCompany;

// --- Seat ---
class Seat {
public:
    int seatNumber;
    std::weak_ptr<Passenger> passenger;
    std::weak_ptr<Airplane> airplane;

    Seat(int number) : seatNumber(number) {}
    void assignePassenger(std::shared_ptr<Passenger> pass) {
        passenger=pass;
    }
    void assignePlane(std::shared_ptr<Airplane> plane) {
        airplane=plane;
    }
};

// --- Passenger ---
class Passenger {
public:
    std::string id;
    std::weak_ptr<Seat> assignedSeat;
    std::weak_ptr<Flight> bookedFlight;
    //Reason for Smart Pointers:
    //reference to assigned Seat and Flight
    void bookFlight(std::shared_ptr<Flight> flight) {
        bookedFlight = flight;
    }
    void assignSeat(std::shared_ptr<Seat> seat) {
        assignedSeat = seat;
    }
    Passenger(const std::string& id) : id(id) {}
    void printPassenger();
    std::weak_ptr<Seat> getCurrentSeat() const { return assignedSeat; }
};

// --- CrewMember ---
class CrewMember {
    std::string id;  
    std::weak_ptr<Flight> assignedFlight;
    //Reason for Smart Pointers:
    //reference to Flight
public:

    std::string role;

    CrewMember(const std::string& _id, const std::string& _role) : id(_id), role(_role) {}
    std::string getId() const { return id; }
    void assignFlight(std::shared_ptr<Flight> flight) {
        assignedFlight = flight;
    }
    std::weak_ptr<Flight> getFlight() const { return assignedFlight;}
    void printCrewMember() const {
        cout << id << " " << role << "\n";
    }
    
};

// --- Airplane ---
class Airplane {
public:
    std::string model;
    int capacity;
    std::unordered_map<int, std::shared_ptr<Seat>> seats;
    //Reason for Smart Pointers:
    //Seats can be shared between Flight and Airplane
    //Ensures that seats remain valid as long as they are used by either Airplane or Flight.

    Airplane(const std::string& model, int capacity) : model(model), capacity(capacity) {
        
        for (int i = 1; i <= capacity; ++i) {
            seats[i] = std::make_shared<Seat>(i);
        }
    }
    void printAirplane() {
        cout << "Model " << model << " capacity " << capacity << "\n";
    }
};

// --- Flight ---
class Flight {
private:
std::string flightNumber;
std::chrono::system_clock::time_point departureTime;
std::chrono::system_clock::time_point arrivalTime;
public:
    std::weak_ptr<Airplane> airplane;
    std::unordered_map<int, std::shared_ptr<Seat>> seats;
    std::unordered_map<std::string, std::weak_ptr<CrewMember>> crew;
    std::unordered_map<std::string, std::shared_ptr<Passenger>> flightPass;
    //Reason for Smart Pointers
    //weak_ptr reference to Airplane
    //unordered_map weak_ptr reference to CrewMember
    //unordered_map shared_ptr<Passenger> Flight owned Passenger

    Flight(const std::string& flightNumber, std::shared_ptr<Airplane> _airplane)
        : flightNumber(flightNumber), airplane(_airplane) {
        if (auto planePtr = airplane.lock()) {
            seats = planePtr->seats;
        }
    }
    void addCrewMember(const std::string& id, std::shared_ptr<CrewMember> crewMember) {
        crew[id] = crewMember;
    }
    void addPassenger(const std::string& passId, std::shared_ptr<Flight> flight,
            int sitN ) {
        auto passPtr = std::make_shared<Passenger>(passId);
        passPtr->bookFlight(flight);
        if (seats.count(sitN)==0) {
            cout << "Seat N " << sitN << " not exist\n";
        }
        if (seats[sitN]->passenger.expired()) {
            seats[sitN]->assignePassenger(passPtr);
            seats[sitN]->assignePlane(airplane.lock());
            flightPass[passId]=passPtr;
            passPtr->assignSeat(seats[sitN]);
        }
    }
    void setDepartureTime(const std::chrono::system_clock::time_point& time) {
        departureTime=time;
    }
    void setArrivalTime(const std::chrono::system_clock::time_point& time) {
        arrivalTime=time;
    }
    
    std::chrono::system_clock::time_point getDepartureTime() const { return departureTime; }
    std::chrono::system_clock::time_point getArrivalTime() const { return arrivalTime; }
    
    bool ifScheduled() { 
        return departureTime != std::chrono::time_point<std::chrono::system_clock>(); 
    }
    
    void printCrew() {
        for (const auto& c : crew) {
            std::shared_ptr<CrewMember> tempPtr=c.second.lock();
            if (tempPtr) {
                tempPtr->printCrewMember();
            }
        }
    }
    void printPassengers() {
        for (const auto& s : seats) {
            if (!s.second->passenger.expired()) {
                auto tempPass=s.second->passenger.lock();
                cout << tempPass->id << " seatNumber " << s.first << endl;
            }
        }
    }
    void printSchedule() const {
        auto printTime = [](const std::chrono::system_clock::time_point& time) {
            auto tt = std::chrono::system_clock::to_time_t(time);
            std::cout << std::put_time(std::localtime(&tt), "%Y-%m-%d %H:%M:%S");
        };
        std::cout << "Flight " << flightNumber << " Schedule:\n";
        std::cout << "Departure: ";
        printTime(departureTime);
        std::cout << " Arrival: ";
        printTime(arrivalTime);
        auto duration = 
        std::chrono::duration_cast<std::chrono::minutes>(arrivalTime - departureTime);
        cout << " flight duration " << duration.count() << " min\n"; 
    }
    
    std::shared_ptr<Passenger> getPassenger(const string& theN) {
        if (flightPass.count(theN)) {
            return flightPass[theN];
        }
        return nullptr;
    }
    
    void printFlight() {
        cout << "Flight " << flightNumber << " " ;
        airplane.lock()->printAirplane();
    }
    void printAvailableSits() {
        cout << "Free seats on Flight " << flightNumber << " ";
        set<int> freeSeats;
        for (const auto& s : seats) {
            if (s.second->passenger.expired()) {
                freeSeats.insert(s.first); 
            } 
        }
        if (freeSeats.size()) {
            for (const auto& s : freeSeats)
                cout << s << " "; cout << "\n";
        } else 
            cout << "No seats available\n";
        
    }
    bool checkSeat(int sitN) const {
        return seats.count(sitN)!=0;
    }
    std::shared_ptr<Seat> getSeatInfo(int sitN) {
        if (seats.count(sitN)==0)
            return nullptr;
        else
            return seats[sitN];
    } 
    
};

void Passenger::printPassenger() {
        cout << "Passenger " << id << " ";
        if (!bookedFlight.expired()) {
            bookedFlight.lock()->printFlight();
            if (!assignedSeat.expired()) {
                cout << "Seat N " << assignedSeat.lock()->seatNumber << endl;
            } else {
                cout << "No assigned any seat\n";
            }
        }
        else 
            cout << "Not on any flight\n";
}
// --- AirlineCompany ---
class AirlineCompany {
    
public:
    // Flight Id => shared_ptr Flight
    std::unordered_map<std::string, std::shared_ptr<Flight>> flights;
    // Airplane ID => shared_ptr Airplane   
    std::unordered_map<std::string, std::shared_ptr<Airplane>> airplanes;
    //CrewMember ID => shared_ptr CrewMember
    std::unordered_map<std::string, std::shared_ptr<CrewMember>> crewMembers;
    //Reason for Smart Pointers:
    //Centralized management of resources (airplanes, flights, crewMembers) in 
    //a single AirlineCompany object; Sharing of these resources across 
    //multiple classes (Flight, Passenger, CrewMember)

    void addAirplane(const std::string& id, const std::string& model, int capacity) {
        cout << "Adding Airplane " << id << " Model " << model << " Capacity " << capacity << endl; 
        if (!airplaneIds.insert(id).second) {
            cout << "AirlineCompany already has Airplane " << id << endl;
            return;
        } 
        airplanes[id] = std::make_shared<Airplane>(model, capacity);
    }

    void addFlight(const std::string& flightNumber, const std::string& airplaneId) {
        auto it = airplanes.find(airplaneId);
        if (it != airplanes.end()) {
            flights[flightNumber] = std::make_shared<Flight>(flightNumber, it->second);
            cout << "Add Flight flightNumber " << flightNumber << " Airplane Id " << airplaneId << " ";
            it->second->printAirplane();
        } else {
            cout << "AirlineCompany doesn't have Airplane with " << airplaneId << endl;
        }
    }
    
    void addCrewMemberToFlight(const std::string& flightNumber, std::vector<std::shared_ptr<CrewMember>>&& crew) {
        auto it = flights.find(flightNumber);
        if (it==flights.end()) {
            std::cerr << "Flight " << flightNumber << " not found.\n";
            return;
        }
        auto flight=it->second;
        for (const auto c : crew) {
            if (c) {
                flight->addCrewMember(c->getId(), c);
                c->assignFlight(flight);
                crewMembers[c->getId()]=c;
            }
        }
    }
    
    void getCrew(const std::string& flightNumber) const {
        auto it = flights.find(flightNumber);
        if (it==flights.end()) {
            std::cerr << "Flight " << flightNumber << " not found.\n";
            return;
        }
        cout << "Crew on Flight " << flightNumber << "\n";
        it->second->printCrew();
    }
    
    void bookFlight(const std::string& pass, const std::string& flightNumber, 
            int sitN) {
        auto it = flights.find(flightNumber);
        if (it==flights.end()) {
            std::cerr << "Flight " << flightNumber << " not found.\n";
            return;
        }
        it->second->addPassenger(pass, it->second, sitN);
        
    }
    void  freeSeats(const std::string& flightNumber) {
        auto it = flights.find(flightNumber);
        if (it==flights.end()) {
            std::cerr << "Flight " << flightNumber << " not found.\n";
            return;
        }
        it->second->printAvailableSits();
    }
    void getPassengerList(const std::string& flightNumber) const {
        auto it = flights.find(flightNumber);
        if (it==flights.end()) {
            std::cerr << "Flight " << flightNumber << " not found.\n";
            return;
        }
        cout << "Passengers on Flight " << flightNumber << "\n";
        it->second->printPassengers();
    } 
    bool switchSeat(const std::string& flightNumber, const string& theName, 
                    int newSitN) {
        auto it = flights.find(flightNumber);        
        if (it==flights.end()) {
            std::cerr << "Flight " << flightNumber << " not found.\n";
            return false;
            
        }  
        if (!it->second->checkSeat(newSitN)) {
            std::cerr << "Seat " << newSitN << " not exist on Flight" << flightNumber << "\n";
            return false;
        }
        auto pass = it->second->getPassenger(theName);
        if (!pass) {
            std::cerr <<"No Passenger " << theName << " on Flight " << flightNumber << "\n";
            return false;
        }    
        auto currentSeat = pass->getCurrentSeat();
        if (currentSeat.lock()->seatNumber == newSitN) {
            std::cerr << theName << " not moving;" << "New and old seats the same  " << newSitN << "\n";
            return false;
        }
        auto newSeat = it->second->getSeatInfo(newSitN);
        if (!newSeat->passenger.expired()) {
           std::cerr << "Seat " << newSitN << " assigned to another passenger " <<
           newSeat->passenger.lock()->id << "\n";
           
        }
        //switch here 
        newSeat->assignePassenger(pass);
        pass->assignSeat(newSeat);
        currentSeat.lock()->passenger.reset();
        cout << "Passenger " << theName << " moved from " << currentSeat.lock()->seatNumber <<
        " to new seat " << newSeat->seatNumber << "\n";
        
        return true;
    }
    
    void scheduleFlight(const std::string& flightNumber, 
                                    const std::chrono::system_clock::time_point& departure, 
                                    const std::chrono::system_clock::time_point& arrival) {

        auto it = flights.find(flightNumber);        
        if (it==flights.end()) {
            std::cerr << "Flight " << flightNumber << " not found.\n";
            return;
        } 
        if (it->second->ifScheduled()) {
            cout << "Update Schedule:\n";
        }
        it->second->setArrivalTime(arrival);
        it->second->setDepartureTime(departure);
        it->second->printSchedule();
    }
    std::shared_ptr<Passenger> findPassengerByName(const string& theName, bool print=false) {
        for (const auto& [flight, flightPtr] : flights ) {
           auto pass = flightPtr->getPassenger(theName);
           if (pass) {
              if (print)   
                pass->printPassenger();
              return pass;
           }
        }
        return nullptr;
    }
private:
    std::set<std::string> airplaneIds;
};
//////////////////////////////////////
// Traiding
/*
Trade: Represents a single trade, including fields like trade ID, order ID, quantity, price, timestamp, etc.
Order: Represents an order in the system, with attributes like order ID, type (buy/sell), quantity, price, etc.
Security: Represents a financial instrument with fields like symbol, exchange, last price, etc.
Exchange: Represents a stock exchange with methods to route orders and update prices.
Router: Responsible for directing orders to the correct exchange or processing engine.

Order <=> Security, Exchange, Trades
Trade <=> Order, Security, Buy Sell Order
OrderBook <=> Orders Open and filled (create Orders), Match Orders(create Trades)  
OrderBookManager <=> multiple OrderBooks
Router <=> routes to Exchange, submits to OrderBookManager

class Security
struct TradingHours
class Exchange

class ExchangeRegistry singleton manager
Manages registered exchanges.
Creates and stores exchanges with fees, open/close times.
Allows lookup of exchanges by name.

class SecurityManager  singleton manager
Manages financial securities.
Creates securities with symbols and prices.
Tracks security exchange identifiers for routing.

enum class OrderStatus
class Order
Represents an individual order:
Attributes: order side (Buy/Sell), price, quantity, remaining quantity, status.
Tracks whether it originated from FIX.
Tracks last trade details for partial fills.
Provides accessors for order details used by FIX engine and order book.

class Trade

class Router           singleton manager
Receives orders (e.g., from FIX engine).
Passes new orders for acknowledgment (FixEngine::newOrderAck).
Sends orders to OrderBookManager for matching and book management.
Maintains exchanges and routes orders accordingly.

class OrderBook
Manages queues of buy and sell orders.
Matches orders and processes fills:
Fully filled orders are marked FILLED and removed from the book.
Partially filled orders are updated accordingly.
Calls FixEngine to send execution reports when orders are filled or partially filled.
Maintains internal data structures (filledOrders, queues).

class OrderBookManager    singleton manager
Manages order books for different securities.
Creates new orders based on inputs (side, price, quantity, destination, security).
Prints the order book state.
Supports multiple securities and exchanges.
Interfaces with OrderBook instances for matching orders

class IFixConnector (Interface) Abstract base class

class FixConnector (Concrete Implementation of IFixConnector)
Implements sendExecutionReport by printing FIX execution reports.
Implements gotNewOrder to create and send a FIX New Order Single message through FixEngine.
Acts as a bridge between the FIX message layer and the internal order engine

FixEngine (Singleton)
Central FIX protocol engine managing:
Sending and receiving FIX messages.
Maintaining an associated IFixConnector (non-owning pointer).
Generating FIX execution reports (acknowledgment, partial fill, full fill).
Keeping a log (fixReport) of all FIX messages per client order (ClOrdID).
Provides utility methods to generate unique FIX IDs (makeClOrdID, makeExecID).
Parses incoming FIX messages and creates internal orders via OrderBookManager.
Sends execution reports to the connector when orders are acknowledged or filled.
Singleton pattern ensures a single FIX engine instance globally.

*/

std::string getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t timeNow = std::chrono::system_clock::to_time_t(now-std::chrono::hours(4));
    std::tm tmNow = *std::localtime(&timeNow);  // Or use gmtime() for UTC

    std::ostringstream oss;
    oss << std::put_time(&tmNow, "%Y-%m-%dT%H:%M:%S");
    return oss.str();
}


class Security {
private:
    std::string symbol;
    double lastPrice;
    std::unordered_map<std::string, std::string> exchangeIDs;

public:
    Security(const std::string& symbol, double lastPrice)
        : symbol(symbol), lastPrice(lastPrice) {}

    std::string getSymbol() const { return symbol; }
                    
    double getLastPrice() const { return lastPrice; }

    void updatePrice(double newPrice) { lastPrice = newPrice; }

    void addExchangeID(const std::string& exchange, const std::string& exchangeID) {
        exchangeIDs[exchange] = exchangeID;
    }

    std::string getExchangeID(const std::string& exchange) const {
        auto it = exchangeIDs.find(exchange);
        return it != exchangeIDs.end() ? it->second : "";
    }

    void printSecurity() const {
        std::cout << "Symbol: " << symbol << "\n"
                  << "Exchanges: ";
        std::cout << "\nLast Price: " << lastPrice << "\n";
        for (const auto& [ex, id] : exchangeIDs) {
            std::cout << "Exchange: " << ex << " - ID: " << id << "\n";
        }
    }
};

struct TradingHours {
    int openMinutes;
    int closeMinutes;

    bool isOpenNow() const {
        int now = getCurrentTimeMinutes();
        return now >= openMinutes && now < closeMinutes;
    }

    static int getCurrentTimeMinutes() {
        using namespace std::chrono;
        auto now = system_clock::now();
        time_t tt = system_clock::to_time_t(now-std::chrono::hours(4)); //NYC
        tm local_tm = *localtime(&tt);
        return local_tm.tm_hour * 60 + local_tm.tm_min;
    }
};


class Exchange {
private:
    std::string name;
    double feePercentage;
    TradingHours hours;
    bool testMode;
public:
    Exchange(const std::string& name, double feePercentage,
             int openTime, int closeTime, bool test=true)
        : name(name), feePercentage(feePercentage), hours{openTime, closeTime}, 
          testMode(test)
        {}

    bool isMarketOpen() const {
        return ((testMode)? true : hours.isOpenNow());
    }
    
    std::string getName() const { return name; }
    double getFeePercentage() const { return feePercentage; }
    
    void printInfo() const {
        std::cout << "Exchange: " << name << ", Fee: " << feePercentage
                  << "%, Trading Hours: " << (hours.openMinutes / 60) << ":"
                  << (hours.openMinutes % 60) << " - " << (hours.closeMinutes / 60)
                  << ":" << (hours.closeMinutes % 60) << " Market " <<
                  (isMarketOpen()?"open":"closed") << "\n";
    }
};

//ExchangeRegistry Singleton
//Creating/storing exchanges
//Lookup by name

class ExchangeRegistry {
private:
    std::unordered_map<std::string, std::shared_ptr<Exchange>> exchanges;

    // Private constructor
    ExchangeRegistry() = default;

public:
    // Delete copy/move constructors and assignment
    ExchangeRegistry(const ExchangeRegistry&) = delete;
    ExchangeRegistry& operator=(const ExchangeRegistry&) = delete;

    static ExchangeRegistry& getInstance() {
        static ExchangeRegistry instance;
        return instance;
    }

    std::shared_ptr<Exchange> createExchange(const std::string& name,
                                             double feePercentage,
                                             int openTimeMinutes,
                                             int closeTimeMinutes) {
        auto exch = std::make_shared<Exchange>(name, feePercentage, openTimeMinutes, closeTimeMinutes);
        exchanges[name] = exch;
        return exch;
    }

    std::shared_ptr<Exchange> getExchange(const std::string& name) const {
        auto it = exchanges.find(name);
        return it != exchanges.end() ? it->second : nullptr;
    }

    void printAll() const {
        for (const auto& [name, exch] : exchanges) {
            exch->printInfo();
        }
    }
};

class SecurityManager {
private:
    std::unordered_map<std::string, std::shared_ptr<Security>> securities;

    // Private constructor for singleton
    SecurityManager() = default;

public:
    // Disable copy/move
    SecurityManager(const SecurityManager&) = delete;
    SecurityManager& operator=(const SecurityManager&) = delete;

    // Singleton accessor
    static SecurityManager& getInstance() {
        static SecurityManager instance;
        return instance;
    }

    // Factory method
    std::shared_ptr<Security> createSecurity(const std::string& symbol,
                                         double lastPrice) {
        auto sec = std::make_shared<Security>(symbol, lastPrice);
        securities[symbol] = sec;
        return sec;
    }

    std::shared_ptr<Security> getSecurity(const std::string& symbol) const {
        auto it = securities.find(symbol);
        return it != securities.end() ? it->second : nullptr;
    }

    void printAll() const {
        for (const auto& [symbol, sec] : securities) {
            sec->printSecurity();
            std::cout << "--------------------------\n";
        }
    }
};

class Trade;  // Forward declaration

using Timestamp = long long;

Timestamp getNanoTimestamp() {
    auto now = std::chrono::high_resolution_clock::now();
    now -=std::chrono::hours(4); //NYC
    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch());
    return ns.count();  // nanoseconds since epoch
}

enum class OrderStatus {
    NEW,
    PARTIALLY_FILLED,
    FILLED,
    CANCELLED,
    REJECTED
};

std::string statusToString(OrderStatus status) {
    switch (status) {
        case OrderStatus::NEW: return "NEW";
        case OrderStatus::PARTIALLY_FILLED: return "PARTIALLY_FILLED";
        case OrderStatus::FILLED: return "FILLED";
        case OrderStatus::CANCELLED: return "CANCELLED";
        case OrderStatus::REJECTED: return "REJECTED";
    }
    return "UNKNOWN";
}


class Order {
private:
    std::string orderID;
    std::string destinationExchange;
    std::string orderType; // "Buy" or "Sell"
    double price;
    int quantity;
    int remainingQuantity;
    std::shared_ptr<Security> securityRef;
    std::vector<std::shared_ptr<Trade>> trades;
    Timestamp timestamp;
    OrderStatus status;
    std::string ClOrdID;
public:
    Order(const std::string& orderID, const std::string& orderType,
          double price, int quantity, std::shared_ptr<Security> securityRef,  
          const std::string& destinationExchange,
          const std::string& theClOrId=""
          )
        : orderID(orderID), orderType(orderType),
          price(price), quantity(quantity), remainingQuantity(quantity), securityRef(securityRef),
          destinationExchange(destinationExchange),
          timestamp(getNanoTimestamp()), status(OrderStatus::NEW), ClOrdID(theClOrId)
          {
              
          }

    std::string getOrderID() const { return orderID; }
    std::string getClOrdID() const { return ClOrdID; }
    std::string getSecuritySymbol() const { return securityRef->getSymbol(); }
    std::string getOrderType() const { return orderType; }
    std::string getOrderSide() const { return ((orderType=="Buy")?"1":"2"); }
    bool isFixOrder() const { return !ClOrdID.empty(); }
    double getPrice() const { return price; }
    int getQuantity() const { return quantity; }
    int getRemainingQuantity() const { return remainingQuantity; }
    std::string getDestinationExchange() const { return destinationExchange; }
    
    std::shared_ptr<Security> getSecurityRef() { return securityRef; }

    Timestamp getOrderTimestamp() { return timestamp; }

    void addTrade(std::shared_ptr<Trade> trade);
    void setStatus(OrderStatus theStatus) { status = theStatus; }
    std::shared_ptr<Trade> getLastTrade() {
        if (trades.empty()) {
            return nullptr;
        } else {
            return trades.back();
        }
    }
    void printOrder() const {
        std::cout << "Order ID: " << orderID << " "
                  << "Security: " << getSecuritySymbol() << " "
                  << "Type: " << orderType << " "
                  << std::fixed << std::setprecision(2)
                  << "Price: " << price << " "
                  << "Quantity: " << quantity << " "
                  << "Remaining: " << remainingQuantity << " "
                  << statusToString(status) << " "
                  << (isFixOrder()?"Fix":"Regular") << " order"
                  << "\n";
    }
};

class Trade {
private:
    std::string tradeID;
    double price;
    int quantity;
    std::string timestamp;
    std::weak_ptr<Order> buyOrder;
    std::weak_ptr<Order> sellOrder;
    std::shared_ptr<Security> securityRef;

public:
    Trade(const std::string& tradeID, 
          double price, int quantity, const std::string& timestamp,
          std::shared_ptr<Order> buyOrderRef, 
          std::shared_ptr<Order> sellOrderRef,           
          std::shared_ptr<Security> securityRef)
          : tradeID(tradeID),
          price(price), quantity(quantity), timestamp(timestamp),
          buyOrder(buyOrderRef), sellOrder(sellOrderRef), 
          securityRef(securityRef) {}

    std::string getTradeID() const { return tradeID; }
    std::string getSecuritySymbol() const { return securityRef->getSymbol(); }
    double getPrice() const { return price; }
    int getQuantity() const { return quantity; }
    std::string getTimestamp() const { return timestamp; }

    void printTrade() const {
        if (buyOrder.expired() || sellOrder.expired()) {
            std::cout << "For Trade ID: " << tradeID << " Order expired\n";
            return;
        }
        std::cout << "Trade ID: " << tradeID << " "
                  << "Buy Order ID: " << buyOrder.lock()->getOrderID() 
                  << " Sell Order ID: " << sellOrder.lock()->getOrderID() << "\n"
                  << "Security: " << getSecuritySymbol() << " "
                  << std::fixed << std::setprecision(2)
                  << "Price: " << price << " "
                  << "Quantity: " << quantity << " "
                  << "Timestamp: " << timestamp << "\n";
    }
};

void Order::addTrade(std::shared_ptr<Trade> trade) {
    trades.push_back(trade);
    remainingQuantity -= trade->getQuantity();
}

/*
Use the order’s destinationExchange to find the correct exchange.
Fetch the security ID from the Security object for the specified exchange.
Route the order only if the exchange is open
*/

class Router {
private:
    std::unordered_map<std::string, std::shared_ptr<Exchange>> exchanges;
    // Private constructor to enforce singleton
    Router() = default;
public:
    // Delete copy and move constructors/assignments
    Router(const Router&) = delete;
    Router& operator=(const Router&) = delete;

    // Access the singleton instance
    static Router& getInstance() {
        static Router instance;
        return instance;
    }

    void addExchange(const std::shared_ptr<Exchange>& exchange) {
        exchanges[exchange->getName()] = exchange;
        cout << "Added " << exchange->getName() << " to routing\n";
    }

    void routeOrder(const std::shared_ptr<Order>& order) {
        std::string destination = order->getDestinationExchange();
        auto it = exchanges.find(destination);

        if (it == exchanges.end()) {
            std::cout << "Exchange " << destination << " not found.\n";
            return;
        }

        auto exchange = it->second;
        if (!exchange->isMarketOpen()) {
            std::cout << "Market at " << destination << " is closed.\n";
            return;
        }

        auto secRef = order->getSecurityRef();
        std::string secID = secRef->getExchangeID(destination);
        if (secID.empty()) {
            std::cout << "Security not tradable at " << destination << ".\n";
            return;
        }

        std::cout << "Order " << order->getOrderID() << " " <<
        order->getOrderType() << " routed to " << destination
        << " for symbol " << secRef->getSymbol() << " qty " <<
        order->getQuantity() << " with exchange ID " << secID << ".\n";
        // Routing succeeded — send to OrderBook
        processOrder(order);
    }
    void processOrder(const std::shared_ptr<Order>& order);

};


/*
Create Order
Maintain separate lists for buy and sell orders.
Match incoming orders against existing orders - Create Trade.
Update order quantities, generate trades, and remove completed orders.
*/

// struct for priority_queue on buy and sell orders
// for buy order first come with highter price
// for sell order first come with lowest price


struct BuyOrderComparator {
    bool operator()(const std::shared_ptr<Order>& lhs, const std::shared_ptr<Order>& rhs) const {
        if (lhs->getPrice() == rhs->getPrice()) {
            return lhs->getOrderTimestamp() > rhs->getOrderTimestamp(); // earlier orders have smaller ID
        }
        return lhs->getPrice() < rhs->getPrice(); // higher price first
    }
};

struct SellOrderComparator {
    bool operator()(const std::shared_ptr<Order>& lhs, const std::shared_ptr<Order>& rhs) const {
        if (lhs->getPrice() == rhs->getPrice()) {
            return lhs->getOrderTimestamp() > rhs->getOrderTimestamp();
        }
        return lhs->getPrice() > rhs->getPrice(); // lower price first
    }
};


class OrderBook {
private:

std::priority_queue<std::shared_ptr<Order>, std::vector<std::shared_ptr<Order>>, BuyOrderComparator> buyOrders;
std::priority_queue<std::shared_ptr<Order>, std::vector<std::shared_ptr<Order>>, SellOrderComparator> sellOrders;
std::string symbol;
std::vector<std::shared_ptr<Trade>> trades;
std::vector<std::shared_ptr<Order>> filledOrders;

public:
    explicit OrderBook(const std::string& symbol) : symbol(symbol) {}

    const std::string& getSymbol() const { return symbol; }

    void createOrder(const std::string& orderType,
                 double price,
                 int quantity,
                 const std::string& destination,
                 const std::shared_ptr<Security>& securityRef,
                 const std::string& theClOrId=""
                 )
    {
        static int nextOrderID = 1;
        std::string orderID = "ORD" + std::to_string(nextOrderID++);

        auto order = std::make_shared<Order>(orderID, orderType, price, quantity, 
        securityRef, destination,theClOrId);

        // Route the order for validation and dispatch
        Router::getInstance().routeOrder(order);
    }
    
    
    void addOrder(const std::shared_ptr<Order>& order) {
        if (order->getOrderType() == "Buy") {
            buyOrders.push(order);
        } else if (order->getOrderType() == "Sell") {
            sellOrders.push(order);
        } else {
            std::cout << "Invalid order type: " << order->getOrderType() << "\n";
            return ;
        }
        // Attempt to match immediately after adding
        matchOrders();
    }

    void matchOrders() {
        while (!buyOrders.empty() && !sellOrders.empty()) {
            auto buyOrder = buyOrders.top();
            auto sellOrder = sellOrders.top();
            
            if (buyOrder->getPrice() < sellOrder->getPrice() ||
                buyOrder->getRemainingQuantity() == 0 ||
                sellOrder->getRemainingQuantity() == 0) {
                break;
            }
            
            cout << "OrderBook " <<  symbol << 
            " matching buyOrder " << buyOrder->getOrderID() << " to sellOrder " <<
            sellOrder->getOrderID() << " creating trade\n";
            int tradeQuantity = std::min(buyOrder->getRemainingQuantity(), sellOrder->getRemainingQuantity());
            static int nextTradeID = 1;
            std::string tradeID = "TRD" + std::to_string(nextTradeID++);
            // Create a trade

            auto trade = std::make_shared<Trade>(tradeID, sellOrder->getPrice(),
                tradeQuantity, getCurrentTimestamp(), 
                buyOrder, sellOrder, buyOrder->getSecurityRef());
                
            // Adjust quantities
            buyOrder->addTrade(trade);
            sellOrder->addTrade(trade);
            trades.push_back(trade);
            trade->printTrade();
            
            // Remove filled orders
            processFilledOrders(buyOrder, sellOrder);

        }
    }
    
    void processFilledOrders(std::shared_ptr<Order> buyOrder, std::shared_ptr<Order> sellOrder);

    void printOrderBook() const {
        //Print queue with copy
        std::cout << "OrderBook " << symbol <<":\n";
        std::cout << "Buy Open Orders:\n";
        auto buyCopy = buyOrders;
        while (!buyCopy.empty()) {
            auto order = buyCopy.top(); buyCopy.pop();
            order->printOrder();
        }
        std::cout << "Sell Open Orders:\n";
        auto sellCopy = sellOrders;
        while (!sellCopy.empty()) {
            auto order = sellCopy.top(); sellCopy.pop();
            order->printOrder();
        }
        if (filledOrders.size()>0) {
            std::cout << "Filled Orders:\n";
            for (const auto& order : filledOrders) {
                order->printOrder();
            }
        }
        
    }
    
    void printTrades() const {
        std::cout << "OrderBook " << symbol <<" Trades:\n";
        if (trades.empty()) {
            std::cout << "No trades yet.\n";
        } else {
            for (const auto& trade : trades)
                trade->printTrade();
        }
    }

};

class OrderBookManager {
private:
    std::unordered_map<std::string, std::unique_ptr<OrderBook>> books;

    OrderBookManager() = default;

public:
    static OrderBookManager& getInstance() {
        static OrderBookManager instance;
        return instance;
    }

    OrderBook& getOrderBook(const std::string& symbol) {
        if (books.find(symbol) == books.end()) {
            books[symbol] = std::make_unique<OrderBook>(symbol);
        }
        return *books[symbol];
    }

    void createOrder(const std::string& orderType,
                 double price,
                 int quantity,
                 const std::string& destination,
                 const std::shared_ptr<Security>& securityRef,
                 const std::string& theClOrId="") { 
        getOrderBook(securityRef->getSymbol()).createOrder(
                orderType,
                price,
                quantity,
                destination,
                securityRef,
                theClOrId
            );
    }
    
    void addOrder(const std::shared_ptr<Order>& order) {
        getOrderBook(order->getSecurityRef()->getSymbol()).addOrder(order);
    }
    void printOrderBook(const std::string& symbol, bool printBook=true) {
        if (auto it=books.find(symbol); it != books.end()) {
            if (printBook)
                it->second->printOrderBook();
            else
                it->second->printTrades();
        } else {
            cout << "No OrderBook for " << symbol << "\n";
        }
    }
    
    void printAllBooks() {
        for (auto& [symbol, book] : books) {
            std::cout << "OrderBook for " << symbol << ":\n";
            book->printOrderBook();
        }
    }
};


/*
Trade/OrderBookManager
        ↓
   FixEngine (creates FIX 35=8 Execution Report)
        ↓
FixConnector (sends it out)
*/

/*
ClOrdID (Tag 11) => Your unique order ID (client-side).
OrderID (Tag 37) tradeID Broker’s unique ID (assigned in the first Execution Report).
OrigClOrdID (Tag 41) => Used in cancels/modifications to reference the original order.

Key Tags for Exchange/Destination Identification
Tag	Field Name	Purpose	Example Value
56	TargetCompID	Primary field: The FIX counterparty ID 
    (e.g., exchange/broker ID).	NASDAQ, NYSE, BROKER_X
100	ExDestination	(Optional) Specifies the specific exchange/market within 
    a broker’s routing network.	ISLAND, BATS, ARCA
49	SenderCompID	Your firm’s FIX ID (source of the message).	
    TRADER_FIRM
50	SenderSubID	(Optional) Sub-identifier for your firm (e.g., trading desk).	
    EQUITIES_DESK
57	TargetSubID	(Optional) Sub-identifier for the exchange/broker 
    (e.g., gateway).	PROD
*/

// Fix Functions

// Split string by delimiter
std::vector<std::string> split(const std::string& s, char delim) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream stream(s);
    while (std::getline(stream, token, delim)) {
        if (!token.empty()) tokens.push_back(token);
    }
    return tokens;
}

// Parse FIX message into a map of tag-value pairs
std::map<int, std::string> parseFixMessage(const std::string& fixMsg) {
    std::map<int, std::string> fields;
    std::vector<std::string> parts = split(fixMsg, '|');

    for (const std::string& part : parts) {
        size_t eqPos = part.find('=');
        if (eqPos != std::string::npos) {
            int tag = std::stoi(part.substr(0, eqPos));
            std::string value = part.substr(eqPos + 1);
            fields[tag] = value;
        }
    }

    return fields;
}



std::string constructFixMessage(const std::map<int, std::string>& fields) {
    std::ostringstream msg;
    //ignoring checksum <10> |10=150| Trailer
    //body length BodyLength <9> = 500 length, in bytes, till to the CheckSum 
    //ALWAYS SECOND FIELD IN MESSAGE 
    

    // Start with header (FIX 4.2)
    msg << "8=FIX.4.2|9=500|";  // Placeholder for length (calculated later)

    // Add fields
    for (const auto& [tag, value] : fields) {
        msg << tag << "=" << value << "|";
    } msg << "10=150|";


    // Insert length (field 9) and add checksum (field 10)
    //std::string fullMsg = "8=FIX.4.2|9=" + std::to_string(body.size() - 12) + "|" + 
    //                      body.substr(12) + "10=" + std::setfill('0') << std::setw(3) << checksum << "|";

    return msg.str();
}

std::string createNewOrderSingle( const std::string& orderType,
                                  const std::string& ClOrdID,
                                  const std::string& symbol,
                                  double price,
                                  int qty,
                                  const std::string& exchange
                                ) {
    std::map<int, std::string> fields = {
        {35, "D"},          // MsgType
        {49, "TRADER1"},    // SenderCompID
        {56, exchange},     // TargetCompID  Exchange
        {11, ClOrdID},   // ClOrdID
        {55, symbol},       // Symbol
        {54, ((orderType=="Buy")?"1":"2")}, // Side (1=Buy)
        {38, std::to_string(qty)},        // OrderQty
        {40, "2"},          // OrdType (2=Limit)
        {44, std::to_string(price)},     // Price
        {59, "0"},           // TimeInForce (0=Day)
        {58, "NewOrderSingle"}
    };
    return constructFixMessage(fields);
}

/*
Key fields 
Order identification: ClOrdID (11), OrderID (37)
Execution details: ExecID (17), LastPx (31), LastQty (32)
Status information: OrdStatus (39), ExecType (150)
Quantities: CumQty (14), LeavesQty (151)
Timestamps: SendingTime (52)

*/

/*
std::string createExecutionReport() {
    std::map<int, std::string> fields = {
        {35, "8"},                 // MsgType (8=ExecutionReport)
        {49, "BROKER"},            // SenderCompID
        {56, "TRADER"},            // TargetCompID
        {6, "105.25"},             // AvgPx (average price)
        {11, "ORD123456"},         // ClOrdID (original client order ID)
        {14, "100"},               // CumQty (cumulative filled quantity)
        {17, "EXEC123"},           // ExecID (unique execution ID)
        {31, "105.25"},            // LastPx (last fill price)
        {32, "100"},               // LastQty (last fill quantity)
        {37, "BROKER_ORDER_ID"},   // OrderID (broker's order ID)
        {38, "100"},               // OrderQty (original quantity)
        {39, "2"},                 // OrdStatus (2=Filled)
        {40, "2"},                 // OrdType (2=Limit)
        {44, "105.00"},            // Price (limit price)
        {54, "1"},                 // Side (1=Buy)
        {55, "MSFT"},              // Symbol
        {59, "1"},                 // TimeInForce (1=GTC)
        {150, "2"},                // ExecType (2=Trade)
        {151, "0"}                 // LeavesQty (remaining quantity)
    };
    return constructFixMessage(fields);
}
*/

std::string createExecutionReport(std::map<int, std::string>& fields) {
    fields[35]="8";
    fields[56]="TRADINGSYSTEM"; // TargetCompID
    fields[40]="2";     // OrdType (2=Limit)
    return constructFixMessage(fields);
}

class IFixConnector {
public:
    virtual ~IFixConnector() = default;
    virtual void sendExecutionReport(const std::string& fixMessage) = 0;
    virtual void gotNewOrder(const std::string& orderType,
                    const std::string& symbol,
                    double price,
                    int quantity,
                    const std::string& destination) =0;
    
};

class FixConnector : public IFixConnector {
public:
    FixConnector() = default;

    void sendExecutionReport(const std::string& fixMessage) override;

    void gotNewOrder(const std::string& orderType,
                    const std::string& symbol,
                    double price,
                    int quantity,
                    const std::string& destination) override;
private:

};

void FixConnector::sendExecutionReport(const std::string& fixMessage) {
    cout << "FixConnector Execution Report " << fixMessage << " sent to client\n";
}

class FixEngine {
public:
    static FixEngine& getInstance() {
        static FixEngine instance;
        return instance;
    }

    void setConnector(IFixConnector* connector) {
        connector_ = connector;
    }

    void generateExecutionReport(const Trade& trade) {
        if (connector_) {
            std::string fixMsg = buildFixExecutionReport(trade);
            connector_->sendExecutionReport(fixMsg);
        }
    }
    static std::string makeClOrdID() {
        std::stringstream ss;
        ss << std::setw(5) << std::setfill('0') << ++nextClOrdID;
        return ss.str();
    } 
    static std::string makeExecID() {
        std::stringstream ss;
        ss << "EX"<< std::setw(5) << std::setfill('0') << ++nextRxecID;
        return ss.str();
    }
    IFixConnector* getClient() { 
        return connector_ ;
    }
    void sendFixOrder(const std::string fixMsg) {
        cout << "Got New Order fixMsg " << fixMsg << endl;
        //Parce
        auto fixData=parseFixMessage(fixMsg);
        std::string clOrderId=fixData[11];
        if (clOrderId.empty()) {
            cout << "No clOrderId in new Order fix message\n";
            return;
        }
        fixReport[clOrderId].push_back(std::move(fixMsg));
        OrderBookManager::getInstance().createOrder(
            ((fixData[54]=="1")? "Buy" : "Sell"),
            std::stod(fixData[44]),  //Price
            std::stoi(fixData[38]),  //qey
            fixData[56],             //exchange TargetCompID 
            SecurityManager::getInstance().getSecurity(fixData[55]),
            clOrderId
            );
    }
    
    void newOrderAck(const std::shared_ptr<Order>& theOrder) {
        //Acknowledgement
        /*
        35=8   Execution Report
        150=0  ExecType=0 (New)
        39=0 OrdStatus=0 (New)
        37=BROKER_ORDER_ID  Broker’s OrderID
        17=EXEC_ID_001  Unique execution ID
        11=clOrderId 
        14=0  Cumulative filled quantity (0, since not filled yet)
        151=100  Leaves quantity (remaining to fill) RemainingQuantity
        */
        std::map<int, std::string> fields = {        
            {150, "0"},     //ExecType
            {39, "0"},      // OrdStatus
            {17, FixEngine::makeExecID()},
            {37, theOrder->getOrderID()},
            {11, theOrder->getClOrdID()},
            {14, "0"},  // Cumulative filled quantity
            {151, std::to_string(theOrder->getRemainingQuantity())},
            {32, "0"},  //  Last executed quantity
            {31, "0" }, //   Last executed pric
            {49, theOrder->getDestinationExchange()}, //SenderCompID
            {55, theOrder->getSecuritySymbol()},
            {44, std::to_string(theOrder->getPrice())},
            {54, theOrder->getOrderSide()},     // Side (1=Buy)
            {38, std::to_string(theOrder->getQuantity())}, // OrderQty
            {59, "0"},   // Time in force (0 = Day)
            {58, "NewOrderAcknowledgment"}
        };   
        string fixM=createExecutionReport(fields);
        cout << "Execution Report Acknowledgment \n";
        //cout << "fixM " << fixM << endl;
        connector_->sendExecutionReport(fixM);
        fixReport[fields[11]].push_back(std::move(fixM));
        
    }
    void orderFilled(const std::shared_ptr<Order>& theOrder) {
        /*
        Execution Report (Full Fill, OrdStatus=2)
        39=150=2  OrdStatus=ExecType=2 (Fully filled)
        14=100 → Cumulative filled = order size
        151=0  Leaves quantity = 0
        */
            std::map<int, std::string> fields = {        
            {150, "2"},     //ExecType
            {39, "2"},      // OrdStatus
            {17, FixEngine::makeExecID()},
            {37, theOrder->getOrderID()},
            {11, theOrder->getClOrdID()},
            {14, std::to_string(theOrder->getQuantity())},  // Cumulative filled quantity
            {151, "0"},   // Leaves quantity
            {49, theOrder->getDestinationExchange()}, //SenderCompID
            {55, theOrder->getSecuritySymbol()},
            {44, std::to_string(theOrder->getPrice())},
            {54, theOrder->getOrderSide()},     // Side (1=Buy)
            {38, std::to_string(theOrder->getQuantity())}, // OrderQty
            {59, "0"},   // Time in force (0 = Day)
            {58, "OrderFullyFilled"}
        };   
        string fixM=createExecutionReport(fields);
        cout << "Execution Report Order Fully Filled\n";
        //cout << "fixM " << fixM << endl;
        connector_->sendExecutionReport(fixM);
        fixReport[fields[11]].push_back(std::move(fixM));
    }
    void orderPartialFill(const std::shared_ptr<Order>& theOrder) {
        /*
        150=1  ExecType=1 
        39=1  OrdStatus=1 (Partially filled)
        32=50  Last executed quantity
        31=250.50  Last executed price
        14=50  Cumulative filled quantity
        151=50  Remaining quantity
        */
            std::map<int, std::string> fields = {        
            {150, "1"},     //ExecType
            {39, "1"},      // OrdStatus
            {17, FixEngine::makeExecID()},
            {37, theOrder->getOrderID()},
            {11, theOrder->getClOrdID()},
            {14, std::to_string(theOrder->getQuantity()-theOrder->getRemainingQuantity())},  // Cumulative filled quantity
            {151, std::to_string(theOrder->getRemainingQuantity())},   // Leaves quantity
            {49, theOrder->getDestinationExchange()},
            {55, theOrder->getSecuritySymbol()},
            {44, std::to_string(theOrder->getPrice())},
            {54, theOrder->getOrderSide()},     // Side (1=Buy)
            {38, std::to_string(theOrder->getQuantity())}, // OrderQty
            {59, "0"},   // Time in force (0 = Day)
            {58, "OrderPartialFilled"}
        };
        auto lastTrade=theOrder->getLastTrade();
        if (lastTrade) {
            fields[31]=std::to_string(lastTrade->getPrice()); //Last executed price
            fields[32]=std::to_string(lastTrade->getQuantity()); // Last executed quantity
        }

        string fixM=createExecutionReport(fields);
        cout << "Execution Report Order Partially Filled\n";
        //cout << "fixM " << fixM << endl;
        connector_->sendExecutionReport(fixM);
        fixReport[fields[11]].push_back(std::move(fixM));
    }
    void printFixReport() {
        for (const auto& [clOrdID, records] : fixReport) {
            cout << "FixReport for ClOrderId " << clOrdID << ":\n";
            for (const auto& r : records) {
                cout << r << "\n";
            }
        }
    }
    
private:
    FixEngine() = default;
    IFixConnector* connector_ = nullptr; // non-owning
    static int nextClOrdID;
    static int nextRxecID;    
    std::unordered_map<std::string, vector<std::string>> fixReport;

    std::string buildFixExecutionReport(const Trade& trade); // internal
};

int FixEngine::nextClOrdID=0;
int FixEngine::nextRxecID=0;

void Router::processOrder(const std::shared_ptr<Order>& order) {
      if (order->isFixOrder()) {
          FixEngine::getInstance().newOrderAck(order);
      }
      OrderBookManager::getInstance().addOrder(order);

}

void OrderBook::processFilledOrders(std::shared_ptr<Order> buyOrder, std::shared_ptr<Order> sellOrder) {
            if (buyOrder->getRemainingQuantity() == 0) {
                buyOrder->setStatus(OrderStatus::FILLED);
                filledOrders.push_back(buyOrder);
                buyOrders.pop();
                if (buyOrder->isFixOrder()) {
                    FixEngine::getInstance().orderFilled(buyOrder);
                }
            } else {
                buyOrder->setStatus(OrderStatus::PARTIALLY_FILLED);
                if (buyOrder->isFixOrder()) {
                    FixEngine::getInstance().orderPartialFill(buyOrder);
                }
            }
            if (sellOrder->getRemainingQuantity() == 0) {
                sellOrder->setStatus(OrderStatus::FILLED);
                filledOrders.push_back(sellOrder);
                sellOrders.pop();
                if (sellOrder->isFixOrder()) {
                    FixEngine::getInstance().orderFilled(sellOrder);
                }
            } else {
                sellOrder->setStatus(OrderStatus::PARTIALLY_FILLED); 
                if (sellOrder->isFixOrder()) {
                    FixEngine::getInstance().orderPartialFill(sellOrder);
                }
            }   
}

void FixConnector::gotNewOrder(const std::string& orderType,
                    const std::string& symbol,
                    double price,
                    int quantity,
                    const std::string& destination) {
                        
    string fixD =createNewOrderSingle(orderType,
                                      FixEngine::makeClOrdID(),
                                      symbol,
                                      price,
                                      quantity,
                                      destination
                            );

    FixEngine::getInstance().sendFixOrder(fixD);
}



void testOrderBook() {
    
    auto& registry = ExchangeRegistry::getInstance();

    registry.createExchange("NASDAQ", 0.01, 570, 960);
    registry.createExchange("NYSE", 0.015, 570, 960);

    registry.printAll();
   
    // Create a Security object with exchange IDs
    auto& secManager = SecurityManager::getInstance();

    auto appl = secManager.createSecurity("AAPL", 190.0);
    appl->addExchangeID("NASDAQ", "AAPLQ");
    appl->addExchangeID("NYSE", "AAPLN");
    
    auto msft = secManager.createSecurity("MSFT", 170.0);
    msft->addExchangeID("NASDAQ", "MSFTQ");
    msft->addExchangeID("NYSE", "MSFTN");
    
    auto& router = Router::getInstance();

    router.addExchange(registry.getExchange("NASDAQ"));
    router.addExchange(registry.getExchange("NYSE"));  
    
    auto& orderBookManager = OrderBookManager::getInstance();
    
    orderBookManager.createOrder("Buy",195,300,"NASDAQ", appl);
    orderBookManager.createOrder("Sell",194,200,"NYSE", appl);
    orderBookManager.createOrder("Buy",200.0,3000,"NASDAQ", secManager.getSecurity("MSFT"));

    double startPrice=190.0; 
    for (int i=0; i<10; ++i) {
        orderBookManager.createOrder("Sell",startPrice,200,"NYSE", secManager.getSecurity("MSFT"));
        startPrice+=3.0;
    }
    orderBookManager.printOrderBook("MSFT");
    orderBookManager.printOrderBook("MSFT", false);
    cout << "====================================\n";
    
    FixConnector connector;
    
    FixEngine::getInstance().setConnector(&connector);
    (FixEngine::getInstance().getClient())->gotNewOrder("Buy", "AAPL", 200.00, 1000, "NASDAQ");
    (FixEngine::getInstance().getClient())->gotNewOrder("Sell", "AAPL", 200.00, 100, "NYSE");
    cout << "====================================\n";
    orderBookManager.printOrderBook("AAPL");
    FixEngine::getInstance().printFixReport();
    
/*
    std::cout << "\nOrder Book After Matching:\n";
    orderBook.printOrderBook();
*/

}


//////////////////////////////////////
int main()
{
    std::cout<<"Hello World\n";
    {    
    LRUCache cache(3);
    cache.put(1, 10);       // Cache: {1=10}
    cache.put(2, 2);       //  Cache: {2=2, 1=10}
    cache.put(9, 100);     //  Cache: {9,100, 2=2, 1=10}
    cache.printCache();
    std::cout << "cache.get(1) value " << cache.get(1)  << endl ; // Returns 1 → {1,10} {9,100} {2,2}
    cache.printCache();
    cout << "cache.put(3, 3);\n";
    cache.put(3, 3);       // Evicts key 2 → Cache: {3,3} {1,10} {9,100}
    cache.printCache();
    cout << cache.get(2) << endl;          // Returns -1 (not found)
    }
    {
        LRUCacheSet cache(5);
        cache.put(1, 10);       // timestamp 1 [1,10]
        cache.put(2, 2);       //  timestamp 2 [2,2]


        cache.put(9, 100);     //  timestamp 3 [9,100]
        cache.put(5,500);      //  timestamp 4 [5,500]
        cache.put(7,700);      //  timestamp 5 [7,700]
        cache.print();
        cache.put(8,800);
        cache.print();        
    }
    {
        cout << "Airline Company Class Design\n====================\n";
        
        // Get the current time
        std::chrono::system_clock::time_point now = std::chrono::system_clock::now();

        // Convert to a time_t for display purposes
        std::time_t now_gmt = std::chrono::system_clock::to_time_t(now);
        
        std::cout << "UTC Time: " << std::put_time(std::gmtime(&now_gmt), "%Y-%m-%d %H:%M:%S") << "\n";
        
        auto now_c = std::chrono::system_clock::to_time_t(now - std::chrono::hours(4));
        std::cout << "Local Time: " << std::put_time(std::localtime(&now_c), "%Y-%m-%d %H:%M:%S") << "\n";
        
        std::cout << "Current time: " << std::put_time(std::localtime(&now_c), "%Y-%m-%d %H:%M:%S") << "\n";
        
        AirlineCompany company;
        std::string f123="F123";
        company.addAirplane("A1", "Boeing 737", 150);
        company.addAirplane("A1", "Boeing 737", 150);
        company.addFlight(f123, "A1");
        
        std::vector<std::shared_ptr<CrewMember>> crew;
        crew.reserve(7);
        crew.emplace_back(std::make_shared<CrewMember>("C001", "Captain"));
        crew.emplace_back(std::make_shared<CrewMember>("C002", "FirstOfficer"));
        crew.emplace_back(std::make_shared<CrewMember>("C003", "FirstAttendant"));
        crew.emplace_back(std::make_shared<CrewMember>("C004", "SecondAttendant"));
        crew.emplace_back(std::make_shared<CrewMember>("C005", "FlightEngineer"));
        
        company.addCrewMemberToFlight(f123, std::move(crew));
        company.getCrew(f123);
        string pStr="P";
        for (int i=1 ; i<=20 ; ++i) {
            company.bookFlight(pStr+std::format("{:03d}", i), f123, i);
        }
        company.getPassengerList(f123);
        now = std::chrono::system_clock::now() - std::chrono::hours(4) ; //NY
        company.scheduleFlight(f123, now +std::chrono::hours(2),
                                     now +std::chrono::hours(5));
        company.scheduleFlight(f123, now +std::chrono::hours(3),
                                     now +std::chrono::hours(6));                             
                          
        auto pass=company.findPassengerByName("P001");
        if(pass) 
            pass->printPassenger();
            
        company.freeSeats(f123);
        company.bookFlight("John Doe",f123, 21);
        company.findPassengerByName("John Doe", true);
        company.switchSeat(f123,"John Doe", 24);
        company.switchSeat(f123,"John Doe", 24);
        company.switchSeat(f123,"John Doe", 21);
            
        std::cout << "Airline Company Setup Complete." << std::endl;
    }
    {
        cout << "Trading system current time " << getCurrentTimestamp() <<"\n";
        testOrderBook();
    }

    return 0;
}