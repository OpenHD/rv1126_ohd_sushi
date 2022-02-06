//
// Created by consti10 on 06.02.22.
//

#ifndef RV1126_OHD_SUSHI_CONSTI_TIME_HELPER_H
#define RV1126_OHD_SUSHI_CONSTI_TIME_HELPER_H

#include <sys/time.h>
#include <chrono>

static std::chrono::nanoseconds timevalToDuration(timeval tv){
    auto duration = std::chrono::seconds{tv.tv_sec}
                    + std::chrono::microseconds{tv.tv_usec};
    return std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
}
static std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds>
timevalToTimePointSystemClock(timeval tv){
    return std::chrono::time_point<std::chrono::system_clock,std::chrono::nanoseconds>{
            std::chrono::duration_cast<std::chrono::system_clock::duration>(timevalToDuration(tv))};
}
static std::chrono::time_point<std::chrono::steady_clock,std::chrono::nanoseconds>
timevalToTimePointSteadyClock(timeval tv){
    return std::chrono::time_point<std::chrono::steady_clock,std::chrono::nanoseconds>{
            std::chrono::duration_cast<std::chrono::steady_clock::duration>(timevalToDuration(tv))};
}
static uint64_t getTimeUs(){
    struct timeval time;
    gettimeofday(&time, NULL);
    uint64_t micros = (time.tv_sec * ((uint64_t)1000*1000)) + ((uint64_t)time.tv_usec);
    return micros;
}

#endif //RV1126_OHD_SUSHI_CONSTI_TIME_HELPER_H
