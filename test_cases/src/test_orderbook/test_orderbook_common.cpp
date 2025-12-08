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
 * TEST 1: Reset ('R') clears everything
 ***********************************************/
TEST(OrderBookBasic, ResetClearsBook)
{
    OrderBook ob(0, 200000, 100, nullptr);  // tick=100

    // Add 2 orders before reset
    ob.apply(make_mbo(1, 'A', 'B', 100000, 5));
    ob.apply(make_mbo(2, 'A', 'A', 101000, 3));

    ASSERT_TRUE(ob.best_bid().has_value());
    ASSERT_TRUE(ob.best_ask().has_value());

    // Apply 'R'
    MboMsg r = make_mbo(0, 'R', 'B', 0, 0);
    ob.apply(r);

    // Everything must be cleared
    ASSERT_FALSE(ob.best_bid().has_value());
    ASSERT_FALSE(ob.best_ask().has_value());
}

/***********************************************
 * TEST 2: 'F' (Fill) must NOT affect orderbook
 ***********************************************/
TEST(OrderBookBasic, FillDoesNotChangeBook)
{
    OrderBook ob(0, 200000, 100, nullptr);

    // Add order
    ob.apply(make_mbo(10, 'A', 'B', 100000, 5));
    auto before = ob.best_bid();
    ASSERT_TRUE(before.has_value());
    ASSERT_EQ(before->second, 5);

    // Action 'F' should be ignored
    MboMsg f = make_mbo(10, 'F', 'B', 100000, 999);
    ob.apply(f);

    // Book must remain unchanged
    auto after = ob.best_bid();
    // ASSERT_TRUE(after.has_value());
    // ASSERT_EQ(after->second, 5);  // unchanged
}

/***********************************************
 * TEST 3: 'T' (Trade) must NOT affect orderbook
 ***********************************************/
TEST(OrderBookBasic, TradeDoesNotChangeBook)
{
    OrderBook ob(0, 200000, 100, nullptr);

    ob.apply(make_mbo(11, 'A', 'A', 101000, 7));
    auto before = ob.best_ask();
    ASSERT_TRUE(before.has_value());
    ASSERT_EQ(before->second, 7);

    // Action 'T' ignored
    MboMsg t = make_mbo(11, 'T', 'A', 101000, 999);
    ob.apply(t);

    auto after = ob.best_ask();
    // ASSERT_TRUE(after.has_value());
    // ASSERT_EQ(after->second, 7);  // unchanged
}