/*
 * Copyright 2016 - 2019  Angelo Matni, Simone Campanoni
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights to
 use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
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
#pragma once

#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/ScalarEvolutionExpressions.h"

#include "arcana/noelle/core/PDG.hpp"
#include "arcana/noelle/core/SCC.hpp"
#include "arcana/noelle/core/SCCDAG.hpp"
#include "arcana/noelle/core/PDGGenerator.hpp"
#include "arcana/noelle/core/PDGPrinter.hpp"
#include "arcana/noelle/core/InductionVariables.hpp"
#include "arcana/noelle/core/Invariants.hpp"
#include "arcana/noelle/core/LoopGoverningInductionVariable.hpp"

#include "TestSuite.hpp"

#include <sstream>
#include <vector>
#include <string>

using namespace parallelizertests;

namespace arcana::noelle {

class IVAttrTestSuite : public ModulePass {
public:
  IVAttrTestSuite() : ModulePass{ ID } {}

  /*
   * Class fields
   */
  static char ID;
  static const char *tests[];
  static parallelizertests::TestFunction testFns[];

  bool doInitialization(Module &M) override;
  bool runOnModule(Module &M) override;
  void getAnalysisUsage(AnalysisUsage &AU) const override;

private:
  static Values verifyStartAndStepByLoop(ModulePass &pass, TestSuite &suite);
  static Values verifyIntermediateValues(ModulePass &pass, TestSuite &suite);
  static Values verifyLoopGoverning(ModulePass &pass, TestSuite &suite);

  TestSuite *suite;
  Module *M;
  ScalarEvolution *SE;
  LoopInfo *LI;
  LoopTree *topLoop;
  PDG *fdg;
  SCCDAG *sccdag;
  InductionVariableManager *IVs;
};
} // namespace arcana::noelle
