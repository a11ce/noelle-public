#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include <algorithm>

#include "arcana/noelle/core/Noelle.hpp"

using namespace arcana::noelle;

namespace {

struct CAT : public ModulePass {
  static char ID;

  CAT() : ModulePass(ID) {}

  bool doInitialization(Module &M) override {
    return false;
  }

  bool runOnModule(Module &M) override {

    /*
     * Fetch NOELLE
     */
    auto &noelle = getAnalysis<Noelle>();
    auto fm = noelle.getFunctionsManager();

    /*
     * Check if we have profiles.
     */
    auto hot = noelle.getProfiles();
    if (!hot->isAvailable()) {
      return false;
    }
    errs() << "The profiler is available\n";

    /*
     * Print the profiles for each function.
     */
    for (auto F : fm->getFunctionsWithBody()) {

      /*
       * Check if the function has been executed at all.
       */
      auto executed = hot->hasBeenExecuted(F);
      if (!executed) {
        continue;
      }

      /*
       * The function has been executed.
       * Print its profile.
       */
      errs() << "Function: \"" << F->getName()
             << "\": Invocations = " << hot->getInvocations(F) << "\n";
      errs() << "Function: \"" << F->getName()
             << "\": Static      = " << hot->getStaticInstructions(F) << "\n";
      errs() << "Function: \"" << F->getName()
             << "\": Self        = " << hot->getSelfInstructions(F) << "\n";
      errs() << "Function: \"" << F->getName()
             << "\": Total       = " << hot->getTotalInstructions(F) << "\n";
      errs() << "Function: \"" << F->getName() << "\": Coverage    = "
             << (hot->getDynamicTotalInstructionCoverage(F)) * 100 << "%\n";

      for (auto &inst : instructions(F)) {
        if (isa<LoadInst>(&inst) || isa<StoreInst>(&inst)) {
          errs() << "  Memory instruction: \"" << inst << "\"\n";
          errs() << "    Self = " << hot->getSelfInstructions(&inst) << "\n";
          errs() << "    Total = " << hot->getTotalInstructions(&inst) << "\n";
          errs() << "    Coverage = "
                 << (hot->getDynamicTotalInstructionCoverage(F)) * 100 << "%\n";
          assert(hot->getSelfInstructions(&inst) < hot->getSelfInstructions(F));
          assert(hot->getTotalInstructions(&inst)
                 < hot->getTotalInstructions(F));
        }
      }
    }

    /*
     * Fetch LoopContent for all program loops.
     */
    auto loops = noelle.getLoopContents();

    /*
     * Use the profiles.
     */
    for (auto loop : *loops) {

      /*
       * Print the loop ID.
       */
      auto LS = loop->getLoopStructure();
      auto loopFunction = LS->getFunction();
      auto entryInst = LS->getEntryInstruction();
      errs() << "Loop:\n";
      errs() << "  " << loopFunction->getName() << "\n";
      errs() << "  " << *entryInst << "\n";
      errs() << "  Self  = " << hot->getSelfInstructions(LS) << "\n";
      errs() << "  Total = " << hot->getTotalInstructions(LS) << "\n";

      /*
       * Print loop statistics.
       */
      errs() << "    Number of invocations of the loop = "
             << hot->getInvocations(LS) << "\n";
      errs() << "    Average number of iterations per invocations = "
             << hot->getAverageLoopIterationsPerInvocation(LS) << "\n";
      errs() << "    Average number of total instructions per invocations = "
             << hot->getAverageTotalInstructionsPerInvocation(LS) << "\n";
      errs() << "    Coverage in terms of total instructions = "
             << (hot->getDynamicTotalInstructionCoverage(LS) * 100) << "%\n";
    }

    return false;
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<Noelle>();
  }
};
} // namespace

// Next there is code to register your pass to "opt"
char CAT::ID = 0;
static RegisterPass<CAT> X("CAT", "Simple user of the Noelle framework");

// Next there is code to register your pass to "clang"
static CAT *_PassMaker = NULL;
static RegisterStandardPasses _RegPass1(PassManagerBuilder::EP_OptimizerLast,
                                        [](const PassManagerBuilder &,
                                           legacy::PassManagerBase &PM) {
                                          if (!_PassMaker) {
                                            PM.add(_PassMaker = new CAT());
                                          }
                                        }); // ** for -Ox
static RegisterStandardPasses _RegPass2(
    PassManagerBuilder::EP_EnabledOnOptLevel0,
    [](const PassManagerBuilder &, legacy::PassManagerBase &PM) {
      if (!_PassMaker) {
        PM.add(_PassMaker = new CAT());
      }
    }); // ** for -O0
