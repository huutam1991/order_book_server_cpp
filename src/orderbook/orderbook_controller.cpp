#include <orderbook/orderbook_controller.h>
#include <utils/latency_tracker.h>

void OrderBookController::initialize(const std::string& dbn_file_path)
{
    // For now hardcode order book parameters and DBN file path (but in reality should be from args)
    m_order_book = std::make_unique<OrderBook>(
        10000000000LL,    // min price
        120000000000LL,   // max price
        10000000LL        // tick = 10e6
    );
    m_dbn_wrapper = std::make_unique<DbnWrapper>(dbn_file_path);

    m_dbn_file_path = dbn_file_path;
}

Task<void> OrderBookController::start_streaming(double speed)
{
    spdlog::info("Start streaming DBN file: {}, with speed={}", m_dbn_file_path, speed);

    m_dbn_wrapper->set_speed(speed);

    auto task = m_dbn_wrapper->start_stream_data([m_order_book = m_order_book.get()](const databento::MboMsg& mbo_msg)
    {
        // spdlog::info("Received MBO message: order_id={}, price={}, size={}, delta_ns={}",
        //                 mbo_msg.order_id,
        //                 mbo_msg.price,
        //                 mbo_msg.size,
        //                 mbo_msg.ts_in_delta.count());

        m_order_book->apply(mbo_msg);
    });
    task.start_running_on(EventBaseManager::get_event_base_by_id(EventBaseID::GATEWAY));

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

Json OrderBookController::get_orderbook_snapshot()
{
    static LatencyTracker latency;

    // Start latency tracking
    auto t0 = std::chrono::steady_clock::now();

    if (m_order_book == nullptr)
    {
        return Json();
    }
    Json snapshot = m_order_book->get_snapshot();

    // End latency tracking
    auto t1 = std::chrono::steady_clock::now();
    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    latency.add_sample(ms);

    snapshot["latency"] = {
        {"p50", latency.p50()},
        {"p90", latency.p90()},
        {"p99", latency.p99()}
    };

    return snapshot;
}