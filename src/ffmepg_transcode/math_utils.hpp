#pragma once

// c++
#include <chrono>
#include <list>
#include <vector>



class MovingAverage {
public:
    MovingAverage(size_t window_size);

    void add(double value);

    double calc() const;

    void reset();


private:
    size_t m_window_size;
    std::list<double> m_values;
    double m_sum;
};



class TimeIt {
public:
    TimeIt();

    void reset();

    double elapsed_milliseconds() const;

    double elapsed_seconds() const;


private:
    std::chrono::time_point<std::chrono::high_resolution_clock> m_begin_time;
};



class Percentile {
public:
    Percentile();

    void add(double value);

    double calc(double percentile);

    void reset();


private:
    std::vector<double> m_values;
};


