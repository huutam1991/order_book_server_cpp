#pragma once

#include <memory>

#include <utils/utils.h>

#include <orderbook/orderbook.h>
#include <dbn_wrapper/dbn_wrapper.h>
#include <utils/latency_tracker.h>

class OrderBookController
{
    Singleton(OrderBookController)

private:
    std::unique_ptr<OrderBook> m_order_book;
    std::unique_ptr<DbnWrapper> m_dbn_wrapper;

    // File path
    std::string m_dbn_file_path;

    // Measure apply stats
    LatencyTracker apply_stats;
    int count_mbo_msgs = 0;

public:
    void initialize(const std::string& dbn_file_path);
    Task<void> stop_streaming();
    Task<void> start_streaming(double speed = 1.0);

    // Get current order book snapshot
    Json get_orderbook_snapshot();
};