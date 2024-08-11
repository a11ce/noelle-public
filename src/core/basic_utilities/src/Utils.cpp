/*
 * Copyright 2019 - 2024  Simone Campanoni
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
#include "arcana/noelle/core/Utils.hpp"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/IR/DerivedTypes.h"

namespace arcana::noelle {

bool Utils::isActualCode(Instruction *inst) {
  auto callInst = dyn_cast<CallBase>(inst);
  if (callInst == nullptr) {
    return true;
  }
  if (callInst->isLifetimeStartOrEnd()) {
    return false;
  }

  auto callee = callInst->getCalledFunction();
  if (callee != nullptr) {
    auto calleeName = callee->getName();
    if (calleeName.startswith("llvm.dbg.")) {
      return false;
    }
  }

  return true;
}

bool Utils::isAllocator(CallBase *callInst) {

  /*
   * Check the instruction.
   */
  if (callInst == nullptr) {
    return false;
  }

  /*
   * Fetch the callee.
   */
  auto callee = callInst->getCalledFunction();
  if (callee == nullptr) {
    return false;
  }

  /*
   * Check if it is a call to a library.
   */
  if (!callee->empty()) {
    return false;
  }

  /*
   * Check if it is a call to a known library function.
   */
  auto calleeName = callee->getName();
  if (false || (calleeName == "malloc") || (calleeName == "calloc")
      || (calleeName == "realloc")) {
    return true;
  }

  return false;
}

bool Utils::isReallocator(CallBase *callInst) {

  /*
   * Check the instruction.
   */
  if (callInst == nullptr) {
    return false;
  }

  /*
   * Fetch the callee.
   */
  auto callee = callInst->getCalledFunction();
  if (callee == nullptr) {
    return false;
  }

  /*
   * Check if it is a call to a library.
   */
  if (!callee->empty()) {
    return false;
  }

  /*
   * Check if it is a call to a known library function.
   */
  auto calleeName = callee->getName();
  if (false || (calleeName == "realloc")) {
    return true;
  }

  return false;
}

bool Utils::isDeallocator(CallBase *callInst) {

  /*
   * Check the instruction.
   */
  if (callInst == nullptr) {
    return false;
  }

  /*
   * Fetch the callee.
   */
  auto callee = callInst->getCalledFunction();
  if (callee == nullptr) {
    return false;
  }

  /*
   * Check if it is a call to a library.
   */
  if (!callee->empty()) {
    return false;
  }

  /*
   * Check if it is a call to a known library function.
   */
  auto calleeName = callee->getName();
  if (false || (calleeName == "free")) {
    return true;
  }

  return false;
}

Value *Utils::getAllocatedObject(CallBase *call) {
  if (!Utils::isAllocator(call)) {
    return nullptr;
  }
  auto callee = call->getCalledFunction();
  if (callee == nullptr) {
    return nullptr;
  }
  auto calleeName = callee->getName();
  if (false || (calleeName == "malloc") || (calleeName == "calloc")
      || (calleeName == "realloc")) {
    return call;
  }

  /*
   * TODO: complete this function with other allocators (e.g., posix_memalign)
   */
  abort();
}

Value *Utils::getFreedObject(CallBase *call) {
  if (!Utils::isDeallocator(call)) {
    return nullptr;
  }
  auto callee = call->getCalledFunction();
  if (callee == nullptr) {
    return nullptr;
  }
  auto calleeName = callee->getName();
  if (false || (calleeName == "free")) {
    return call->getArgOperand(0);
  }

  /*
   * TODO: complete this function with other deallocators
   */
  abort();
}

/*
 * Builds a printf call at builder's insert point that prints toPrint.
 * ARGS: toPrint: value to print. format: A format string as one would pass to
 * printf: determines the format of the printed value. Side effect: injects
 * format as a global, builds an alloca and a store because toPrint must be
 * passed to printf by reference.
 */
Value *Utils::injectPrint(Value *toPrint,
                          std::string format,
                          IRBuilder<> &builder) {

  auto M = builder.GetInsertBlock()->getModule();
  auto funcType =
      FunctionType::get(builder.getInt32Ty(),
                        ArrayRef<Type *>({ builder.getInt8PtrTy() }),
                        true);
  auto printfFunc = M->getOrInsertFunction("printf", funcType);

  StringRef formatStringRef = StringRef(format);

  Function *F = builder.GetInsertBlock()->getParent();
  LLVMContext &stringContext = F->getContext();

  // stringBuilder used in these three lines only
  IRBuilder<> stringBuilder(stringContext);
  stringBuilder.SetInsertPoint(&(F->front()));
  GlobalVariable *formatString =
      stringBuilder.CreateGlobalString(formatStringRef, "printingFormatString");

  auto valuePtr = builder.CreateAlloca(toPrint->getType());
  auto valueStore = builder.CreateStore(toPrint, valuePtr);

  auto stringGEP = builder.CreateGEP(
      formatString->getValueType(),
      formatString,
      ArrayRef<Value *>({ builder.getInt64(0), builder.getInt64(0) }));
  /*auto valueGEP = builder.CreateGEP(
      valuePtr->getType(),
      valuePtr,
      ArrayRef<Value *>({ builder.getInt64(0), builder.getInt64(0) }));*/
  auto callToPrintf =
      builder.CreateCall(printfFunc,
                         ArrayRef<Value *>({ stringGEP, valuePtr }));
  return callToPrintf;
}

/*
 * Builds a printf call at builder's insert point that prints all elements of
 * toPrint in one go. Useful for parallel settings. ARGS: toPrint: value to
 * print. format: A format string as one would pass to printf: determines the
 * format of the printed value. Side effect: injects format as a global, builds
 * allocas and stores because toPrint must be passed to printf by reference.
 */
Value *Utils::injectPrint(std::vector<Value *> &toPrint,
                          std::string format,
                          IRBuilder<> &builder) {

  auto M = builder.GetInsertBlock()->getModule();
  auto funcType =
      FunctionType::get(builder.getInt32Ty(),
                        ArrayRef<Type *>({ builder.getInt8PtrTy() }),
                        true);
  auto printfFunc = M->getOrInsertFunction("printf", funcType);

  StringRef formatStringRef = StringRef(format);

  Function *F = builder.GetInsertBlock()->getParent();
  LLVMContext &stringContext = F->getContext();

  // stringBuilder used in these three lines only
  IRBuilder<> stringBuilder(stringContext);
  stringBuilder.SetInsertPoint(&(F->front()));
  GlobalVariable *formatString =
      stringBuilder.CreateGlobalString(formatStringRef, "printingFormatString");

  auto stringGEP = builder.CreateGEP(
      formatString->getValueType(),
      formatString,
      ArrayRef<Value *>({ builder.getInt64(0), builder.getInt64(0) }));

  std::vector<Value *> ptrs;
  ptrs.push_back(stringGEP);
  for (auto &x : toPrint) {
    auto valuePtr = builder.CreateAlloca(x->getType());
    auto valueStore = builder.CreateStore(x, valuePtr);
    ptrs.push_back(valuePtr);
  }
  /*auto valueGEP = builder.CreateGEP(
      valuePtr->getType(),
      valuePtr,
      ArrayRef<Value *>({ builder.getInt64(0), builder.getInt64(0) }));*/
  auto callToPrintf = builder.CreateCall(printfFunc, ArrayRef<Value *>(ptrs));
  return callToPrintf;
}

/*
 * Builds a printf call at builder's insert point that prints toPrint.
 * Side effect: injects toPrint as a global.
 */
Value *Utils::injectPrint(std::string toPrint, IRBuilder<> &builder) {

  StringRef debugStringRef = StringRef(toPrint);

  Function *F = builder.GetInsertBlock()->getParent();
  LLVMContext &stringContext = F->getContext();

  // stringBuilder used in these three lines only
  IRBuilder<> stringBuilder(stringContext);
  stringBuilder.SetInsertPoint(&(F->front()));
  GlobalVariable *printString =
      stringBuilder.CreateGlobalString(debugStringRef, "debugString");

  auto M = F->getParent();
  auto funcType =
      FunctionType::get(builder.getInt32Ty(),
                        ArrayRef<Type *>({ builder.getInt8PtrTy() }),
                        true);
  auto printfFunc = M->getOrInsertFunction("printf", funcType);

  auto stringGEP = builder.CreateGEP(
      printString->getValueType(),
      printString,
      ArrayRef<Value *>({ builder.getInt64(0), builder.getInt64(0) }));
  auto callToPrintf =
      builder.CreateCall(printfFunc, ArrayRef<Value *>({ stringGEP }));
  return callToPrintf;
}

} // namespace arcana::noelle
