#pragma once

#include <memory>

#include <utils/utils.h>

#include <orderbook/orderbook.h>
#include <dbn_wrapper/dbn_wrapper.h>

class OrderBookController
{
    Singleton(OrderBookController)

private:
    std::unique_ptr<OrderBook> m_order_book;
    std::unique_ptr<DbnWrapper> m_dbn_wrapper;

public:
    void initialize(const std::string& dbn_file_path);
    void start_streaming(double speed = 1.0);
    void stop_streaming();
};