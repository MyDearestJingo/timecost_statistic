#include <chrono>
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <set>
#include <iostream>
// #include <stack>
// using namespace std::chrono_literals;
// using namespace std::chrono;


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
 */
class TimerManager{
 public:
  TimerManager()
    : log_info_(std::cout), log_warn_(std::cout), log_err_(std::cerr)
  {};

  // ~TimerManager();

  bool registerTimer(const std::string &id) {
    timers_.insert(std::pair<std::string, Timer>(id,{}));
    return true;
  }

  inline bool startTimer(
      const std::string& id,
      const TimePointT* beginning=nullptr,
      bool create_if_not_existed=true)
  {
    auto timer = timers_.find(id);
    if(timer == timers_.end() && create_if_not_existed) {
      // create a new timer with the id
      timers_.insert(std::pair<std::string, Timer>(id,{}));
      timer = timers_.find(id);
    } else if (!create_if_not_existed) {
      log_err_ << "Timer with ID \"" << id << "\" does not exist" << std::endl;
      return false;
    }

    timer->second.ticking = true;
    timer->second.beginning =
        beginning ? *beginning : std::chrono::high_resolution_clock::now();

    return true;
  }

  inline bool endTimer(
      const std::string& id,
      const TimePointT ending=std::chrono::high_resolution_clock::now())
  {
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

    return true;
  }

  void flattenRecords(){
    for(size_t i = 0; i < max_timer_record_size_; ++i){
      log_info_ << "Record #" << i <<": " << std::endl << "\t";
      for(const auto &timer : timers_) {
        log_info_ << timer.first << ": ";
        if(timer.second.records[i]==NULL) log_info_ << "nan | ";
        else
          log_info_ << std::chrono::duration_cast<std::chrono::microseconds>(
              *timer.second.records[i]).count()*1e-3 << "ms | ";
      }
      log_info_ << std::endl;
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
      if(timers_.find(id) != timers_.end()) {

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
