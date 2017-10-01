#ifndef _REPEATED_STORE_REMOVAL_H_
#define _REPEATED_STORE_REMOVAL_H_

#include "base_opt.h"

class RepeatedStoreRemoval : public BaseAladdinOpt {
 public:
  using BaseAladdinOpt::BaseAladdinOpt;
  virtual void optimize();
  virtual std::string getCenteredName(size_t size);
};

#endif

