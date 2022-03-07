#ifndef ERER_UTILS_TIMER_H_
#define ERER_UTILS_TIMER_H_

#include <iostream>
#include <chrono>

namespace Utils
{
    class Timer
    {
    public:
        static std::chrono::system_clock::time_point tp1;
        static std::chrono::system_clock::time_point tp2;

        static void start()
        {
            tp1 = std::chrono::system_clock::now();
        }
        static void clock()
        {
            tp2 = std::chrono::system_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(tp2 - tp1);
            std::cout << "Cost : "
                      << double(duration.count()) * std::chrono::microseconds::period::num / std::chrono::microseconds::period::den
                      << " s" << std::endl;
            tp1 = std::chrono::system_clock::now();
        }
    };
    std::chrono::system_clock::time_point Timer::tp1;
    std::chrono::system_clock::time_point Timer::tp2;
}

#endif // ERER_UTILS_TIMER_H_