#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include <algorithm>

#include "Noelle.hpp"

using namespace llvm;

namespace {

  struct CAT : public ModulePass {
    static char ID; 
    bool printDependences = false;
    bool printLoops = false;
    bool printDFA = false;

    CAT() : ModulePass(ID) {}

    bool doInitialization (Module &M) override {
      return false;
    }

    bool runOnModule (Module &M) override {

      /*
       * Fetch noelle.
       */
      auto& noelle = getAnalysis<Noelle>();
      errs() << "The program has " << noelle.numberOfProgramInstructions() << " instructions\n";

      /*
       * Fetch the dependence graph of the entry function.
       */
      auto mainF = noelle.getEntryFunction();
      auto FDG = noelle.getFunctionDependenceGraph(mainF);

      /*
       * Print dependences
       */
      if (this->printDependences){
        auto iterF = [](Value *src, DataDependenceType d) -> bool {
          errs() << "   needs " << *src << "\n";
          return false;
        };
        for (auto& inst : instructions(mainF)){
          errs() << inst << "\n";
          FDG->iterateOverDependencesTo(&inst, false, true, true, iterF);
        }
      }

      /*
       * Print loop induction variables and invariant.
       */
      auto loops = noelle.getLoops();
      //auto loops = noelle.getLoops(mainF);
      if (this->printLoops){
        for (auto loop : *loops){
          auto LS = loop->getLoopStructure();
          auto entryInst = LS->getEntryInstruction();
          errs() << "Loop " << *entryInst << "\n";

          /*
           * Induction variables.
           */
          errs() << " Induction variables\n";
          auto IVM = loop->getInductionVariableManager();
          auto GIV = IVM->getLoopGoverningInductionVariable(*LS);
          if (GIV != nullptr){
            errs() << "   GIV: " << *GIV->getLoopEntryPHI()  << "\n";
          }
          auto IVs = IVM->getInductionVariables(*LS);
          for (auto IV : IVs){
            errs() << "   IV: " << *IV->getLoopEntryPHI() << "\n";
          }

          /*
           * Invariants.
           */
          errs() << " Invariants\n";
          auto IM = loop->getInvariantManager();
          for (auto inv : IM->getLoopInstructionsThatAreLoopInvariants()){
            errs() << "   " << *inv << "\n";
          }
        }
      }

      /*
       * Print profiles.
       */
      auto hot = noelle.getProfiles();
      if (hot->isAvailable()){
        errs() << "The profiler is available\n";
        for (auto loop : *loops){
          auto LS = loop->getLoopStructure();
          auto entryInst = LS->getEntryInstruction();
          errs() << " Loop " << *entryInst << "\n";

          for (auto bb : LS->getBasicBlocks()){
            for (auto &inst : *bb){
              errs() << "   [" << hot->getTotalInstructions(&inst) << "] " << inst << "\n";
            }
          }
        }
      }

      /*
       * Data flow analysis
       */
      if (this->printDFA){
        auto dfa = noelle.getDataFlowAnalyses();
        auto dfr = dfa.runReachableAnalysis(mainF);
        errs() << "Data flow reachable analysis\n";
        for (auto& inst : instructions(mainF)){
          errs() << " Next are the instructions reachable from " << inst << "\n";
          auto& outSet = dfr->OUT(&inst);
          for (auto reachInst : outSet){
            errs() << "   " << *reachInst << "\n";
          }
        }

        /*
         * New data flow analysis
         *
         * Fetch the hottest loop.
         */
        noelle.sortByHotness(*loops);
        auto loop = (*loops)[0];
        auto loopStructure = loop->getLoopStructure();
        auto loopHeader = loopStructure->getHeader();
        auto loopFunction = loopStructure->getFunction();

        /*
         * Fetch the data flow engine.
         */
        auto dfe = noelle.getDataFlowEngine();

        /*
         * Define the data flow equations
         */
        auto computeGEN = [loopStructure](Instruction *i, DataFlowResult *df) {
          if (!loopStructure->isIncluded(i)){
            return ;
          }
          auto& gen = df->GEN(i);
          gen.insert(i);
          return ;
        };
        auto computeKILL = [](Instruction *, DataFlowResult *) {
          return ;
        };
        auto computeOUT = [loopHeader](std::set<Value *>& OUT, Instruction *succ, DataFlowResult *df) {

          /*
           * Check if the successor is the header.
           * In this case, we do not propagate the reachable instructions.
           * We do this because we are interested in understanding the reachability of instructions within a single iteration.
           */
          auto succBB = succ->getParent();
          if (succ == &*loopHeader->begin()) {
            return ;
          }

          /*
           * Propagate the data flow values.
           */
          auto& inS = df->IN(succ);
          OUT.insert(inS.begin(), inS.end());
          return ;
        } ;
        auto computeIN = [](std::set<Value *>& IN, Instruction *inst, DataFlowResult *df) {
          auto& genI = df->GEN(inst);
          auto& outI = df->OUT(inst);
          IN.insert(outI.begin(), outI.end());
          IN.insert(genI.begin(), genI.end());
          return ;
        };

        /*
         * Run the data flow analysis
         */
        errs() << "Data flow reachable analysis within loop iteration\n";
        auto customDfr = dfe.applyBackward(loopFunction, computeGEN, computeKILL, computeIN, computeOUT);

        /*
         * Print
         */
        for (auto bb : loopStructure->getBasicBlocks()){
          for (auto &inst : *bb){
            auto reachableInstsWithinIteration = customDfr->OUT(&inst);
            errs() << " Next are the " << reachableInstsWithinIteration.size() << " loop instructions reachable from " << inst << "\n";
            for (auto reachInst : reachableInstsWithinIteration){
              errs() << "   " << *reachInst << "\n";
            }
          }
        }
      }

      return false;
    }

    // We don't modify the program, so we preserve all analyses.
    // The LLVM IR of functions isn't ready at this point
    void getAnalysisUsage(AnalysisUsage &AU) const override {
      AU.addRequired<Noelle>();
    }
  };
}

// Next there is code to register your pass to "opt"
char CAT::ID = 0;
static RegisterPass<CAT> X("CAT", "Simple user of the Noelle framework");

// Next there is code to register your pass to "clang"
static CAT * _PassMaker = NULL;
static RegisterStandardPasses _RegPass1(PassManagerBuilder::EP_OptimizerLast,
    [](const PassManagerBuilder&, legacy::PassManagerBase& PM) {
        if(!_PassMaker){ PM.add(_PassMaker = new CAT());}}); // ** for -Ox
static RegisterStandardPasses _RegPass2(PassManagerBuilder::EP_EnabledOnOptLevel0,
    [](const PassManagerBuilder&, legacy::PassManagerBase& PM) {
        if(!_PassMaker){ PM.add(_PassMaker = new CAT()); }}); // ** for -O0