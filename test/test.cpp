#include<unistd.h>

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
  TimerManager timers;

  for(int i=0; i < 3; ++i){
    timers.startTimer("foo1_timer");
    foo1();
    timers.endTimer("foo1_timer");

    timers.startTimer("foo2_timer");
    foo2();
    timers.endTimer("foo2_timer");

    if(i%2==0) {
      timers.startTimer("foo3_timer");
      foo3();
      timers.endTimer("foo3_timer");
    }
  }

  timers.calcStatistic();
  timers.flattenRecords();

  return 0;
}