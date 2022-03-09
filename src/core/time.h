#ifndef ERER_CORE_TIME_H_
#define ERER_CORE_TIME_H_
#include <chrono>
#include <iostream>

namespace Core
{
    class Time
    {
    private:
        static std::chrono::system_clock::time_point current_frame_time_;
        static std::chrono::system_clock::time_point last_frame_time_;

    public:
        static void clock()
        {
            current_frame_time_ = std::chrono::system_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(current_frame_time_ - last_frame_time_);
            std::cout << "Cost : "
                      << double(duration.count()) * std::chrono::microseconds::period::num / std::chrono::microseconds::period::den
                      << " s" << std::endl;
            last_frame_time_ = current_frame_time_;
        }
        // Return delta time (s) between two frames
        static double get_delta_time()
        {
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(current_frame_time_ - last_frame_time_);
            return static_cast<double>(duration.count()) * std::chrono::microseconds::period::num / std::chrono::microseconds::period::den;
        }
    };

    std::chrono::system_clock::time_point Time::current_frame_time_ = std::chrono::system_clock::now();
    std::chrono::system_clock::time_point Time::last_frame_time_ = std::chrono::system_clock::now();
}

#endif // ERER_CORE_TIME_H_
