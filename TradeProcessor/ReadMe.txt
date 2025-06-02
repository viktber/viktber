# Multithreaded Trading Simulator

##  Overview

This project is a fully functional event-driven trading simulator in C++ using modern concurrency constructs.

It simulates multiple exchange adapters (like NYSE, NASDAQ), each generating trading and market data events, processed by consumer threads. A central `OrderBook` matches buy/sell orders in real-time.

---

##  Architecture

### Core Components

- **ExchangeAdapter**  
  Produces `TradeEvent`s (e.g., `NewOrder`, `CancelOrder`) in a dedicated thread.

- **MarketDataAdapter**  
  Produces `MDataEvent`s (e.g., `MarketDataUpdate`) in a dedicated thread.

- **ThreadSafeQueue**  
  Mutex-based thread-safe queue used to pass events between producers and consumers.

- **TradeProcessor**  
  Consumes `TradeEvent`s, routes them to the `OrderBook`.

- **MarketDataProcessor**  
  Consumes `MDataEvent`s and prints processed data.

- **OrderBook**  
  Maintains live buy/sell orders, supports add/modify/cancel/match.

- **TradeProcessorManager**  
  Owns both processors and manages lifecycle.

- **AdapterManager**  
  Manages all adapters (start, stop, restart, status).

---

## How It Works

- Start `TradeProcessorManager`, passing `TradeEvent` and `MarketDataEvent` queues.
- Register and start `ExchangeAdapter` (e.g., NYSE).
- Each adapter pushes events into the queues.
- Processors consume events, and the order book is updated accordingly.
- `StopSignal` is pushed to gracefully stop all threads.

---

## Event Types

### `TradeEvent` (variant)
- `NewOrder`
- `CancelOrder`
- `ModifyOrder`
- `TradeConfirmation`
- `RejectedOrder`
- `OrderBookUpdate`
- `SessionStatus`
- `Heartbeat`
- `StopSignal`

### `MDataEvent` (variant)
- `MarketDataUpdate`
- `StopSignal`

---

##  Thread Model

| Component             | Thread? |
|----------------------|---------|
| ExchangeAdapter       |  Yes |
| MarketDataAdapter     |  Yes |
| TradeProcessor        |  Yes |
| MarketDataProcessor   |  Yes |
| Main Thread           |  Yes |

All event queues are thread-safe and support graceful shutdown via `StopSignal`.

