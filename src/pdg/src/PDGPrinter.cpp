/*
 * Copyright 2016 - 2019  Angelo Matni, Simone Campanoni
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/ADT/GraphTraits.h"
#include "llvm/Analysis/DOTGraphTraitsPass.h"
#include "llvm/Analysis/DomPrinter.h"
#include "llvm/Support/GraphWriter.h"
#include "llvm/Support/DOTGraphTraits.h"

#include <set>
#include <queue>

#include "PDGPrinter.hpp"

using namespace llvm;

PDGPrinter::PDGPrinter () {
  return ;
}

void PDGPrinter::printPDG (
  Module &module, 
  CallGraph &callGraph, 
  PDG *graph,
  std::function<LoopInfo& (Function *f)> getLoopInfo
  ){

  /*
   * Collect functions through call graph starting at function "main"
   */
  std::set<Function *> funcToGraph;
  collectAllFunctionsInCallGraph(module, callGraph, funcToGraph);

  /*
   * Print the PDG
   */
  writeGraph<PDG>("pdg-full.dot", graph);
  for (auto F : funcToGraph) {
    auto& LI = getLoopInfo(F);
    printGraphsForFunction(*F, graph, LI);
  }

  return ;
}

void PDGPrinter::collectAllFunctionsInCallGraph (Module &M, CallGraph &callGraph, std::set<Function *> &funcSet) {
  std::queue<Function *> funcToTraverse;
  funcToTraverse.push(M.getFunction("main"));
  while (!funcToTraverse.empty())
  {
    auto func = funcToTraverse.front();
    funcToTraverse.pop();
    if (funcSet.find(func) != funcSet.end()) continue;
    funcSet.insert(func);

    auto funcCGNode = callGraph[func];
    for (auto &callRecord : make_range(funcCGNode->begin(), funcCGNode->end()))
    {
      auto F = callRecord.second->getFunction();
      if (F->empty()) continue;
      funcToTraverse.push(F);
    }
  }
}

void PDGPrinter::printGraphsForFunction(Function &F, PDG *graph, LoopInfo &LI) {

  /*
   * Name and graph the function's DG
   */
  std::string filename;
  raw_string_ostream ros(filename);
  ros << "pdg-" << F.getName() << ".dot";
  auto subgraph = graph->createFunctionSubgraph(F);
  writeGraph<PDG>(ros.str(), subgraph);

  /*
   * Name and graph the function's SCCDAG
   */
  filename.clear();
  ros << "sccdg-" << F.getName() << ".dot";
  SCCDAG *sccSubgraph = SCCDAG::createSCCDAGFrom(subgraph);
  writeGraph<SCCDAG>(ros.str(), sccSubgraph);

  /*
   * Name and graph each SCC within the function's SCCDAG
   */
  // int count = 0;
  // for (auto sccI = sccSubgraph->begin_nodes(); sccI != sccSubgraph->end_nodes(); ++sccI) {
  //   auto scc = (*sccI)->getT();
  //   filename.clear();
  //   ros << "scc-" << F.getName() << "-" << (count++) << ".dot";
  //   writeGraph<SCC>(ros.str(), scc);
  // }

  delete sccSubgraph;
  delete subgraph;

  /*
   * Name and graph the loop DG of the function
   */
  if (LI.empty()) return ;
  filename.clear();
  ros << "pdg-" << F.getName() << "-loop1.dot";

  subgraph = graph->createLoopsSubgraph(*(LI.getLoopsInPreorder().begin()));
  writeGraph<PDG>(ros.str(), subgraph);
  delete subgraph;

  return ;
}