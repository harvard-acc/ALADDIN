#ifndef _PER_LOOP_PIPELINING_H_
#define _PER_LOOP_PIPELINING_H_

#include "base_opt.h"

class PerLoopPipelining : public BaseAladdinOpt {
 public:
  using BaseAladdinOpt::BaseAladdinOpt;
  virtual void optimize();
  virtual std::string getCenteredName(size_t size);
};

#endif

