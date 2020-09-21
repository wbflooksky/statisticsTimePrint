#ifndef STATISTICSTIMEPRINT_HPP
#define STATISTICSTIMEPRINT_HPP
#include <iostream>
#include <chrono>
#include <ctime>
#include <mutex>
#include <functional>
#include <queue>
#include <thread>
#include <map>
#include <sstream>
#include <vector>
#include <queue>
#include <memory>
#include <future>
#include <stdexcept>
#include <atomic>
#include <condition_variable>
#include <unordered_map>

/*--------------------------------------------ElapsedTimer-------------------------------------------*/
class ElapsedTimer {
public:
    ElapsedTimer() = default;
    ~ElapsedTimer() = default;
    uint64_t elapsed() const;
    bool hasExpired(const uint64_t timeout) const;
    void start();
    void restart();
private:
    std::chrono::steady_clock::time_point clock_;
};

uint64_t ElapsedTimer::elapsed() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - clock_).count();
}

bool ElapsedTimer::hasExpired(const uint64_t timeout) const {
    return elapsed() >= timeout;
}

void ElapsedTimer::start() {
    clock_ = std::chrono::steady_clock::now();;
}

void ElapsedTimer::restart() {
    start();
}


/*-----------------------------------TimeConsumingPrint------------------------------*/
class TimeConsumingPrint {
public:
    const int RETRY = 3;
    // count min max total arg
    using timeItemPrint = std::tuple<uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t>;
    TimeConsumingPrint();
    ~TimeConsumingPrint();
    void appendRecording(const std::string& tag, const uint64_t& elapsed);
private:
    void printResult();

    std::unordered_map<std::string, timeItemPrint> tagPrint_;
    ElapsedTimer et_;
    int printInterval_ = 1000;
    std::mutex mtx_;
};

TimeConsumingPrint::TimeConsumingPrint() {
    et_.start();
}

TimeConsumingPrint::~TimeConsumingPrint() {
    printResult();
}

void TimeConsumingPrint::appendRecording(const std::string& tag, const uint64_t& elapsed) {
    {
        std::lock_guard<std::mutex> lck(mtx_);
        auto it = tagPrint_.find(tag);
        if (it != tagPrint_.end()) {
            auto& timePrint = it->second;
            std::get<0>(timePrint)++;
            std::get<1>(timePrint) = std::get<1>(timePrint) < elapsed ? std::get<1>(timePrint) : elapsed;
            std::get<2>(timePrint) = std::get<2>(timePrint) > elapsed ? std::get<1>(timePrint) : elapsed;
            std::get<3>(timePrint) += elapsed;
            std::get<4>(timePrint) = std::get<3>(timePrint) / std::get<0>(timePrint);
            std::get<5>(timePrint) = RETRY;
        }
        else {
            auto timePrint = timeItemPrint(1, elapsed, elapsed, elapsed, elapsed, RETRY);
            tagPrint_[tag] = std::move(timePrint);
        }
    }
    printResult();
}

void TimeConsumingPrint::printResult() {
    std::lock_guard<std::mutex> lck(mtx_);
    if (!et_.hasExpired(printInterval_)) {
        return;
    }
    for (auto it = tagPrint_.begin(); it != tagPrint_.end(); it++) {
        auto& timePrint = it->second;
        if (--std::get<5>(timePrint) < 0) {
            continue;
        }
        std::cout << "tag " << it->first << " statistics number " << std::get<0>(timePrint) << " time consuming min "
            << std::get<1>(timePrint) << "ms time consuming max " << std::get<2>(timePrint) << "ms time consuming count "
            << std::get<3>(timePrint) << "ms time consuming arg " << std::get<4>(timePrint) << "ms" << std::endl;
    }
    et_.start();
}

/*----------------------------------TimeConsuming-------------------------*/
class TimeConsuming {
public:
    TimeConsuming() = default;
    TimeConsuming(const std::string& tag);
    ~TimeConsuming();

    void addTimeConsumingTag(const std::string& tag);
    void end();
    bool drivingEnd(const std::string& tag);
private:
    void uniteOutput(const std::string& tag, const uint64_t& elapsed);
    std::map<std::string, ElapsedTimer> tagMap_;
};

TimeConsuming::TimeConsuming(const std::string& tag) {
    tagMap_[tag].start();
}

TimeConsuming::~TimeConsuming() {
    end();
}

void TimeConsuming::addTimeConsumingTag(const std::string& tag) {
    tagMap_[tag].start();
}

void TimeConsuming::end() {
    for (const auto& item : tagMap_) {
        uniteOutput(item.first, item.second.elapsed());
    }
}

bool TimeConsuming::drivingEnd(const std::string& tag) {
    auto it = tagMap_.find(tag);
    if (it == tagMap_.end()) {
        return false;
    }
    uniteOutput(it->first, it->second.elapsed());
    tagMap_.erase(it);
}

void TimeConsuming::uniteOutput(const std::string& tag, const uint64_t& elapsed) {
    static TimeConsumingPrint tp;
    tp.appendRecording(tag, elapsed);
}
#endif