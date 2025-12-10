#pragma once

#include <vector>
#include <list>
#include <unordered_map>
#include <cstdint>
#include <optional>

#include <databento/dbn.hpp>
#include <databento/dbn_file_store.hpp>

#include <json/json.h>
#include <coroutine/event_base_manager.h>
#include <coroutine/task.h>
#include <coroutine/future.h>

class OrderBook
{
public:
    struct Order
    {
        uint64_t order_id;
        uint32_t size;
    };

    struct Level
    {
        std::list<Order> queue;     // FIFO queue
        uint64_t total_size = 0;

        inline bool empty() const { return queue.empty(); }
    };

    struct LevelByPrice
    {
        int64_t price;
        Level* level;
    };

    struct DepthEntry
    {
        int64_t price;
        uint64_t size;
    };

    struct DepthSnapshot
    {
        std::vector<DepthEntry> bids;   // sorted desc (best → worst)
        std::vector<DepthEntry> asks;   // sorted asc (best → worst)
    };

private:
    // ===== CONFIG =====
    int64_t m_price_min;
    int64_t m_price_max;
    int64_t m_tick_size;
    size_t  m_num_levels;

    // ===== BOOK ARRAYS =====
    std::vector<Level> m_bids;  // indexed as: higher price → bigger index
    std::vector<Level> m_asks;  // indexed as: lower price  → smaller index

    // ===== ORDER LOOKUP TABLE =====
    struct Ref
    {
        bool is_bid;
        size_t index;
        std::list<Order>::iterator it;
        int64_t price;
    };
    std::unordered_map<uint64_t, Ref> m_orders_ref;

    EventBase* event_base = nullptr;

public:

    OrderBook(int64_t price_min, int64_t price_max, int64_t tick, EventBase* event_base)
        : m_price_min(price_min),
          m_price_max(price_max),
          m_tick_size(tick),
          event_base(event_base)
    {
        m_num_levels = (m_price_max - m_price_min) / m_tick_size + 1;
        m_bids.resize(m_num_levels);
        m_asks.resize(m_num_levels);
        m_orders_ref.reserve(10000000); // preallocate for performance
    }

    inline size_t price_to_index(int64_t px) const
    {
        return (px - m_price_min) / m_tick_size;
    }

    inline Level& get_side(bool is_bid, size_t idx)
    {
        return is_bid ? m_bids[idx] : m_asks[idx];
    }

    inline const Level& get_side(bool is_bid, size_t idx) const
    {
        return is_bid ? m_bids[idx] : m_asks[idx];
    }

    // ============================================
    // APPLY MBO MESSAGE
    // ============================================
    void apply(const databento::MboMsg& mbo)
    {
        char action = (char)mbo.action;
        char side   = (char)mbo.side;

        if (action == 'R') { clear(); return; }
        if (side != 'A' && side != 'B') return;

        bool is_bid = (side == 'B');
        size_t idx = price_to_index(mbo.price);

        switch (action)
        {
            case 'T': handle_trade(mbo); break;
            case 'F': handle_full_fill(mbo); break;
            case 'N': handle_non_printed(mbo); break;
            case 'A': add(mbo, is_bid, idx); break;
            case 'C': cancel(mbo); break;
            case 'M': modify(mbo, is_bid, idx); break;
        }
    }

    // ============================================
    // OPERATIONS
    // ============================================

    void add(const databento::MboMsg& mbo, bool is_bid, size_t idx)
    {
        Level& level = get_side(is_bid, idx);

        level.queue.push_back({mbo.order_id, mbo.size});
        auto it = std::prev(level.queue.end());
        level.total_size += mbo.size;

        m_orders_ref[mbo.order_id] = Ref{is_bid, idx, it, mbo.price};
    }

    void cancel(const databento::MboMsg& mbo)
    {
        auto it = m_orders_ref.find(mbo.order_id);
        if (it == m_orders_ref.end()) return;

        Ref& ref = it->second;
        Level& level = get_side(ref.is_bid, ref.index);

        uint32_t cancel_sz = mbo.size;
        uint32_t& ord_sz = ref.it->size;

        if (cancel_sz >= ord_sz)
        {
            level.total_size -= ord_sz;
            level.queue.erase(ref.it);
            m_orders_ref.erase(it);
        }
        else
        {
            ord_sz -= cancel_sz;
            level.total_size -= cancel_sz;
        }
    }

    void modify(const databento::MboMsg& mbo, bool new_is_bid, size_t new_idx)
    {
        auto it = m_orders_ref.find(mbo.order_id);

        // Treat modify on missing ID as add()
        if (it == m_orders_ref.end())
        {
            add(mbo, new_is_bid, new_idx);
            return;
        }

        Ref ref = it->second; // copy
        Level& old_level = get_side(ref.is_bid, ref.index);
        uint32_t old_size = ref.it->size;

        // CASE 1: Size becomes zero → delete order
        if (mbo.size == 0)
        {
            old_level.total_size -= old_size;
            old_level.queue.erase(ref.it);
            m_orders_ref.erase(it);
            return;
        }

        // CASE 2: Price changed → remove and re-add (loses all priority)
        if (mbo.price != ref.price)
        {
            // remove from old level
            old_level.total_size -= old_size;
            old_level.queue.erase(ref.it);
            m_orders_ref.erase(it);

            // re-add into new level
            add(mbo, new_is_bid, new_idx);
            return;
        }

        // CASE 3: Same price, size increases → lose priority
        if (mbo.size > old_size)
        {
            old_level.total_size += (mbo.size - old_size);
            ref.it->size = mbo.size;

            // move to back (lose priority)
            old_level.queue.splice(old_level.queue.end(), old_level.queue, ref.it);

            // update ref info
            it->second.price = mbo.price;
            return;
        }

        // CASE 4: Same price, size decreases → keep priority
        if (mbo.size < old_size)
        {
            old_level.total_size -= (old_size - mbo.size);
            ref.it->size = mbo.size;
        }

        // Update stored price for future comparisons
        it->second.price = mbo.price;
    }

    void handle_trade(const databento::MboMsg& mbo)
    {
        auto it = m_orders_ref.find(mbo.order_id);
        if (it == m_orders_ref.end()) return;

        Ref& ref = it->second;
        Level& level = get_side(ref.is_bid, ref.index);

        uint32_t& ord_sz = ref.it->size;

        if (mbo.size >= ord_sz) {
            level.total_size -= ord_sz;
            level.queue.erase(ref.it);
            m_orders_ref.erase(it);
        } else {
            ord_sz -= mbo.size;
            level.total_size -= mbo.size;
        }
    }

    void handle_full_fill(const databento::MboMsg& mbo)
    {
        auto it = m_orders_ref.find(mbo.order_id);
        if (it == m_orders_ref.end()) return;

        Ref& ref = it->second;
        Level& level = get_side(ref.is_bid, ref.index);

        level.total_size -= ref.it->size;
        level.queue.erase(ref.it);
        m_orders_ref.erase(it);
    }

    void handle_non_printed(const databento::MboMsg& mbo)
    {
        handle_trade(mbo); // same logic as T
    }

    // ============================================
    // BEST BID / BEST ASK
    // ============================================

    std::optional<std::pair<int64_t,uint64_t>> best_bid() const
    {
        for (int i = (int)m_num_levels-1; i >= 0; --i)
        {
            if (!m_bids[i].empty())
            {
                int64_t price = m_price_min + i * m_tick_size;
                return std::make_pair(price, m_bids[i].total_size);
            }
        }
        return std::nullopt;
    }

    std::optional<std::pair<int64_t,uint64_t>> best_ask() const
    {
        for (size_t i = 0; i < m_num_levels; ++i)
        {
            if (!m_asks[i].empty())
            {
                int64_t price = m_price_min + i * m_tick_size;
                return std::make_pair(price, m_asks[i].total_size);
            }
        }
        return std::nullopt;
    }

    // ============================================

    const Level& get_level(bool is_bid, size_t idx) const
    {
        return is_bid ? m_bids[idx] : m_asks[idx];
    }

    // ============================================

    DepthSnapshot get_depth(int levels) const
    {
        DepthSnapshot out;
        out.bids.reserve(levels);
        out.asks.reserve(levels);

        // -------------------- BIDS (descending) --------------------
        int count = 0;
        for (int idx = (int)m_num_levels - 1; idx >= 0 && count < levels; --idx)
        {
            if (!m_bids[idx].empty())
            {
                int64_t price = m_price_min + idx * m_tick_size;
                uint64_t size = m_bids[idx].total_size;

                out.bids.push_back({price, size});
                count++;
            }
        }

        // -------------------- ASKS (ascending) --------------------
        count = 0;
        for (size_t idx = 0; idx < m_num_levels && count < levels; ++idx)
        {
            if (!m_asks[idx].empty())
            {
                int64_t price = m_price_min + idx * m_tick_size;
                uint64_t size = m_asks[idx].total_size;

                out.asks.push_back({price, size});
                count++;
            }
        }

        return out;
    }

    // ============================================
    // FULL BOOK SNAPSHOT
    // ============================================

    Json build_snapshot() const
    {
        Json snap;

        // ----------- BIDS (high → low) --------------
        Json bids;
        for (int i = (int)m_num_levels - 1; i >= 0; --i)
        {
            const Level& lvl = m_bids[i];
            if (!lvl.empty())
            {
                int64_t price = m_price_min + i * m_tick_size;
                bids.push_back({
                    {"price", price},
                    {"size" , lvl.total_size}
                });
            }
        }

        // ----------- ASKS (low → high) --------------
        Json asks;
        for (size_t i = 0; i < m_num_levels; ++i)
        {
            const Level& lvl = m_asks[i];
            if (!lvl.empty())
            {
                int64_t price = m_price_min + i * m_tick_size;
                asks.push_back({
                    {"price", price},
                    {"size" , lvl.total_size}
                });
            }
        }

        snap["bids"] = bids;
        snap["asks"] = asks;

        return snap;
    }

    Task<void> get_snapshot_async(Future<Json>::FutureValue future_value)
    {
        Json snap = build_snapshot();
        future_value.set_value(std::move(snap));

        co_return;
    }

    Future<Json> get_snapshot()
    {
        return Future<Json>([this](Future<Json>::FutureValue future_value)
        {
            auto task = this->get_snapshot_async(future_value);
            task.start_running_on(event_base);
        });
    }

    // ===========================================
    // Builld MbpMsg10
    // ===========================================
    Json build_mbp_msg10_from_mbo_msg(const databento::MboMsg& mbo_msg) const
    {
        Json bid_ask_pairs = build_bid_ask_pairs_by_level(10);

        Json mbp10_msg = {
            {"hd", {
                {"rtype", mbo_msg.hd.rtype},
                {"ts_event", mbo_msg.hd.ts_event.time_since_epoch().count()},
                {"instrument_id", mbo_msg.hd.instrument_id},
                {"publisher_id", mbo_msg.hd.publisher_id},
                {"length", mbo_msg.hd.length}
            }},
            {"price", mbo_msg.price},
            {"size", mbo_msg.size},
            {"action", mbo_msg.action},
            {"side", mbo_msg.side},
            {"flags", mbo_msg.flags.Raw()},
            {"depth", bid_ask_pairs["num_levels"]},
            {"ts_recv", mbo_msg.ts_recv.time_since_epoch().count()},
            {"ts_in_delta", mbo_msg.ts_in_delta.count()},
            {"sequence", mbo_msg.sequence},
            {"levels", bid_ask_pairs["bid_ask_pairs"]}
        };

        return mbp10_msg;
    }

    Json build_bid_ask_pairs_by_level(size_t levels) const
    {
        LevelByPrice bid_levels[20];
        LevelByPrice ask_levels[20];
        size_t bid_count = 0;
        size_t ask_count = 0;

        // ----------- BIDS (high → low) --------------
        for (int i = (int)m_num_levels - 1; i >= 0; --i)
        {
            const Level& lvl = m_bids[i];
            if (!lvl.empty())
            {
                int64_t price = m_price_min + i * m_tick_size;
                bid_levels[bid_count++] = LevelByPrice{price, (Level*)&lvl};

                if (bid_count >= levels) break;
            }
        }

        // ----------- ASKS (low → high) --------------
        for (size_t i = 0; i < m_num_levels; ++i)
        {
            const Level& lvl = m_asks[i];
            if (!lvl.empty())
            {
                int64_t price = m_price_min + i * m_tick_size;
                ask_levels[ask_count++] = LevelByPrice{price, (Level*)&lvl};

                if (ask_count >= levels) break;
            }
        }

        // Build bid-ask pairs
        Json bid_ask_pairs;
        for (size_t i = 0; i < levels; ++i)
        {
            Json pair;

            // Bid
            if (i < bid_count)
            {
                pair["bid_px"] = bid_levels[i].price;
                pair["bid_sz"] = bid_levels[i].level->total_size;
                pair["bid_ct"] = bid_levels[i].level->queue.size();
            }
            else
            {
                pair["bid_px"] = 0;
                pair["bid_sz"] = 0;
                pair["bid_ct"] = 0;
            }

            // Ask
            if (i < ask_count)
            {
                pair["ask_px"] = ask_levels[i].price;
                pair["ask_sz"] = ask_levels[i].level->total_size;
                pair["ask_ct"] = ask_levels[i].level->queue.size();
            }
            else
            {
                pair["ask_px"] = 0;
                pair["ask_sz"] = 0;
                pair["ask_ct"] = 0;
            }

            bid_ask_pairs.push_back(pair);
        }

        return {
            {"bid_ask_pairs", bid_ask_pairs},
            {"num_levels", std::min(levels, std::max(bid_count, ask_count))}
        };
    }

    // ============================================

    void clear()
    {
        for (auto& lvl : m_bids) lvl = Level{};
        for (auto& lvl : m_asks) lvl = Level{};
        m_orders_ref.clear();
    }
};
