#pragma once

// c++
#include <chrono>
#include <list>



class MovingAverage {
public:
    MovingAverage(size_t window_size);

    void add(double value);

    double calc() const;

    void reset();


private:
    size_t window_size;
    std::list<double> values;
    double sum;
};



class TimeIt {
public:
    TimeIt();

    void reset();

    double elapsed_milliseconds() const;

    double elapsed_seconds() const;


private:
    std::chrono::time_point<std::chrono::high_resolution_clock> begin_time;
};

