#pragma once

#include <functional>
#include <chrono>

#include <databento/dbn.hpp>
#include <databento/dbn_file_store.hpp>

#include <coroutine/task.h>
#include <coroutine/future.h>
#include <time/timer.h>

class DbnWrapper
{
public:
    using Callback = std::function<void(const databento::MboMsg&)>;

    DbnWrapper(const std::string& file_path) : m_store(file_path), m_speed(1.0), m_is_streaming(false)
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
        m_is_streaming = true;

        const databento::Record* rec;
        std::optional<databento::UnixNanos> prev_ts;

        while (m_is_streaming && (rec = m_store.NextRecord()))
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

        m_is_streaming = false;

        if (m_end_callback)
        {
            m_end_callback();
        }

        if (m_stop_future_value.is_value_set() == false)
        {
            m_stop_future_value.set_value(true);
        }

        spdlog::warn("Finished streaming DBN file");

        co_return;
    }

    void set_end_callback(std::function<void()> cb)
    {
        m_end_callback = std::move(cb);
    }

    bool get_is_streaming()
    {
        return m_is_streaming;
    }

    Future<bool> stop()
    {
        return Future<bool>([this](Future<bool>::FutureValue future_value)
        {
            m_stop_future_value = future_value;
            m_is_streaming = false;
        });
    }

private:
    databento::DbnFileStore m_store;
    double m_speed;
    bool m_is_streaming = false;
    std::function<void()> m_end_callback = nullptr;
    Future<bool>::FutureValue m_stop_future_value;
};
