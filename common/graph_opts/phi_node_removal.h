#ifndef _REMOVE_PHI_NODES_H_
#define _REMOVE_PHI_NODES_H_

#include "base_opt.h"

class PhiNodeRemoval : public BaseAladdinOpt {
 public:
  using BaseAladdinOpt::BaseAladdinOpt;
  virtual void optimize();
  virtual std::string getCenteredName(size_t size);
};

#endif
