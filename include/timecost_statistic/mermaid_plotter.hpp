#include <memory>
#include <fstream>
#include <vector>
#include <set>
#include <string>
#include <boost/filesystem.hpp>
#include "timecost_statistic/timer_manager.hpp"

namespace timecost_statistic
{

struct RecordTreeNode;
using RecordTreeNodePtr = std::shared_ptr<RecordTreeNode>;

struct RecordTreeNode {
  const std::string name;
  std::vector<DurationT> records;
  std::vector<RecordTreeNodePtr> children;
  float proportion{1.0};

  RecordTreeNode(const TimerPtr timer) : name(timer->name){
    records.push_back(timer->duration);
  }

  RecordTreeNode(const std::string& name) : name(name) {}

  void calcAvgProportion() {
    DurationT avg = std::accumulate(records.begin(),
                                    records.end(), DurationT(0)
                    ) / records.size();
    for(auto& child : children) {
      child->proportion = float(std::accumulate(child->records.begin(),
                                                child->records.end(),
                                                DurationT(0)).count()
                         ) / child->records.size() / avg.count();
    }
  }
};

struct RecordTree;
using RecordTreePtr = std::shared_ptr<RecordTree>;

struct RecordTree {
  RecordTreeNodePtr root;
  size_t num;
};

class MermaidPlotter {
  const std::string ROOT_ENC_H = "((\"`";   // root node enclosure head
  const std::string ROOT_ENC_T = "`\"))";   // root node enclosure tail
  const std::string NODE_ENC_H = "[\"`";    // normal node enclosure head
  const std::string NODE_ENC_T = "`\"]";    // normal node enclosure head
  const std::string CHLINE = "<br/>";       // line-change symbol in mermaid

 public:
  void init(const std::vector<Record>& records) {
    std::cout << "Init from " << records.size() << " records" << std::endl;
    for(const auto& record : records) {
      RecordTreePtr found(nullptr);
      for(const auto& tree : trees_) {
        if(sameStructure(record, *tree)) {
          found = tree;
          break;
        }
      }

      // not found, create a new tree
      if(!found) {
        found = createTree(record.timers);
        trees_.push_back(found);
      }

      // fill the tree
      for(const auto& timer : record.timers) {
        RecordTreeNodePtr node = getTreeNode(
            boost::filesystem::path(timer->path).begin(), found->root, false);
        node->records.push_back(timer->duration);
      }
    }
    std::cout << "Generated " << trees_.size() << " trees"
              << " from " << records.size() << " records" << std::endl;

    // for(const auto& tree : trees_) {
    //   std::cout << "Tree contains " << tree->root->records.size()
    //             << " duration records" << std::endl;
    // }
    // printAllTrees();
  }

  /**
   * @brief Check if the record has the same structure with the tree
   * @param record the input record
   * @param tree the root node of the tree compared
   * @return true or not
   **/
  bool sameStructure(const Record& record, const RecordTree& tree) {
    if(record.timers.size() != tree.num) {
      // std::cout << "Unequal number of nodes" << std::endl;
      return false;
    }

    for(const auto& timer : record.timers) {
      boost::filesystem::path path(timer->path);
      if(!getTreeNode(path.begin(), tree.root, false)) {
        // std::cout << "Failed to find node at path: " << path.generic_string() << std::endl;
        return false;
      }
    }
    return true;
  }

  bool exportMermaidFile(std::ofstream& mermaid_file) {
    for(const auto& tree : trees_){
      mermaid_file << "```mermaid\nmindmap" << std::endl;
      exportTree(mermaid_file, tree->root, 0);
      mermaid_file << "```\n" << std::endl;
    }

    return true;
  }

  void printAllTrees() {
    for(const auto& tree : trees_) {
      std::cout << "Tree with root: " << tree->root->name << std::endl;
      preorderTraverse(tree->root);
      std::cout << std::endl;
    }
  }

  void preorderTraverse(const RecordTreeNodePtr tree) {
    std::cout << tree->name << std::endl;
    for(const auto& child : tree->children) {
      preorderTraverse(child);
    }
  }

 private:
  void exportTree(std::ofstream& out, const RecordTreeNodePtr root, int level){
    // pre-order traversal
    if(level < 1){
      out << getIndention(level+1)
          << ROOT_ENC_H << stringNodeInfo(root) << ROOT_ENC_T
          << std::endl;
    } else {
      out << getIndention(level+1)
          << NODE_ENC_H << stringNodeInfo(root) << NODE_ENC_T << std::endl;
    }
    for(const auto& child : root->children) {
      exportTree(out, child, level+1);
    }
    // out << std::endl;
  }

  /**
   * @brief output information of the timer given by the node
   * @todo complete usage of arg flag
   * @param node
   * @param flag option for indicating what info to output
   * @return
   */
  std::string stringNodeInfo(const RecordTreeNodePtr node, int flag=0xf){
    const auto& records = node->records;
    std::stringstream ss;
    ss << node->name;

    ss.precision(3);

    // if output count
    ss << '(' << node->records.size() << ')' << CHLINE;

    // if output min
    ss << "Min: "
        << std::chrono::duration_cast<std::chrono::microseconds>(
              *std::min_element(records.begin(), records.end())
            ).count()*1e-3
        << ", ";

    // if output max
    ss << "Max: "
        << std::chrono::duration_cast<std::chrono::microseconds>(
              *std::max_element(records.begin(), records.end())
            ).count()*1e-3
        << ", ";

    // if output avg
    ss << "Avg: "
        << std::chrono::duration_cast<std::chrono::microseconds>(
              std::accumulate(records.begin(), records.end(), DurationT(0))
            ).count()*1e-3 / records.size()
        << ", ";

    // if output stddev
    ss << "Stddev: "
        << std::chrono::duration_cast<std::chrono::microseconds>(
              TimerManager::stddev(records)).count()*1e-3 << ", ";

    // if output portion
    node->calcAvgProportion();
    ss << "Avg proportion: " << node->proportion * 100.0 << "\%";

    return ss.str();
  }

  RecordTreeNodePtr getTreeNode(
      const boost::filesystem::path::iterator path_iter,
      RecordTreeNodePtr tree, bool create_if_not_existing=true)
  {
    if(path_iter->empty() || !tree) return tree;

    RecordTreeNodePtr found(nullptr);
    if(path_iter->string() == tree->name) {
      auto next = path_iter; ++next;
      if(next->empty()) {
        found = tree;
      } else {
        for(auto& child : tree->children) {
          found = getTreeNode(next, child, create_if_not_existing);
          if(found) break;
        }
      }
    }

    if(!create_if_not_existing) return found;

    // not found, so create a new branch to this node
    // std::cout << "Creating new branch for path: ";
    // for(auto iter = path_iter; !iter->empty(); ++iter) {
    //   std::cout << iter->string() << "/";
    // }
    // std::cout << std::endl;
    auto next = path_iter;
    return createBranch(++next, tree);
  }

  RecordTreeNodePtr createBranch(
      boost::filesystem::path::iterator path_iter, RecordTreeNodePtr parent)
  {
    if(path_iter->empty()) return parent;
    RecordTreeNodePtr new_node(new RecordTreeNode(path_iter->c_str()));
    parent->children.push_back(new_node);
    // std::cout << "Add child node \"" << new_node->name << "\" to parent \"" << parent->name << "\"" << std::endl;
    return createBranch(++path_iter, new_node);
  }

  inline std::string getIndention(int level, int base_length=2) {
    return std::string(level*base_length, ' ');
  }

  RecordTreePtr createTree(const std::vector<TimerPtr>& timers) {
    if(timers.empty()) return nullptr;

    // create the root node of the new tree
    std::string root_name =
        boost::filesystem::path(timers.front()->path).begin()->c_str();
    RecordTreePtr new_tree(new RecordTree);
    new_tree->root.reset(new RecordTreeNode(root_name));
    // std::cout << "Creating a new tree with root name: " << root_name << std::endl;

    // create branches
    for(const auto& timer : timers) {
      getTreeNode(
        boost::filesystem::path(timer->path).begin(), new_tree->root, true);
    }

    new_tree->num = timers.size();

    return new_tree;
  }

 private:
  std::vector<RecordTreePtr> trees_;
};

} // namespace timecost_statistic
