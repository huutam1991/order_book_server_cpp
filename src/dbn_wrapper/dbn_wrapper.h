#pragma once

#include <functional>
#include <chrono>

#include <databento/dbn.hpp>
#include <databento/dbn_file_store.hpp>

#include <coroutine/task.h>
#include <time/timer.h>

class DbnWrapper
{
public:
    using Callback = std::function<void(const databento::MboMsg&)>;

    DbnWrapper(const std::string& file_path) : m_store(file_path), m_speed(1.0), m_stop(false)
    {}

    // speed = 1.0  => real time
    // speed = 2.0  => 2x faster
    // speed = 0.5  => half speed
    // speed = 0    => max speed (no sleep)
    void set_speed(double speed)
    {
        m_speed = speed;
    }

    Task<void> start_stream_data(Callback cb)
    {
        m_stop = false;

        const databento::Record* rec;
        std::optional<databento::UnixNanos> prev_ts;

        while (!m_stop && (rec = m_store.NextRecord()))
        {
            const auto* mbo = rec->GetIf<databento::MboMsg>();
            if (!mbo) continue;

            if (mbo->ts_in_delta.count() > 0 && m_speed > 0.0)
            {
                auto sleep_ns = static_cast<long long>(mbo->ts_in_delta.count() / m_speed);
                co_await Timer::sleep_for(sleep_ns, Timer::TimerUnit::NANOSECOND);
            }

            // invoke user callback
            cb(*mbo);
        }

        if (m_end_callback)
        {
            m_end_callback();
        }

        co_return;
    }

    void set_end_callback(std::function<void()> cb)
    {
        m_end_callback = cb;
    }

    void stop()
    {
        m_stop = true;
    }

private:
    databento::DbnFileStore m_store;
    double m_speed;
    bool m_stop;
    std::function<void()> m_end_callback = nullptr;
};
