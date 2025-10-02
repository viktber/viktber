Matching Engine Project
Overview

This project implements a limit order book matching engine with:
Buy/Sell order management
FIFO queue per price level
Efficient price lookup using fixed array levels
Custom memory pool (OrderPool) for fast allocation
Event dispatching via MessageHub (observer pattern)
It supports:
Submitting new buy/sell orders
Matching incoming orders against the opposite book
Canceling orders by ID
Querying best bid/ask
Dispatching order events (Accepted, Fill, Cancel, Reject)

Class Overview
Order
Represents a single order.
Fields: id, side, price, qty, symbol(not in use)
Intrusive doubly-linked list via next and prev
Methods:
is_active() → true if qty > 0
printOrder() → debug printing

PriceLevel linked list
Represents all orders at a given price, stored in FIFO order.
Maintains head/tail linked list of Orders
Methods:
push_back(Order*) → add new order (time priority preserved)
remove(Order*) → remove arbitrary order
head() / tail() accessors
empty(), size(), getPrice()

OrderPool
Custom allocator for orders with fixed capacity.
Stores pre-allocated Order objects
Recycles freed orders using free list (LIFO)
Methods:
allocate() → get free or fresh Order
reclaim(Order*) → return order back to pool
empty(), full(), size()

FixedBookSide
Represents one side of the book (bids or asks).
Holds all price levels in a fixed array
Hash map for orderId → (Order, level index)*
Methods:
insert_order(Order*) → add to correct price level
cancel_order(OrderId) → remove order
best_price() → return best bid/ask
getOrder(OrderId) → lookup order
getPriceLevel(idx) → access by index

OrderBookEvent
Lightweight event struct.
Types: Accepted, Fill, Cancel, Reject
Fields: orderId, qty, price
Method: printEvent() → log event

MessageHub
Event dispatcher (Observer pattern).
Allows subscribers per event type
Default subscribers:
logger → prints events
sendMsg → simulates sending message
Methods:
subscribe(EventType, callback)
notify(OrderBookEvent)

MatchingEngine
The core engine.
Holds bids (FixedBookSide) and asks (FixedBookSide)
Owns an OrderPool
Event dispatch through MessageHub
Methods:
submit_new_order(side, price, qty) → create + match order
add_order(Order*) → insert or match
cancel_order(OrderId) → cancel by id
best_bid(), best_ask()
Internal: try_match() → matching logic
Internal: dispatch_book_event() → send events
Internal: printBook() print bid /ask books

