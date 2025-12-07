#include <orderbook/orderbook_controller.h>

void OrderBookController::initialize(const std::string& dbn_file_path)
{
    // Stop existing streaming if any
    if (m_dbn_wrapper && m_dbn_wrapper->get_is_streaming())
    {
        m_dbn_wrapper->stop();
    }

    m_order_book = std::make_unique<OrderBook>(
        10000000000LL,    // min price
        120000000000LL,   // max price
        10000000LL        // tick = 10e6
    );
    m_dbn_wrapper = std::make_unique<DbnWrapper>(dbn_file_path);
}

void OrderBookController::start_streaming(double speed)
{
    m_dbn_wrapper->set_speed(speed);
    m_dbn_wrapper->set_end_callback([m_order_book = m_order_book.get()]()
    {
        spdlog::info("DBN stream ended");
    });

    auto task = m_dbn_wrapper->start_stream_data([m_order_book = m_order_book.get()](const databento::MboMsg& mbo_msg)
    {
        spdlog::info("Received MBO message: order_id={}, price={}, size={}, delta_ns={}",
                        mbo_msg.order_id,
                        mbo_msg.price,
                        mbo_msg.size,
                        mbo_msg.ts_in_delta.count());

        m_order_book->apply(mbo_msg);
    });

    task.start_running_on(EventBaseManager::get_event_base_by_id(EventBaseID::GATEWAY));
}

void OrderBookController::stop_streaming()
{
    if (m_dbn_wrapper && m_dbn_wrapper->get_is_streaming())
    {
        m_dbn_wrapper->stop();
    }
}