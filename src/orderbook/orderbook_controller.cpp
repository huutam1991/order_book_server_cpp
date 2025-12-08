#include <orderbook/orderbook_controller.h>

void OrderBookController::initialize(const std::string& dbn_file_path)
{
    // For now hardcode order book parameters and DBN file path (but in reality should be from args)
    m_order_book = std::make_unique<OrderBook>(
        20000000LL,    // min price
        120000000000LL,   // max price
        10000000LL,        // tick = 10e6
        event_base
    );
    m_dbn_wrapper = std::make_unique<DbnWrapper>(dbn_file_path);

    m_dbn_file_path = dbn_file_path;
}

Task<void> OrderBookController::start_streaming(double speed)
{
    spdlog::info("Start streaming DBN file: {}, with speed={}", m_dbn_file_path, speed);

    m_dbn_wrapper->set_speed(speed);

    // Reset metrics
    apply_stats.clear();
    count_mbo_msgs = 0;

    auto task = m_dbn_wrapper->start_stream_data([this, m_order_book = m_order_book.get()](const databento::MboMsg& mbo_msg)
    {
        auto start = std::chrono::high_resolution_clock::now();

        m_order_book->apply(mbo_msg);

        auto end = std::chrono::high_resolution_clock::now();
        double us = std::chrono::duration<double, std::micro>(end - start).count();
        apply_stats.add_sample(us);

        count_mbo_msgs++;
    });
    task.start_running_on(event_base);

    co_return;
}

Task<void> OrderBookController::stop_streaming()
{
    if (m_dbn_wrapper && m_dbn_wrapper->get_is_streaming())
    {
        co_await m_dbn_wrapper->stop();
    }
    co_return;
}

Task<Json> OrderBookController::get_orderbook_snapshot()
{
    static LatencyTracker latency;

    // Start latency tracking
    auto t0 = std::chrono::steady_clock::now();

    if (m_order_book == nullptr)
    {
        co_return Json();
    }
    Json snapshot = co_await m_order_book->get_snapshot();

    // End latency tracking
    auto t1 = std::chrono::steady_clock::now();
    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    latency.add_sample(ms);

    snapshot["latency_get_snapshot"] = {
        {"p50", latency.p50()},
        {"p90", latency.p90()},
        {"p99", latency.p99()}
    };
    snapshot["latency_apply_mbo_msg"] = {
        {"p50", apply_stats.p50()},
        {"p90", apply_stats.p90()},
        {"p99", apply_stats.p99()},
        {"throughput_p50", 1000000.0 / apply_stats.p50()},
        {"throughput_p90", 1000000.0 / apply_stats.p90()},
        {"throughput_p99", 1000000.0 / apply_stats.p99()}
    };

    co_return snapshot;
}