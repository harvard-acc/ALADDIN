#ifndef _LOAD_BUFFERING_H_
#define _LOAD_BUFFERING_H_

#include "base_opt.h"

class LoadBuffering : public BaseAladdinOpt {
 public:
  using BaseAladdinOpt::BaseAladdinOpt;
  virtual void optimize();
  virtual std::string getCenteredName(size_t size);
};

#endif
