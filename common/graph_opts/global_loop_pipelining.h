#ifndef _GLOBAL_LOOP_PIPELINING_H_
#define _GLOBAL_LOOP_PIPELINING_H_

#include "base_opt.h"

class GlobalLoopPipelining : public BaseAladdinOpt {
 public:
  using BaseAladdinOpt::BaseAladdinOpt;
  virtual void optimize();
  virtual std::string getCenteredName(size_t size);
};

#endif


