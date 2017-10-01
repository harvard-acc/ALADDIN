#ifndef _CONSECUTIVE_BRANCH_FUSION_H_
#define _CONSECUTIVE_BRANCH_FUSION_H_

#include <list>
#include <set>

#include "base_opt.h"

class ConsecutiveBranchFusion : public BaseAladdinOpt {
 public:
  using BaseAladdinOpt::BaseAladdinOpt;
  virtual void optimize();
  virtual std::string getCenteredName(size_t size);

 protected:
  void findBranchChain(ExecNode* root,
                       std::list<ExecNode*>& branch_chain,
                       std::set<Edge>& to_remove_edges);
};

#endif
