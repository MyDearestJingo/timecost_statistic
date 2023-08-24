#include<unistd.h>
#include<fstream>
#include<boost/filesystem.hpp>

// #define DISABLE_TIMECOST_STATISTIC
#include "timecost_statistic/timer_manager.hpp"
#include "timecost_statistic/mermaid_plotter.hpp"
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

  for(int i=0; i < 3; ++i){
    TS_START()
    foo1();
    TS_START()
    foo2();
    TS_END()
    TS_END()

    if(i%2==0) {
      TS_START()
      foo3();
      TS_END();
    }
  }

  TS_EXPORT_TO_TXT("/home/ylan3982/agioe/ros2_ws/src/timecost_statistic/test/temp/records.txt");

  MermaidPlotter plotter;
  plotter.init(timecost_statistic::TimerManager::getInstance().getRecords());
  std::ofstream mermaid_file;
  mermaid_file.open("/home/ylan3982/agioe/ros2_ws/src/timecost_statistic/graph/test.md");

  plotter.exportGraph(mermaid_file);
  mermaid_file.close();

  return 0;
}
