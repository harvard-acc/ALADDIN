#ifndef _INDUCTION_DEPENDENCE_REMOVAL_H_
#define _INDUCTION_DEPENDENCE_REMOVAL_H_

#include "base_opt.h"

class InductionDependenceRemoval : public BaseAladdinOpt {
 public:
  using BaseAladdinOpt::BaseAladdinOpt;
  virtual void optimize();
  virtual std::string getCenteredName(size_t size);
};

#endif
