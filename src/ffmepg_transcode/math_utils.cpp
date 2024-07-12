// self
#include "math_utils.hpp"

// c
#include <math.h>

// c++
#include <algorithm>



MovingAverage::MovingAverage(size_t window_size)
    : m_window_size(window_size)
    , m_sum(0.0)
{

}


void MovingAverage::add(double value) {
    if (m_values.size() >= m_window_size) {
        m_sum -= m_values.front();
        m_values.pop_front();
    }
    m_values.push_back(value);
    m_sum += value;
}


double MovingAverage::calc() const {
    if (m_values.empty()) {
        return 0.0;
    }
    return m_sum / m_values.size();
}


void MovingAverage::reset() {
    m_values.clear();
    m_sum = 0.0;
}



TimeIt::TimeIt()
    : m_begin_time(std::chrono::high_resolution_clock::now())
{

}


void TimeIt::reset() {
    m_begin_time = std::chrono::high_resolution_clock::now();
}


double TimeIt::elapsed_milliseconds() const {
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - m_begin_time);
    return duration.count();
}


double TimeIt::elapsed_seconds() const {
    return elapsed_milliseconds() / 1000.0;
}



Percentile::Percentile()
{
}


void Percentile::add(double value)
{
    m_values.push_back(value);
}


double Percentile::calc(double percentile)
{
    std::sort(m_values.begin(), m_values.end());  // sort the data

    double pos_of_percentile = (m_values.size() - 1) * percentile + 1;  // calculate the position of the percentile in the sorted data
    int npos = static_cast<int>(pos_of_percentile);  // convert the position to an integer
    double decimal_part = pos_of_percentile - npos;  // calculate the decimal part

    if (npos < 1) {
        return m_values[0];  // if the position is less than 1, return the minimum value
    }
    else if (npos >= m_values.size()) {
        return m_values[m_values.size() - 1];  // if the position is greater than or equal to the size of the dataset, return the maximum value
    }
    else {
        return m_values[npos - 1] + decimal_part * (m_values[npos] - m_values[npos - 1]);  // linear interpolation to calculate the percentile
    }
}


void Percentile::reset()
{
    m_values.clear();
}
