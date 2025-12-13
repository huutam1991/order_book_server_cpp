#pragma once

#include <vector>
#include <algorithm>
#include <chrono>
#include <mutex>

struct LatencyTracker
{
    std::vector<double> samples;
    size_t max_samples = 200000;

    inline void add_sample(double value)
    {
        samples.push_back(value);
        if (samples.size() > max_samples)
        {
            samples.erase(samples.begin(), samples.begin() + samples.size() / 4);
        }
    }

    // Get percentile (50, 90, 99)
    inline double percentile(double p)
    {
        if (samples.empty())
        {
            return 0.0;
        }

        std::vector<double> v = samples;
        std::sort(v.begin(), v.end());
        size_t idx = static_cast<size_t>((p / 100.0) * (v.size() - 1));
        return v[idx];
    }

    inline double p50() { return percentile(50); }
    inline double p90() { return percentile(90); }
    inline double p99() { return percentile(99); }

    inline void clear()
    {
        samples.clear();
    }
};
