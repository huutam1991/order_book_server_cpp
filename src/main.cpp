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
        60000000000LL,   // min price
        70000000000LL,   // max price
        10000000LL       // tick = 10e6
    );

    // Initialize DBN Wrapper with file path
    DbnWrapper dbn_engine("z_orderbook_data/CLX5_mbo.dbn");
    dbn_engine.set_speed(0.01); // 0.01x speed

    int count = 0;

    auto task = dbn_engine.start_stream_data([&ob, &count](const databento::MboMsg& mbo_msg)
    {
        // spdlog::info("Received MBO message: order_id={}, price={}, size={}, delta_ns={}",
        //                 mbo_msg.order_id,
        //                 mbo_msg.price,
        //                 mbo_msg.size,
        //                 mbo_msg.ts_in_delta.count());

        ob.apply(mbo_msg);

        count++;
        if (count % 1000 == 0)
        {
            OrderBook::DepthSnapshot depth = ob.get_depth(10);
            spdlog::info("Depth at level 10:");

            // Print ask side
            if (depth.asks.size() > 0)
            {
                for (size_t i = 0; i < depth.asks.size(); i++)
                {
                    spdlog::info("  Ask - Price: {}, Size: {}", depth.asks[i].price, depth.asks[i].size);
                }
            }
            else
            {
                spdlog::info("  Ask - None");
            }

            // Print bid side
            if (depth.bids.size() > 0)
            {
                for (size_t i = 0; i < depth.bids.size(); i++)
                {
                    spdlog::info("  Bid - Price: {}, Size: {}", depth.bids[i].price, depth.bids[i].size);
                }
            }
            else
            {
                spdlog::info("  Bid - None");
            }
        }
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
