#include <filesystem>
#include <string>
#include <thread>
#include <chrono>

#include <spdlog/spdlog.h>
#include <utils/log_init.h>
#include <utils/utils.h>
#include <network/https_server/route/route_controller.h>
#include <system_io/https_server_io/https_server_socket.h>
#include <dbn_wrapper/dbn_wrapper.h>
#include <coroutine/event_base_manager.h>
#include <orderbook/orderbook_controller.h>

void init_api_endpoints()
{
    ADD_ROUTE(RequestMethod::POST, "/start_streaming_orderbook")
    {
        Json body_json = request->get_body_json();
        double speed = body_json.has_field("speed") ? (double)(body_json["speed"]) : 1.0;

        co_await OrderBookController::instance().stop_streaming();
        OrderBookController::instance().initialize("z_orderbook_data/CLX5_mbo.dbn");
        auto task = OrderBookController::instance().start_streaming(speed);
        task.start_running_on(EventBaseManager::get_event_base_by_id(EventBaseID::GATEWAY));

        Json response;
        response["status"] = "OK";
        response["message"] = "Started streaming orderbook data, with [speed] = " + std::to_string(speed);

        co_return HttpResponse(OK_200, response);
    };
}

int main(int argc, char **argv)
{
    // Get port from args
    const int port = atoi(argv[1]);

    // Init spdlog
    LogInit::init();

    // Init API endpoints
    init_api_endpoints();

    // Start HTTPS server - running on EpollBase
    EpollBase* epoll_base = (EpollBase*)EventBaseManager::get_event_base_by_id(EpollBaseID::SYSTEM_IO_TASK);
    HttpsServerSocket* https_server_object = new HttpsServerSocket(port);
    epoll_base->start_living_system_io_object(https_server_object);

    // Main loop, only sleep here
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1000));
    }

    spdlog::info("Main exit");

    return EXIT_SUCCESS;
}
