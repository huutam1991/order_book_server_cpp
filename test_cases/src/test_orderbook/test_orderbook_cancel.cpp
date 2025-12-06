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

/**
 * Test 1: Cancel partial size decreases size correctly
 * but keeps the order in the book.
 */
TEST(OrderBookCancel, PartialCancel)
{
    OrderBook ob(0, 200000, 100);

    ob.apply(make_mbo(1, 'A', 'B', 100000, 10));

    // Cancel 4
    ob.apply(make_mbo(1, 'C', 'B', 100000, 4));

    auto bb = ob.best_bid();
    ASSERT_TRUE(bb.has_value());
    ASSERT_EQ(bb->first, 100000);
    ASSERT_EQ(bb->second, 6);  // 10 - 4
}

/**
 * Test 2: Full cancel removes the order entirely
 * leaving the level empty.
 */
TEST(OrderBookCancel, FullCancelRemovesOrder)
{
    OrderBook ob(0, 200000, 100);

    ob.apply(make_mbo(2, 'A', 'A', 101000, 5));

    // Cancel entire size
    ob.apply(make_mbo(2, 'C', 'A', 101000, 5));

    ASSERT_FALSE(ob.best_ask().has_value());
}

/**
 * Test 3: Cancel order among multiple orders at same price.
 * Only the target order is removed.
 */
TEST(OrderBookCancel, CancelAmongMultipleOrdersSamePrice)
{
    OrderBook ob(0, 200000, 100);

    ob.apply(make_mbo(3, 'A', 'B', 100000, 5));
    ob.apply(make_mbo(4, 'A', 'B', 100000, 7));

    // Cancel the second order
    ob.apply(make_mbo(4, 'C', 'B', 100000, 7));

    auto bb = ob.best_bid();
    ASSERT_TRUE(bb.has_value());
    ASSERT_EQ(bb->second, 5);
}

/**
 * Test 4: Cancel non-best order must NOT change best bid/ask.
 */
TEST(OrderBookCancel, CancelNonBestDoesNotChangeTop)
{
    OrderBook ob(0, 200000, 100);

    ob.apply(make_mbo(5, 'A', 'B', 101000, 4)); // best bid
    ob.apply(make_mbo(6, 'A', 'B', 100000, 8)); // lower bid

    // Cancel the lower price order
    ob.apply(make_mbo(6, 'C', 'B', 100000, 8));

    auto bb = ob.best_bid();
    ASSERT_TRUE(bb.has_value());
    ASSERT_EQ(bb->first, 101000);
    ASSERT_EQ(bb->second, 4);
}
