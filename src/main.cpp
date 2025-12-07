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
    ADD_ROUTE(RequestMethod::GET, "/")
    {
        co_return request->send_file_from_directory("/index.html");
    };

    ADD_ROUTE(RequestMethod::GET, "/get_snapshot")
    {
        Json response;
        response["status"] = "OK";
        response["snapshot"] = OrderBookController::instance().get_orderbook_snapshot();

        co_return HttpResponse(OK_200, response);
    };

    ADD_ROUTE(RequestMethod::POST, "/start_streaming_orderbook")
    {
        // Get speed from request body
        Json body_json = request->get_body_json();
        double speed = body_json.has_field("speed") ? (double)(body_json["speed"]) : 1.0;

        // Stop first if it's already streaming
        co_await OrderBookController::instance().stop_streaming();

        // Init with DBN file path (hardcode for now)
        OrderBookController::instance().initialize("z_orderbook_data/CLX5_mbo.dbn");

        // Start streaming orderbook data
        auto task = OrderBookController::instance().start_streaming(speed);
        task.start_running_on(EventBaseManager::get_event_base_by_id(EventBaseID::GATEWAY));

        Json response;
        response["status"] = "OK";
        response["message"] = "Started streaming orderbook data, with [speed] = " + std::to_string(speed);

        co_return HttpResponse(OK_200, response);
    };

    ADD_ROUTE(RequestMethod::GET, "/stop_streaming_orderbook")
    {
        co_await OrderBookController::instance().stop_streaming();

        Json response;
        response["status"] = "OK";
        response["message"] = "Stopped streaming orderbook data";

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
