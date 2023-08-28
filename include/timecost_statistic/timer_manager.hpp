#ifndef TIMER_MANAGER_HPP
#define TIMER_MANAGER_HPP

#include <chrono>
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <set>
#include <iostream>
#include <sstream>
#include <ostream>
#include <cmath>
#include <stack>
#include <memory>
#include <numeric>

// using namespace std::chrono_literals;
// using namespace std::chrono;

// #define __DEBUG__

namespace timecost_statistic
{

inline void replace_str(std::string& s, const std::string a, const std::string b) {
    size_t pos;
    while ((pos = s.find(a)) != std::string::npos) {
        s.replace(pos, a.length(), b);
    }
}

using ClockT = std::chrono::system_clock;
using TimePointT = std::chrono::time_point<ClockT>;
using DurationT = std::chrono::microseconds;

struct Timer;
using TimerPtr = std::shared_ptr<Timer>;

struct Timer{
  const std::string name; // name of this timer
  const std::string path; // full path from root to this timer
  TimePointT beginning;
  DurationT duration;

  Timer(const std::string& name_, const std::string& path_)
    : name(name_), path(path_){}

};

struct Record {
  const size_t id{0};
  std::vector<TimerPtr> timers;   // stores the timer tree in post-order

  Record(const size_t id_) : id(id_){}
};

/**
 * @brief
 * @todo change to a template class for specifying duration type
 */
class TimerManager{
 private:
  TimerManager()
    : log_info_(std::cout), log_warn_(std::cout), log_err_(std::cerr)
  {};

  TimerManager(std::ostream& info, std::ostream& warn, std::ostream& err)
    : log_info_(info), log_warn_(warn), log_err_(err)
  {};

  TimerPtr getTimer(const std::string& name) const {
    std::string path =
        timer_stack_.empty() ? std::string("") : timer_stack_.top()->path + "/";
    TimerPtr new_timer(new Timer(name, path+name));
    return new_timer;
  }

 public:
  static TimerManager& getInstance() {
    static TimerManager instance;
    return instance;
  }

  ~TimerManager() {
    calcStatistic();
    flattenRecords();
  }

  std::vector<Record> &getRecords() {
    return records_;
  }

  bool startTimer(
      std::string name,
      const TimePointT* beginning=nullptr)
  {
    std::replace(name.begin(), name.end(), '/', '_');
    if(inactive_timers_.find(name) != inactive_timers_.end()) {
      return false;
    }

    if(timer_stack_.empty()) {
      records_.push_back(Record(records_.size()));
    }

    TimerPtr timer = getTimer(name);
    // timer->ticking = true;
    timer->beginning =
        beginning ? *beginning : std::chrono::high_resolution_clock::now();
    timer_stack_.push(timer);

    return true;
  }

  bool endTimer(
      const TimePointT ending=std::chrono::high_resolution_clock::now())
  {
    if(timer_stack_.empty()) {
      log_err_ << "No timer ticking" << std::endl;
      return false;
    }

    TimerPtr timer = timer_stack_.top();
    timer_stack_.pop();
    timer->duration =
        std::chrono::duration_cast<DurationT>(ending - timer->beginning);

    records_.back().timers.push_back(timer);
    return true;
  }

  /**
   * @brief Disable timers with IDs specified
   * @param names specified names of timers to disable
   */
  void disableTimers(const std::vector<std::string>& names){
    for(const auto& name : names) {
      inactive_timers_.insert(name);
    }
  }

  /**
   * @brief Enable all timers
  */
  void enableAllTimers(){
    inactive_timers_.clear();
  }

  /**
   * @brief Enable timers with IDs specified
   * @param names specified names of timers to enable
   */
  void enableTimers(const std::vector<std::string>& names){
    for(const auto& name : names) {
      auto timer = inactive_timers_.find(name);
      if(timer != inactive_timers_.end()){
        inactive_timers_.erase(timer);
      }
    }
  }

  const std::vector<Record>& getRecords() const {
    return records_;
  }

  void flattenRecords(){
    flattenRecords(log_info_);
  }

  /**
   * @brief flatten records of all timers
   * @param out ostream object that stores flattened records
   * @todo feature: reverse output
   * @todo feature: only output n records from head
   * @todo feature: set output step size
   */
  void flattenRecords(std::ostream& out){
    out.precision(6);
#ifdef __DEBUG__
    for(const auto &timer : timers_) {
      out << timer.first << "(" << timer.second.records.size() <<")" << ", ";
    }
    out << std::endl;
#endif
    out << "List of Records: \n";

    size_t i = 0;
    for(const auto& record : records_) {
      out << "  [ #" << i++ << " ] | ";
      for(const auto& timer : record.timers) {
        out << timer->name  << ": "
            << std::chrono::duration_cast<std::chrono::microseconds>(
                    timer->duration).count()*1e-3 << "ms | ";
      }
      out << std::endl;
    }

  }

  void calcStatistic(){
    calcStatistic(log_info_);
  }

  void calcStatistic(std::ostream& out){
    out.precision(6);
    if(records_.empty()) {
      log_warn_ << "No records found" << std::endl;
      return;
    }

    using MapElemT = std::pair<std::string, std::vector<DurationT>>;
    std::map<std::string, std::vector<DurationT>> timers;
    for(const auto& record : records_) {
      for(const auto& timer : record.timers) {
        auto iter = timers.find(timer->name);
        if(iter == timers.end()) {
          timers.insert(MapElemT(timer->name, {}));
          iter = timers.find(timer->name);
        }
        iter->second.emplace_back(timer->duration);
      }
    }

    out << "Statistics:\n";

    for(const auto& timer : timers) {
      const auto &durations = timer.second;
      out << "  [" << timer.first << "(count: " << durations.size() <<")] | ";

      // min
      out << "Min: "
          << std::chrono::duration_cast<std::chrono::microseconds>(
               *std::min_element(durations.begin(), durations.end())
             ).count()*1e-3
          << "ms | ";

      // max
      out << "Max: "
          << std::chrono::duration_cast<std::chrono::microseconds>(
               *std::max_element(durations.begin(), durations.end())
             ).count()*1e-3
          << "ms | ";

      // average
      out << "Avg: "
          << std::chrono::duration_cast<std::chrono::microseconds>(
               std::accumulate(durations.begin(), durations.end(), DurationT(0))
             ).count()*1e-3 / durations.size()
          << "ms | ";

      // stddev
      out << "Stddev: "
          << std::chrono::duration_cast<std::chrono::microseconds>(
                  stddev(durations)).count()*1e-3 << "ms | ";

      out << std::endl;
    }
    // out << std::endl;
  }

  bool exportRecords(std::ostream& out) {
    // log_info_ << "Export " << records_.size() << " records ..." << std::endl;
    std::string indentation = "  ";
    out.precision(6);
    size_t count = 0;
    for(const auto& record : records_) {
      if((++count % 50)==0)
        log_info_ << "Exporting " << count << "/" << records_.size()
                  << " record ..." << std::endl;

      out << "Record #" << record.id << ":\n";
      for(const auto& timer : record.timers) {
        out << indentation << timer->path << ": "
            << std::chrono::duration_cast<std::chrono::microseconds>(
                    timer->duration).count()*1e-3 << "ms"
            << std::endl;
      }
      out << std::endl;
    }
    log_info_ << "Exported " << count << " records" << std::endl;
    return true;
  }

  /**
   * @brief Calculate stddev of duration records
   * @param records
   * @return stddev
   */
  static DurationT stddev(const std::vector<DurationT>& records){
    auto sum = [](const std::vector<DurationT>& records)->DurationT{
      DurationT total(0);
      for(const auto& record : records) {
        total += record;
      }
      return total;
    };

    auto avg = [&](const std::vector<DurationT>& records)->DurationT{
      if(records.size() < 1) {
        return DurationT(0);
      }
      return sum(records)/records.size();
    };

    if(records.size() < 1) return DurationT(0);

    DurationT avg_val = avg(records);
    double var=0.0;
    for(const auto& record : records) {
      var += std::pow((avg_val-record).count(), 2);
    }
    var /= records.size();
    double stddev_val = std::sqrt(var);

    return DurationT(int64_t(stddev_val));
  };

 private:
  std::mutex mtx_;

  std::ostream& log_info_;
  std::ostream& log_warn_;
  std::ostream& log_err_;

  // std::map<std::string, TimerRegPtr> timers_;
  std::set<std::string> inactive_timers_;
  std::stack<TimerPtr> timer_stack_;
  std::vector<Record> records_;
};

} // namespace time_statistic


//===============================================
//                    Macros
//===============================================

#ifdef DISABLE_TIMECOST_STATISTIC

#define TS_START(name)
#define TS_END()
#define TS_EXPORT_TO_TXT(filepath)

#else

#define TS_START(name) \
  do { \
    std::string timer_name{name}; \
    if (timer_name.empty()) { \
      timer_name = std::string(__FILE__) + "_" + \
        std::string(__FUNCTION__) + "_" + \
        std::to_string(__LINE__); \
      timer_name = timer_name.substr(timer_name.find_last_of("/") + 1); \
    } else { \
      timecost_statistic::replace_str(timer_name, "/", "_"); \
    } \
    timecost_statistic::TimerManager::getInstance().startTimer(timer_name); \
  } while (0);

#define TS_END() \
  timecost_statistic::TimerManager::getInstance().endTimer();

#define TS_EXPORT_TO_TXT(filepath) \
  std::ofstream fs; \
  fs.open(filepath); \
  timecost_statistic::TimerManager::getInstance().exportRecords(fs); \
  fs.close(); \

#endif

#endif  // TIME_MANAGER_HPP
