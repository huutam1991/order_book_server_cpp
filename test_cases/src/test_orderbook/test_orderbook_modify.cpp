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

/*******************************************************
 *  MODIFY TESTS (8 tests)
 *******************************************************/

/**
 * Test 1: Modify size decrease — priority is kept.
 */
TEST(OrderBookModify, SizeDecreaseKeepsPriority)
{
    OrderBook ob(0, 200000, 100);

    ob.apply(make_mbo(10, 'A', 'B', 100000, 10));

    // Decrease size only
    ob.apply(make_mbo(10, 'M', 'B', 100000, 6));

    auto bb = ob.best_bid();
    ASSERT_TRUE(bb.has_value());
    ASSERT_EQ(bb->second, 6);
}

/**
 * Test 2: Modify size increase — order must lose priority
 * and move to the back of FIFO queue.
 */
TEST(OrderBookModify, SizeIncreaseLosesPriority)
{
    OrderBook ob(0, 200000, 100);

    ob.apply(make_mbo(10, 'A', 'B', 100000, 5));
    ob.apply(make_mbo(11, 'A', 'B', 100000, 7));

    // Increase size of first order => move to back
    ob.apply(make_mbo(10, 'M', 'B', 100000, 8));

    size_t idx = ob.price_to_index(100000);
    auto& level = ob.get_level(true, idx);

    ASSERT_EQ(level.queue.back().order_id, 10);
}

/**
 * Test 3: Modify price increase — order moves to new price level.
 */
TEST(OrderBookModify, PriceIncreaseMovesOrder)
{
    OrderBook ob(0, 200000, 100);

    ob.apply(make_mbo(20, 'A', 'B', 100000, 5));

    // Move price upward
    ob.apply(make_mbo(20, 'M', 'B', 101000, 5));

    auto bb = ob.best_bid();
    ASSERT_TRUE(bb.has_value());
    ASSERT_EQ(bb->first, 101000);
}

/**
 * Test 4: Modify price decrease — order moves to lower level.
 */
TEST(OrderBookModify, PriceDecreaseMovesOrder)
{
    OrderBook ob(0, 200000, 100);

    ob.apply(make_mbo(21, 'A', 'B', 102000, 5));

    // Move down
    ob.apply(make_mbo(21, 'M', 'B', 100000, 5));

    auto bb = ob.best_bid();
    ASSERT_TRUE(bb.has_value());
    ASSERT_EQ(bb->first, 100000);
}

/**
 * Test 5: Modify on missing order should behave like an Add.
 */
TEST(OrderBookModify, ModifyOnMissingOrderBecomesAdd)
{
    OrderBook ob(0, 200000, 100);

    // Modify an order that doesn't exist
    ob.apply(make_mbo(30, 'M', 'A', 101000, 4));

    auto ba = ob.best_ask();
    ASSERT_TRUE(ba.has_value());
    ASSERT_EQ(ba->first, 101000);
    ASSERT_EQ(ba->second, 4);
}

/**
 * Test 6: Modify size to zero — order must be removed.
 */
TEST(OrderBookModify, ModifySizeToZeroErasesOrder)
{
    OrderBook ob(0, 200000, 100);

    ob.apply(make_mbo(31, 'A', 'B', 100000, 5));

    // Size becomes 0 => remove
    ob.apply(make_mbo(31, 'M', 'B', 100000, 0));

    ASSERT_FALSE(ob.best_bid().has_value());
}

/**
 * Test 7: Complex sequence — Add → increase size → change price.
 */
TEST(OrderBookModify, ComplexModifySequence)
{
    OrderBook ob(0, 200000, 100);

    ob.apply(make_mbo(40, 'A', 'B', 100000, 5));

    // Size increase => move to back (only one order so no effect)
    ob.apply(make_mbo(40, 'M', 'B', 100000, 9));

    // Move to new price
    ob.apply(make_mbo(40, 'M', 'B', 102000, 9));

    auto bb = ob.best_bid();
    ASSERT_TRUE(bb.has_value());
    ASSERT_EQ(bb->first, 102000);
    ASSERT_EQ(bb->second, 9);
}

/**
 * Test 8: Modify among multiple FIFO orders — size increase
 * should move order to end of queue while preserving others.
 */
TEST(OrderBookModify, ModifyAmongMultipleFIFO)
{
    OrderBook ob(0, 200000, 100);

    ob.apply(make_mbo(50, 'A', 'A', 101000, 3)); // A
    ob.apply(make_mbo(51, 'A', 'A', 101000, 4)); // B
    ob.apply(make_mbo(52, 'A', 'A', 101000, 5)); // C

    // Increase size of B => move B to back
    ob.apply(make_mbo(51, 'M', 'A', 101000, 10));

    size_t idx = ob.price_to_index(101000);
    auto& level = ob.get_level(false, idx);

    auto it = level.queue.begin();
    ASSERT_EQ(it->order_id, 50);  // A still first

    ++it;
    ASSERT_EQ(it->order_id, 52);  // C stays before B

    ASSERT_EQ(level.queue.back().order_id, 51);  // B moved to end
}

/*******************************************************
 *  ADDITIONAL MODIFY SIZE TESTS (4 cases)
 *******************************************************/

/**
 * Test M1: Modify size decrease (still > 0) — FIFO priority must remain unchanged.
 */
TEST(OrderBookModify, SizeDecreaseKeepFIFO)
{
    OrderBook ob(0, 200000, 100);

    // Add initial order
    ob.apply(make_mbo(60, 'A', 'B', 100000, 10));

    // Modify: decrease size 10 → 4
    ob.apply(make_mbo(60, 'M', 'B', 100000, 4));

    auto bb = ob.best_bid();
    ASSERT_TRUE(bb.has_value());
    ASSERT_EQ(bb->second, 4);

    size_t idx = ob.price_to_index(100000);
    auto& lvl = ob.get_level(true, idx);

    ASSERT_EQ(lvl.queue.size(), 1);
    ASSERT_EQ(lvl.queue.front().order_id, 60);
    ASSERT_EQ(lvl.queue.front().size, 4);
}

/**
 * Test M2: Modify size decrease among multiple orders — FIFO must remain unchanged.
 */
TEST(OrderBookModify, SizeDecreaseMultipleFIFO)
{
    OrderBook ob(0, 200000, 100);

    // Add three orders at same price
    ob.apply(make_mbo(70, 'A', 'B', 100000, 5)); // A
    ob.apply(make_mbo(71, 'A', 'B', 100000, 6)); // B
    ob.apply(make_mbo(72, 'A', 'B', 100000, 7)); // C

    // Decrease only B’s size → keep order in place
    ob.apply(make_mbo(71, 'M', 'B', 100000, 2));

    size_t idx = ob.price_to_index(100000);
    auto& lvl = ob.get_level(true, idx);

    ASSERT_EQ(lvl.queue.size(), 3);

    auto it = lvl.queue.begin();
    ASSERT_EQ(it->order_id, 70); // A stays first

    ++it;
    ASSERT_EQ(it->order_id, 71); // B stays second
    ASSERT_EQ(it->size, 2);

    ++it;
    ASSERT_EQ(it->order_id, 72); // C stays third

    ASSERT_EQ(lvl.total_size, 5 + 2 + 7);
}

/**
 * Test M3: Modify size increase within multiple orders — order must lose FIFO
 * priority and move to the end of the queue.
 */
TEST(OrderBookModify, SizeIncreaseMovesToEnd)
{
    OrderBook ob(0, 200000, 100);

    // A, B, C added
    ob.apply(make_mbo(80, 'A', 'A', 101000, 3));
    ob.apply(make_mbo(81, 'A', 'A', 101000, 4));
    ob.apply(make_mbo(82, 'A', 'A', 101000, 5));

    // Increase size of B → loses priority → move to back
    ob.apply(make_mbo(81, 'M', 'A', 101000, 10));

    size_t idx = ob.price_to_index(101000);
    auto& lvl = ob.get_level(false, idx);

    ASSERT_EQ(lvl.queue.size(), 3);

    auto it = lvl.queue.begin();
    ASSERT_EQ(it->order_id, 80); // A stays first

    ++it;
    ASSERT_EQ(it->order_id, 82); // C moves before B

    ASSERT_EQ(lvl.queue.back().order_id, 81); // B moved to end
    ASSERT_EQ(lvl.total_size, 3 + 10 + 5);
}

/**
 * Test M4: Modify size decreases after a previous increase — FIFO logic must follow:
 * 1) First modify: size increase → lose priority → move to end
 * 2) Second modify: size decrease → must NOT move back to original place
 * This test ensures priority loss is permanent.
 */
TEST(OrderBookModify, IncreaseThenDecreaseKeepsLostPriority)
{
    OrderBook ob(0, 200000, 100);

    // Add A, B, C
    ob.apply(make_mbo(90, 'A', 'B', 100000, 3)); // A
    ob.apply(make_mbo(91, 'A', 'B', 100000, 3)); // B
    ob.apply(make_mbo(92, 'A', 'B', 100000, 3)); // C

    // Step 1: increase B → loses priority → moved to back
    ob.apply(make_mbo(91, 'M', 'B', 100000, 8));

    size_t idx = ob.price_to_index(100000);
    auto& lvl = ob.get_level(true, idx);

    ASSERT_EQ(lvl.queue.back().order_id, 91);

    // Step 2: decrease B’s size → must stay in same position (cannot gain priority)
    ob.apply(make_mbo(91, 'M', 'B', 100000, 2));

    ASSERT_EQ(lvl.queue.back().order_id, 91); // still last
    ASSERT_EQ(lvl.queue.size(), 3);
    ASSERT_EQ(lvl.total_size, 3 + 3 + 2);
}