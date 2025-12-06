#include <filesystem>
#include <string>
#include <thread>
#include <chrono>

#include <spdlog/spdlog.h>
#include <utils/log_init.h>
#include <dbn_wrapper/dbn_wrapper.h>
#include <coroutine/event_base_manager.h>

int main(int argc, char **argv)
{
    // Init spdlog
    LogInit::init();

    // Initialize DBN Wrapper with file path
    DbnWrapper dbn_engine("z_orderbook_data/CLX5_mbo.dbn");
    dbn_engine.set_speed(0.01); // 0.01x speed

    auto task = dbn_engine.start_stream_data(
    [](const databento::MboMsg& mbo_msg)
    {
        spdlog::info("Received MBO message: order_id={}, price={}, size={}, delta_ns={}",
                        mbo_msg.order_id,
                        mbo_msg.price,
                        mbo_msg.size,
                        mbo_msg.ts_in_delta.count());
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
