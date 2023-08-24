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
// using namespace std::chrono_literals;
// using namespace std::chrono;

// #define __DEBUG__

namespace timecost_statistic
{
using ClockT = std::chrono::system_clock;
using TimePointT = std::chrono::time_point<ClockT>;
using DurationT = std::chrono::microseconds;

struct Timer;
// using TimerRegT = std::pair<std::string, Timer>;
// using TimerRegPtr = std::shared_ptr<TimerRegT>;
using TimerPtr =  std::shared_ptr<Timer>;

struct Timer{
  const std::string name;                 // name of this timer
  const std::string path;                   // full path from root to this timer
  // bool ticking{false};
  // bool enable{true};
  TimePointT beginning;
  // std::vector<TimerPtr> childs;
  std::vector<DurationT> records;
  DurationT duration;

  Timer(const std::string& name_, const std::string& path_)
    : name(name_), path(path_){}

  DurationT sum() const {
    DurationT total(0);
    for(const auto& record : records) {
      total += record;
    }
    return total;
  }

  DurationT avg() const {
    if(records.size() < 1) {
      return DurationT(0);
    }
    return sum()/records.size();
  }

  DurationT min() const {
    DurationT min_duration = DurationT::max();
    for(const auto& record : records) {
      if(record < min_duration) min_duration = record;
    }
    return min_duration;
  }

  DurationT max() const {
    DurationT max_duration = DurationT::min();
    for(const auto& record : records) {
      if(record > max_duration) max_duration = record;
    }
    return max_duration;
  }

  DurationT stddev() const {
    if(records.size() < 1) return DurationT(0);

    DurationT avg_val = this->avg();
    double var=0.0;
    for(const auto& record : records) {
      var += std::pow((avg_val-record).count(), 2);
    }
    var /= records.size();
    double stddev_val = std::sqrt(var);

    return DurationT(int64_t(stddev_val));
  }
};

struct Record {
  const size_t id{0};
  std::vector<TimerPtr> timers;   // stores the timer tree in post-order

  Record(const size_t id_) : id(id_){}
};

/**
 * @brief
 * @todo offer interface for setting log message stream
 * @todo change to a template class for specifying duration type
 * @todo use a stack to organize the waking up order of timers
 * @todo add singleton mode
 */
class TimerManager{
 public:
  TimerManager()
    : log_info_(std::cout), log_warn_(std::cout), log_err_(std::cerr)
  {

  };

  // ~TimerManager();

  inline TimerPtr getTimer(const std::string& name) const {
    std::string path =
        timer_stack_.empty() ? std::string("/") : timer_stack_.top()->path + "/";
    TimerPtr new_timer(new Timer(name, path+name));
    return new_timer;
  }

  inline bool startTimer(
      const std::string& name,
      const TimePointT* beginning=nullptr)
  {
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

  inline bool endTimer(
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
    timer->records.emplace_back(timer->duration);

    records_.back().timers.push_back(timer);
    return true;
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

    // for(size_t i = 0; i < max_timer_record_size_; ++i){
    //   out << "  [ #" << i <<" ] | ";
    //   for(const auto& timer : timers_) {
    //     out << timer.first << "(" << timer.second.count_valid <<")" << ": ";
    //     if(timer.second.records.size() <= i
    //        || timer.second.records[i]==nullptr)
    //     {
    //       out << "nan | ";
    //     } else {
    //       out << std::chrono::duration_cast<std::chrono::microseconds>(
    //                 *timer.second.records[i]).count()*1e-3 << "ms | ";
    //     }
    //   }
    //   out << std::endl;
    // }

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
    // if(timers_.empty()) {
    //   out << "No timers registered" << std::endl;
    //   log_warn_ << "No timers registered" << std::endl;
    //   return;
    // }
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

    auto min = [](const std::vector<DurationT>& records)->DurationT{
      DurationT min_duration = DurationT::max();
      for(const auto& record : records) {
        if(record < min_duration) min_duration = record;
      }
      return min_duration;
    };

    auto max = [](const std::vector<DurationT>& records)->DurationT{
      DurationT max_duration = DurationT::min();
      for(const auto& record : records) {
        if(record > max_duration) max_duration = record;
      }
      return max_duration;
    };

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

    auto stddev = [&](const std::vector<DurationT>& records)->DurationT{
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

    out << "Statistics:\n";

    for(const auto& timer : timers) {
      out << "  [" << timer.first << "(count: " << timer.second.size() <<")] | ";

      // min
      out << "Min: "
          << std::chrono::duration_cast<std::chrono::microseconds>(
                  min(timer.second)).count()*1e-3 << "ms | ";

      // max
      out << "Max: "
          << std::chrono::duration_cast<std::chrono::microseconds>(
                  max(timer.second)).count()*1e-3 << "ms | ";

      // average
      out << "Avg: "
          << std::chrono::duration_cast<std::chrono::microseconds>(
                  avg(timer.second)).count()*1e-3 << "ms | ";

      // stddev
      out << "Stddev: "
          << std::chrono::duration_cast<std::chrono::microseconds>(
                  stddev(timer.second)).count()*1e-3 << "ms | ";


      out << std::endl;
    }
    // out << std::endl;
  }


  // /**
  //  * @brief Disable all timers
  // */
  // void disableTimers(){
  //   for(auto& timer : timers_){
  //     timer.second.enable = false;
  //   }
  // }

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

 private:

  std::mutex mtx_;
  size_t max_timer_record_size_{0};

  std::ostream& log_info_;
  std::ostream& log_warn_;
  std::ostream& log_err_;

  // std::map<std::string, TimerRegPtr> timers_;
  std::set<std::string> inactive_timers_;
  std::stack<TimerPtr> timer_stack_;
  std::vector<Record> records_;
};

} // namespace time_statistic_ros
