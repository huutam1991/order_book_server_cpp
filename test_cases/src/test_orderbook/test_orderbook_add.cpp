#include <gtest/gtest.h>
#include <orderbook/orderbook.h>

using databento::MboMsg;
using databento::RecordHeader;

static MboMsg make_mbo(uint64_t order_id, char action, char side, int64_t px, uint32_t sz)
{
    MboMsg m{};
    m.hd.instrument_id = 1;      // dummy
    m.order_id = order_id;
    m.price = px;
    m.size = sz;
    m.action = (databento::Action)action;
    m.side = (databento::Side)side;
    return m;
}

/***********************************************
 * ADD TEST 1:
 * Add first BID order → best bid must match
 ***********************************************/
TEST(OrderBookAdd, AddFirstBid)
{
    OrderBook ob(0, 200000, 100, nullptr);

    ob.apply(make_mbo(1, 'A', 'B', 100000, 5));  // price = 1000.00

    auto bb = ob.best_bid();
    ASSERT_TRUE(bb.has_value());
    ASSERT_EQ(bb->first, 100000);
    ASSERT_EQ(bb->second, 5);
}

/***********************************************
 * ADD TEST 2:
 * Add first ASK order → best ask must match
 ***********************************************/
TEST(OrderBookAdd, AddFirstAsk)
{
    OrderBook ob(0, 200000, 100, nullptr);

    ob.apply(make_mbo(2, 'A', 'A', 101000, 3));

    auto ba = ob.best_ask();
    ASSERT_TRUE(ba.has_value());
    ASSERT_EQ(ba->first, 101000);
    ASSERT_EQ(ba->second, 3);
}

/***********************************************
 * ADD TEST 3:
 * Add multiple orders at same price → total_size
 * and FIFO count must match
 ***********************************************/
TEST(OrderBookAdd, AddMultipleSamePriceFIFO)
{
    OrderBook ob(0, 200000, 100, nullptr);

    ob.apply(make_mbo(10, 'A', 'B', 100000, 5));
    ob.apply(make_mbo(11, 'A', 'B', 100000, 7));
    ob.apply(make_mbo(12, 'A', 'B', 100000, 2));

    auto bb = ob.best_bid();
    ASSERT_TRUE(bb.has_value());
    ASSERT_EQ(bb->first, 100000);
    ASSERT_EQ(bb->second, 5 + 7 + 2);  // total size = 14

    // Verify FIFO order count
    auto idx = ob.price_to_index(100000);
    auto& level = ob.get_level(true, idx);
    ASSERT_EQ(level.queue.size(), 3);
}

/***********************************************
 * ADD TEST 4:
 * Add orders at different prices → ensure
 * best bid/ask updates correctly
 ***********************************************/
TEST(OrderBookAdd, AddDifferentPrices)
{
    OrderBook ob(0, 200000, 100, nullptr);

    // Lower bid
    ob.apply(make_mbo(20, 'A', 'B', 99000, 4));

    auto bb1 = ob.best_bid();
    ASSERT_TRUE(bb1.has_value());
    ASSERT_EQ(bb1->first, 99000);
    ASSERT_EQ(bb1->second, 4);

    // Higher bid → must become new best bid
    ob.apply(make_mbo(21, 'A', 'B', 101000, 6));

    auto bb2 = ob.best_bid();
    ASSERT_TRUE(bb2.has_value());
    ASSERT_EQ(bb2->first, 101000);
    ASSERT_EQ(bb2->second, 6);
}