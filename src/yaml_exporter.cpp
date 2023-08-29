#include "timecost_statistic/yaml_exporter.hpp"

namespace timecost_statistic
{

void YamlExporter::dump(const std::vector<Record>& data, const std::string& path) {
  YAML::Node n;
  for (const auto& record : data) {
    for (int i = record.timers.size() - 1; i >=0; --i) {
      const auto& timer = record.timers[i];
      std::string full_name = timer->path;
      if (full_name.substr(0, 1) == "/") {
        full_name.erase(0, 1);
      }
      std::vector<YAML::Node> v_node;
      v_node.push_back(n[std::string("#") + std::to_string(record.id)]);
      auto v_full_name = split_str(full_name, '/');
      for (const auto& sub_name : v_full_name) {
        YAML::Node node = v_node.back()[std::string("[R]") + sub_name];
        v_node.push_back(node);
      }
      v_node.back()["time"] = std::to_string((timer->duration.count() * 1e-3)) + "ms";
    }
  }
  save_yaml(path, n);
}

bool YamlExporter::load_yaml(const std::string& f, YAML::Node& n) {
  try {
    n = YAML::LoadFile(f);
  } catch(YAML::BadFile& e) {
    std::cerr << "Failed to load " << f << "error:" << e.what() << std::endl;
    return false;
  }
  return true;
}

bool YamlExporter::save_yaml(const std::string& f, YAML::Node& n) {
  std::ofstream ofs(f.c_str());
  if (!ofs) {
    std::cerr << "File " << f << " not found" << std::endl;
    return false;
  }
  ofs << YAML::Dump(n);
  ofs.close();
  return true;
}

} // namespace timecost_statistic
