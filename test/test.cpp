#include<unistd.h>
#include<fstream>

// #define DISABLE_ALL_TIMERS
#include "timecost_statistic/timer_manager.hpp"
using namespace timecost_statistic;
using namespace std::chrono_literals;

void foo1(){
  std::cout << "foo1 running" << std::endl;
  sleep(0);
  // sleep(0.1111);
}

void foo2(){
  std::cout << "foo2 running" << std::endl;
  sleep(0);


  // sleep(0.2222);
}

void foo3(){
  std::cout << "foo3 running" << std::endl;
  sleep(0);
  // sleep(0.3333);
}

int main(int argc, char** argv) {
  TimerManager timers(std::cout, std::cout, std::cerr);

  for(int i=0; i < 102; ++i){
    timers.startTimer("foo1_timer");
    foo1();
    timers.startTimer("foo2_timer");
    foo2();
    timers.endTimer();
    timers.endTimer();
    timers.enableTimers({"foo1_timer"});


    if(i%2==0) {
      timers.startTimer("foo3_timer");
      foo3();
      timers.endTimer();
      timers.disableTimers({"foo1_timer"});
    }
  }

  timers.calcStatistic();
  timers.flattenRecords();

  std::ofstream file_export;
  std::string export_path;
  if(argc > 1) {
    export_path = argv[0];
  } else {
    export_path =
      "/home/ylan3982/agioe/ros2_ws/src/timecost_statistic/test/temp/records.txt";
    std::cout << "Using default export path: " << export_path << std::endl;
  }

  file_export.open(export_path);
  timers.exportRecords(file_export);
  file_export.close();

  return 0;
}