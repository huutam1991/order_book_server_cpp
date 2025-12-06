#include <filesystem>
#include <string>
#include <thread>
#include <chrono>

#include <spdlog/spdlog.h>
#include <utils/log_init.h>

#include <databento/dbn.hpp>
#include <databento/dbn_file_store.hpp>

int main(int argc, char **argv)
{
    // Init spdlog
    LogInit::init();

    spdlog::info("Databento test app started.");

    databento::DbnFileStore store{"CLX5_mbo.dbn"};
    int count = 0;

    while (const auto* rec = store.NextRecord())
    {
        const auto* mbo = rec->GetIf<databento::MboMsg>();
        if (mbo != nullptr)
        {
            spdlog::info("Processed MBO record {}: instrument_id={}, price={}, size={}, action={}, side={}, ts_recv={}",
                        count,
                        mbo->hd.instrument_id,
                        mbo->price,
                        mbo->size,
                        static_cast<char>(mbo->action),
                        static_cast<char>(mbo->side),
                        mbo->ts_recv.time_since_epoch().count());

            count++;
        }
    }

    spdlog::info("Total MBO records processed: {}", count);

    auto metadata = store.GetMetadata();

    for (const auto& inst : metadata.mappings)
    {
        spdlog::info(" => {}", inst.raw_symbol);
    }


    // Main loop, only sleep here
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1000));
    }

    spdlog::info("Main exit");

    return EXIT_SUCCESS;
}
