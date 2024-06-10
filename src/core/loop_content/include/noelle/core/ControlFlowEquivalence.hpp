/*
 * Copyright 2016 - 2019  Angelo Matni, Simone Campanoni
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights to
 use, copy, modify, merge, publoopsh, distribute, sublicense, and/or sell copies
 of the Software, and to permit persons to whom the Software is furnished to do
 so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
 OR OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef NOELLE_SRC_CORE_LOOP_CONTENT_CONTROLFLOWEQUIVALENCE_H_
#define NOELLE_SRC_CORE_LOOP_CONTENT_CONTROLFLOWEQUIVALENCE_H_

#include <memory>
#include "arcana/noelle/core/SystemHeaders.hpp"
#include "arcana/noelle/core/Dominators.hpp"
#include "arcana/noelle/core/LoopForest.hpp"

namespace arcana::noelle {

class ControlFlowEquivalence {
public:
  ControlFlowEquivalence(const DominatorSummary *ds,
                         LoopTree *loops,
                         Function &F);

  ControlFlowEquivalence(const DominatorSummary *ds,
                         LoopTree *loops,
                         const LoopStructure *loopStructure);

  std::unordered_set<BasicBlock *> getEquivalences(BasicBlock *bb) const;

  raw_ostream &print(raw_ostream &stream, std::string prefixToUse = "") const;

private:
  void calculateControlFlowEquivalences(const DominatorSummary *DS,
                                        LoopTree *loops);

  void createEquivalenceSet(BasicBlock *singleB);

  void mergeEquivalenceSets(BasicBlock *a, BasicBlock *b);

  std::set<BasicBlock *> startBBs, endBBs;

  std::unordered_set<std::unique_ptr<std::unordered_set<BasicBlock *>>>
      equivalentBBs;
  std::unordered_map<BasicBlock *, std::unordered_set<BasicBlock *> *>
      bbToEquivalence;
};
} // namespace arcana::noelle

#endif // NOELLE_SRC_CORE_LOOP_CONTENT_CONTROLFLOWEQUIVALENCE_H_
