#ifndef YAML_EXPORTER_HPP_
#define YAML_EXPORTER_HPP_

#include <string>
#include <vector>
#include <fstream>

#include "yaml-cpp/yaml.h"
#include "timer_manager.hpp"

namespace timecost_statistic
{

inline std::vector<std::string> split_str(const std::string& s, const char delimiter) {
  size_t start;
  size_t end = 0;
  std::vector<std::string> ret;
  while ((start = s.find_first_not_of(delimiter, end)) != std::string::npos) {
    end = s.find(delimiter, start);
    ret.push_back(s.substr(start, end - start));
  }
  return ret;
}

class YamlExporter {
 public:
  void dump(const std::vector<Record>& data, const std::string& path);

 private:
  bool load_yaml(const std::string& f, YAML::Node& n);
  bool save_yaml(const std::string& f, YAML::Node& n);
};

} // namespace timecost_statistic

#ifdef DISABLE_TIMECOST_STATISTIC

#define TS_EXPORT_TO_YAML(filepath)

#else

#define TS_EXPORT_TO_YAML(filepath) \
  do { \
    timecost_statistic::YamlExporter exporter; \
    exporter.dump(timecost_statistic::TimerManager::getInstance().getRecords(), filepath); \
  } while (0);

#endif

#endif