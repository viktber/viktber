# Coroutine Projects in C++20

This project demonstrates two coroutine-based systems written in C++20:

1. Producer-Consumer Queue using coroutines  
2. MarketDataUpdate pipeline for real-time data processing

Each section below describes one project, its purpose, key classes, and how they interact.

---

##  Project 1: Coroutine-Based Producer-Consumer Queue

### Overview

Implements an asynchronous bounded queue that supports coroutine-based producers and consumers. 
Instead of blocking threads, producers and consumers are suspended using `co_await` and resumed when conditions are met.

###  Key Classes and Relationships

#### `CoroutineQueue<T>`
- Central data structure.
- Holds:
  - `buffer_`: actual item queue
  - `prodWaiters_`: queue of suspended producers (waiting for space)
  - `consWaiters_`: queue of suspended consumers (waiting for data)
- Methods:
  - `push(value)`: `co_await` suspends if full, resumes when space available.
  - `pop()`: `co_await` suspends if empty, resumes when item is available.

#### `Task<T>`
- A coroutine wrapper used to define `Producer` and `Consumer` tasks.
- Manages suspension/resumption logic using `std::coroutine_handle`.

#### `makeProducer(...)`, `makeConsumer(...)`
- Factory functions that spawn producer/consumer coroutines.
- Interact with a shared `CoroutineQueue<T>`.

#### Relationship Diagram 
Producer/Consumer <--> CoroutineQueue<T> <--> Task<T>


---

## Project 2: MarketDataUpdate Coroutine Pipeline

###  Overview

Simulates streaming market data. Each symbol gets continuous quote updates, which are processed in real-time by a coroutine-based `Processor`. 
Averaging and cleanup are performed periodically via coroutine timers.

###  Key Classes and Relationships

#### `MarketDataUpdate`
- Data structure representing a market quote.
- Fields: `symbol`, `lastPrice`, `bidPrice`, `askPrice`, `bidSize`, `askSize`, `timestamp`
- Method: `quoteToStr()` formats the data.

#### `TimerAwaitable`
- Custom awaitable for `co_await`-based time delays.
- Resumes coroutine after given duration using a detached thread.

#### `Info`
- Singleton managing:
  - List of symbols
  - Data generation using `std::mt19937`
  - Quote publishing into the queue

#### `Processor`
- Consumes `MarketDataUpdate` from the queue.
- Maintains:
  - `symbolMap_`: recent updates (max 50) per symbol
  - `timestampIndex_`: timestamp → iterator mapping
  - `symbolMutex_`: per-symbol mutex for safe access
- Runs two background loops:
  - Averaging every 2 seconds
  - Cleanup every 6 seconds

#### `CoroutineManager<T>`
- Spawns producers and consumers.
- Keeps a registry (`producers_`, `consumers_`) with optional tagging.
- Responsible for lifecycle and trace logging.

#### Relationship Diagram 
Info --> CoroutineQueue<MarketDataUpdate> --> Processor
^ |
| v
CoroutineManager<T> <--- manages ---> Coroutines

##  C++20 Features Highlighted

- Coroutines: `co_await`, `co_return`, `co_yield`
- `std::coroutine_handle`, `promise_type`
- Custom awaitables
- `std::jthread`, `std::stop_token` for cancellation
- Thread-safe structures with fine-grained locking

