// self
#include "math_utils.hpp"



MovingAverage::MovingAverage(size_t window_size)
    : window_size(window_size)
    , sum(0.0)
{

}


void MovingAverage::add(double value) {
    if (values.size() >= window_size) {
        sum -= values.front();
        values.pop_front();
    }
    values.push_back(value);
    sum += value;
}


double MovingAverage::calc() const {
    if (values.empty()) {
        return 0.0;
    }
    return sum / values.size();
}


void MovingAverage::reset() {
    values.clear();
    sum = 0.0;
}



TimeIt::TimeIt()
    : begin_time(std::chrono::high_resolution_clock::now())
{

}


void TimeIt::reset() {
    begin_time = std::chrono::high_resolution_clock::now();
}


double TimeIt::elapsed_milliseconds() const {
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - begin_time);
    return duration.count();
}


double TimeIt::elapsed_seconds() const {
    return elapsed_milliseconds() / 1000.0;
}

