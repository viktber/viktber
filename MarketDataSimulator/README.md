Simulated Trading Engine (C++23)
This project is a multithreaded simulated trading system implemented in modern C++23. 
It features coroutine-driven market data generation, a thread-safe event queue, 
strategy-based trade signal processing, and an asynchronous logger.

Features
Market Data Feed using C++23 coroutines (std::generator)
Thread-safe event queue (ThreadSafeQueue)
TradeProcessor with plugin-based strategies
Trading strategies:
Narrow Spread Strategy
Price Breakout Strategy
Moving Average Crossover Strategy
Asynchronous Logger

Technologies Used
C++23
std::generator, std::variant, std::thread, std::atomic
STL containers and algorithms
RAII and Singleton patterns

Components
Market Data Feed
Simulates quotes using a coroutine
Produces MarketDataUpdate events
Emits a StopSignal to terminate the stream
ThreadSafeQueue
Facilitates communication between producer (MarketData) and consumer (TradeProcessor)
Thread-safe push and pop operations
TradeProcessor
Singleton class that consumes events
Maintains a map of best quotes (QuoteBook)
Evaluates strategies per quote
Strategies
IStrategy base class
NarrowSpreadStrategy: triggers when spread is narrow
PriceBreakoutStrategy: triggers on new highs/lows
MovingAvCStrategy: triggers on moving average crossovers

Logger
Singleton async logger
Logs include timestamp, thread ID, log level, and message
Uses std::atomic_flag to serialize output
