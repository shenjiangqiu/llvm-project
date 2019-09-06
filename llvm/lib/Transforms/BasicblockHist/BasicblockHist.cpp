//===- BasicblockHist.cpp - Example code from "Writing an LLVM Pass" ---------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements two versions of the LLVM "BasicblockHist World" pass described
// in docs/WritingAnLLVMPass.html
//
//===----------------------------------------------------------------------===//

#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
using namespace llvm;

#define DEBUG_TYPE "basicblockhist"

STATISTIC(BasicblockHistCounter, "Counts number of functions greeted");

namespace {
  // BasicblockHist - The first implementation, without getAnalysisUsage.
  struct BasicblockHist : public FunctionPass {
    static char ID; // Pass identification, replacement for typeid
    BasicblockHist() : FunctionPass(ID) {}

    bool runOnFunction(Function &F) override {
      ++BasicblockHistCounter;
      errs() << "BasicblockHist: ";
      errs().write_escaped(F.getName()) << '\n';
      return false;
    }
  };
}

char BasicblockHist::ID = 0;
static RegisterPass<BasicblockHist> X("basicblockhist", "BasicblockHist World Pass");

namespace {
  // BasicblockHist2 - The second implementation with getAnalysisUsage implemented.
  struct BasicblockHist2 : public FunctionPass {
    static char ID; // Pass identification, replacement for typeid
    BasicblockHist2() : FunctionPass(ID) {}

    bool runOnFunction(Function &F) override {
      ++BasicblockHistCounter;
      errs() << "BasicblockHist: ";
      errs().write_escaped(F.getName()) << '\n';
      return false;
    }

    // We don't modify the program, so we preserve all analyses.
    void getAnalysisUsage(AnalysisUsage &AU) const override {
      AU.setPreservesAll();
    }
  };
}

char BasicblockHist2::ID = 0;
static RegisterPass<BasicblockHist2>
Y("basicblockhist2", "BasicblockHist World Pass (with getAnalysisUsage implemented)");
