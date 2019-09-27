#ifndef _DMA_BASE_ADDRESS_INIT_H_
#define _DMA_BASE_ADDRESS_INIT_H_

#include "base_opt.h"

class DmaBaseAddressInit : public BaseAladdinOpt {
 public:
  using BaseAladdinOpt::BaseAladdinOpt;
  virtual void optimize();
  virtual std::string getCenteredName(size_t size);

 private:
  // Set the memory type of this host memory access node.
  void setHostMemoryType(ExecNode* node);
};

#endif
