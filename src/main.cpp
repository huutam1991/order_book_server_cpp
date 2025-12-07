#include <filesystem>
#include <string>
#include <thread>
#include <chrono>

#include <spdlog/spdlog.h>
#include <utils/log_init.h>
#include <dbn_wrapper/dbn_wrapper.h>
#include <coroutine/event_base_manager.h>
#include <orderbook/orderbook.h>

int main(int argc, char **argv)
{
    // Init spdlog
    LogInit::init();

    // Create OrderBook instance
    OrderBook ob(
        10000000000LL,    // min price
        120000000000LL,   // max price
        10000000LL        // tick = 10e6
    );

    // Initialize DBN Wrapper with file path
    DbnWrapper dbn_engine("z_orderbook_data/CLX5_mbo.dbn");
    dbn_engine.set_speed(1); // 0.01x speed
    dbn_engine.set_end_callback([&ob]()
    {
        Json snapshot = ob.get_snapshot();
        spdlog::info("DBN stream ended. Final Order Book Snapshot: {}", snapshot);
    });

    auto task = dbn_engine.start_stream_data([&ob](const databento::MboMsg& mbo_msg)
    {
        // spdlog::info("Received MBO message: order_id={}, price={}, size={}, delta_ns={}",
        //                 mbo_msg.order_id,
        //                 mbo_msg.price,
        //                 mbo_msg.size,
        //                 mbo_msg.ts_in_delta.count());

        ob.apply(mbo_msg);
    });

    task.start_running_on(EventBaseManager::get_event_base_by_id(EventBaseID::GATEWAY));


    // Main loop, only sleep here
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1000));
    }

    spdlog::info("Main exit");

    return EXIT_SUCCESS;
}
