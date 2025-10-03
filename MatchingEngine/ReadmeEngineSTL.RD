Matching Engine (STL-based)
Overview
This project implements a limit order book matching engine using STL containers.
It features:
	Two engines:
1.	Engine → complex engine with random order generation, statuses, and reporting.
2.	MatchingEngine → simpler, minimal engine with a MessageHub for event handling.
	Order lifecycle management (New, PartiallyFilled, Filled, Cancelled, Rejected).
	FIFO ordering at each price level, with priority and timestamp tie-breaking.
	Efficient lookups using maps of price → list of orders and a fast orderIndex_.
	Event notification (Fill, Cancel, Accepted) via MessageHub.
________________________________________
Core Components
Enums
	Side → Buy/Sell.
	Status → lifecycle (New, Filled, Cancelled, etc.).
	OrderType → action (Place, Pick, Cancel, Replace).
	EventType → event (Accepted, Fill, Cancel, Reject).
	ProcessingAction → outcome flags (Dropped, Matched, etc.).
________________________________________
Order
Represents a rich order with:
	id, symbol, side, price, qty, remainingQty.
	Matching metadata: averagePrice, priority, timestamp, status, type.
	Method: printOrder() for debugging.
________________________________________
OrderBook
Per-symbol order book with:
	Buy side → std::map<double, std::list<Order>, std::greater<double>> (best = highest).
	Sell side → std::map<double, std::list<Order>> (best = lowest).
	Index → orderId → iterator for O(1) lookup/cancel.
Key methods:
	processOrder(Order&) → entry point, calls matchOrders.
	matchOrders → matches incoming order against opposite side.
	addOrder(Order&&) → inserts into proper price level, ordered by priority/timestamp.
	cancelOrder(Order&) → cancels an order by ID.
________________________________________
Engine
High-level simulator:
	Generates random test orders (generateOrder).
	Processes orders in FIFO queue (processOrders).
	Dispatches fills via dispatchOrderBookEvent.
	Maintains state of all orders (allOrders_, readyForReport).
	Produces final report (report).
Example Flow
Engine → OrderBook → Orders
	Engine creates and feeds orders.
	OrderBook manages matching and cancelation.
	Orders are stored in FIFO per price level with priority.

________________________________________
Simplified Structures
	SimpleOrder, SimpleOrderBookEvent → minimal lightweight version.
	Fill, Cancel, OrderAck → wrappers around events.
________________________________________
MessageHub & SystemMessageHub
Observer/notification system:
	Virtual base MessageHub with SendFill, SendCancel, SendOrderAck.
	Concrete SystemMessageHub implementation prints to console.
________________________________________
MatchingEngine (Simple)
Minimal matching engine:
	Works with SimpleOrder.
	Uses buyLevels_, sellLevels_ (map<price, list<SimpleOrder>>).
	Maintains orderIndex_ for lookup.
	Methods:
	submit_new_order → add & try to match.
	processOrder → core matching logic.
	addOrder(SimpleOrder&&) → add to order book.
	cancel_existing_order → cancel order.
	dipatch_result → calls MessageHub to notify events.

Notes:
Engine is a more realistic simulator with random input.
MatchingEngine is a minimal core engine suitable for extension.
STL containers (map, list, unordered_map) provide deterministic ordering and fast lookup.
Observer pattern (MessageHub) makes it easy to integrate with external systems.
