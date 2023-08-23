#include <chrono>
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <set>
#include <iostream>
#include <sstream>
// #include <stack>
// using namespace std::chrono_literals;
// using namespace std::chrono;

// #define __DEBUG__

namespace timecost_statistic
{
using ClockType = std::chrono::system_clock;
using TimePointT = std::chrono::time_point<ClockType>;
using DurationT = std::chrono::microseconds;

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
    log_info_.precision(6);
  };

  // ~TimerManager();

  /**
   *
  */
  inline bool registerTimer(const std::string &id) {
    timers_.insert(std::pair<std::string, Timer>(id,{}));
    log_info_ << "Registered timer \""<< id << "\" " << std::endl;
    return true;
  }

  /**
   * @brief
  */
  inline bool startTimer(
      const std::string& id,
      const TimePointT* beginning=nullptr,
      bool create_if_not_existed=true)
  {
#ifndef DISABLE_ALL_TIMERS
    auto timer = timers_.find(id);
    if(timer == timers_.end() && create_if_not_existed) {
      registerTimer(id);
      timer = timers_.find(id);
    } else if (!create_if_not_existed) {
      log_err_ << "Timer with ID \"" << id << "\" does not exist" << std::endl;
      return false;
    }

    timer->second.ticking = true;
    timer->second.beginning =
        beginning ? *beginning : std::chrono::high_resolution_clock::now();
#endif

    return true;
  }

  inline bool endTimer(
      const std::string& id,
      const TimePointT ending=std::chrono::high_resolution_clock::now())
  {
#ifndef DISABLE_ALL_TIMERS
    auto timer = timers_.find(id);
    if(timer == timers_.end()) {
      log_err_ << "Timer with ID \"" << id << "\" does not exist" << std::endl;
      return false;
    } else if (!timer->second.ticking) {
      log_err_ << "Timer with ID \"" << id << "\" is not ticking" << std::endl;
      log_err_ << "Call startTimer with this ID before calling endTimer"
               << std::endl;
      return false;
    }

    auto& records = timer->second.records;
    if(records.size() == max_timer_record_size_) {
      ++max_timer_record_size_;
    } else {
      while(records.size() < max_timer_record_size_ - 1) {
        records.push_back(nullptr);
      }
    }

    records.push_back(new DurationT(
        std::chrono::duration_cast<DurationT>(
            ending - timer->second.beginning)));
#endif
    return true;
  }

  std::string flattenRecords(){
    std::stringstream out;
    flattenRecords(out);
    log_info_ << out.str();
    return out.str();
  }

  void flattenRecords(std::stringstream& out){
    out.precision(6);

#ifdef __DEBUG__
    for(const auto &timer : timers_) {
      out << timer.first << "(" << timer.second.records.size() <<")" << ", ";
    }
    out << std::endl;
#endif

    for(size_t i = 0; i < max_timer_record_size_; ++i){
      out << "Record #" << i <<": " << std::endl << "\t";
      for(const auto &timer : timers_) {
        out << timer.first << "(" << timer.second.records.size() <<")" << ": ";
        if(timer.second.records.size() <= i
           || timer.second.records[i]==nullptr)
        {
          out << "nan | ";
        } else {
          out << std::chrono::duration_cast<std::chrono::microseconds>(
                    *timer.second.records[i]).count()*1e-3 << "ms | ";
        }
      }
      out << std::endl;
    }
  }

  /**
   * @brief Disable all timers
  */
  void disableTimers(){
    for(auto& timer : timers_){
      timer.second.enable = false;
    }
  }

  /**
   * @brief Disable timers with IDs specified
   * @param ids specified IDs of timers to disable
   */
  void disableTimers(const std::vector<std::string>& ids){
    for(const auto& id : ids) {
      auto timer = timers_.find(id);
      if(timer != timers_.end()) {
        timer->second.enable = false;
      }
    }
  }

  /**
   * @brief Enable all timers
  */
  void enableTimers(){
    for(auto& timer : timers_){
      timer.second.enable = true;
    }
  }

  /**
   * @brief Enable timers with IDs specified
   * @param ids specified IDs of timers to enable
   */
  void enableTimers(const std::vector<std::string>& ids){
    for(const auto& id : ids) {
      auto timer = timers_.find(id);
      if(timer != timers_.end()) {
        timer->second.enable = true;
      }
    }
  }

 private:

  std::mutex mtx_;
  size_t max_timer_record_size_{0};

  std::ostream& log_info_;
  std::ostream& log_warn_;
  std::ostream& log_err_;

  struct Timer{
    bool ticking{false};
    bool enable{false};
    TimePointT beginning;
    std::vector<DurationT*> records;
  };

  std::map<std::string, Timer> timers_;
};

} // namespace time_statistic_ros
