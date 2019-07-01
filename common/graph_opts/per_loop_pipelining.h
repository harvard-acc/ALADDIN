#ifndef _PER_LOOP_PIPELINING_H_
#define _PER_LOOP_PIPELINING_H_

#include "base_opt.h"

class PerLoopPipelining : public BaseAladdinOpt {
 public:
  using BaseAladdinOpt::BaseAladdinOpt;
  void findAndPipelineLoops(LoopIteration* loop,
                            std::set<Edge>& to_remove_edges,
                            std::vector<NewEdge>& to_add_edges);
  virtual void optimize();
  void optimize(std::list<LoopIteration*>& loops,
                std::set<Edge>& to_remove_edges,
                std::vector<NewEdge>& to_add_edges);
  virtual std::string getCenteredName(size_t size);
};

#endif

