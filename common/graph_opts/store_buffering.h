#ifndef _STORE_BUFFERING_H_
#define _STORE_BUFFERING_H_

#include "base_opt.h"

class StoreBuffering : public BaseAladdinOpt {
 public:
  using BaseAladdinOpt::BaseAladdinOpt;
  virtual void optimize();
  virtual std::string getCenteredName(size_t size);
};

#endif
