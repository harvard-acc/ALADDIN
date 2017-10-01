#ifndef _TREE_HEIGHT_REDUCTION_H_
#define _TREE_HEIGHT_REDUCTION_H_

#include "base_opt.h"

class TreeHeightReduction : public BaseAladdinOpt {
 public:
  using BaseAladdinOpt::BaseAladdinOpt;
  virtual void optimize();
  virtual std::string getCenteredName(size_t size);

 protected:
  void findMinRankNodes(ExecNode** node1,
                        ExecNode** node2,
                        std::map<ExecNode*, unsigned>& rank_map);

  unsigned begin_node_id;
  unsigned end_node_id;
};

#endif
