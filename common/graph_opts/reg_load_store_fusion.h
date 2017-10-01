#ifndef _REG_LOAD_STORE_FUSION_H_
#define _REG_LOAD_STORE_FUSION_H_

#include "base_opt.h"

class RegLoadStoreFusion : public BaseAladdinOpt {
 public:
  using BaseAladdinOpt::BaseAladdinOpt;
  virtual void optimize();
  virtual std::string getCenteredName(size_t size);
};

#endif
