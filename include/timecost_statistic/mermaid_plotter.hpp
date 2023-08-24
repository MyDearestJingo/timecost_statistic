#include <memory>
#include <fstream>
#include <vector>
#include <boost/filesystem.hpp>
#include "timecost_statistic/timer_manager.hpp"

namespace timecost_statistic
{

struct TimerNode;
using TimerNodePtr = std::shared_ptr<TimerNode>;

struct TimerNode {
  const std::string name;
  std::vector<Timer> records;
  std::vector<TimerNodePtr> children;

  TimerNode(const TimerPtr timer) : name(timer->name)
  { records.push_back(*timer); }

  TimerNode(const std::string& name) : name(name) {}
};

class MermaidPlotter {
 public:
  void init(const std::vector<Record>& records) {
    std::cout << "Init from " << records.size() << " records" << std::endl;
    for(const auto& record : records) {
      for(const auto& timer : record.timers) {
        TimerNodePtr node = getTreeNode(timer->path);
        node->records.push_back(*timer);
      }
    }
    std::cout << "Generate " << trees_.size() << " trees" << std::endl;
    // printAllTrees();
  }

  bool exportGraph(std::ofstream& mermaid_file) {
    for(const auto& tree : trees_){
      mermaid_file << "```mermaid\nmindmap" << std::endl;
      exportTree(mermaid_file, tree, 0);
      mermaid_file << "```\n" << std::endl;
    }

    return true;
  }

  void printAllTrees() {
    for(const auto& tree : trees_) {
      std::cout << "Tree with root: " << tree->name << std::endl;
      preorderTraverse(tree);
      std::cout << std::endl;
    }
  }

  void preorderTraverse(const TimerNodePtr tree) {
    std::cout << tree->name << std::endl;
    for(const auto& child : tree->children) {
      preorderTraverse(child);
    }
  }

 private:
  void exportTree(std::ofstream& out, const TimerNodePtr root, int level){
    // pre-order traversal
    if(level < 1){
      out << getIndention(level+1) << "root((" <<root->name << "))" << std::endl;
    } else {
      out << getIndention(level+1) << root->name << std::endl;
    }
    for(const auto& child : root->children) {
      exportTree(out, child, level+1);
    }
    // out << std::endl;
  }

  TimerNodePtr getTreeNode(const boost::filesystem::path& path)
  {
    TimerNodePtr node(nullptr);
    std::string root_name = path.begin()->c_str();
    // std::cout << "Searching path: " << path.c_str() << std::endl;

    if(root_name.empty()) return node;

    for(auto& tree : trees_) {
      if(tree->name == root_name) {
        // std::cout << "Find the node with name: " << root_name << std::endl;
        node = getTreeNode(++path.begin(), tree);  // recursive search
        if(node!=nullptr) return node;
      }
    }

    // not found, so create a new branch to this node
    TimerNodePtr new_root(new TimerNode(root_name));
    trees_.push_back(new_root);
    // std::cout << "Creating a new tree with root name: " << new_root->name << std::endl;
    return createBranch(++path.begin(), new_root);
  }

  TimerNodePtr getTreeNode(
      boost::filesystem::path::iterator path_iter, TimerNodePtr tree)
  {
    if(path_iter->empty()) return tree;

    std::string name = path_iter->c_str();

    TimerNodePtr node(nullptr);
    for(auto& child : tree->children) {
      if(child->name == name) {
        auto next = path_iter;
        node = getTreeNode(++next, child);
        if(node != nullptr) return node;
      }
    }

    // not found, so create a new branch to this node
    return createBranch(path_iter, tree);
  }

  TimerNodePtr createBranch(
      boost::filesystem::path::iterator path_iter,
      TimerNodePtr parent)
  {
    if(path_iter->empty()) return parent;
    TimerNodePtr new_node(new TimerNode(path_iter->c_str()));
    parent->children.push_back(new_node);
    // std::cout << "Add child node \"" << new_node->name << "\" to parent \"" << parent->name << "\"" << std::endl;
    return createBranch(++path_iter, new_node);
  }

  inline std::string getIndention(int level, int base_length=2) {
    return std::string(level*base_length, ' ');
  }

 private:
  std::vector<TimerNodePtr> trees_;
};

} // namespace timecost_statistic
