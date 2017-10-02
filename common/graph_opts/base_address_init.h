#ifndef _BASE_ADDRESS_INIT_H_
#define _BASE_ADDRESS_INIT_H_

#include "base_opt.h"

class BaseAddressInit : public BaseAladdinOpt {
 public:
  using BaseAladdinOpt::BaseAladdinOpt;
  virtual void optimize();
  virtual std::string getCenteredName(size_t size);
};

#endif

