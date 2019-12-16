//===----- LoopCostAnalysis.h - Analyse a loop for its cost ---------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements a set of routines that does the loop cost analysis.
//
// Prerequisite Reading:
// Compiler Optimizations for Improving Data Locality [Carr-McKinley-Tseng]

// High level details:
// Look through the memory accesses and create groups of references such that
// two references fall into different groups if they are accessed in different
// cache lines. Each group is then analysed with respect to innermost loops
// considering cache lines.

// Penalty for the reference is:
// a. 1 if the reference is invariant with the innermost loop,
// b. TripCount for non-unit stride access,
// c. TripCount/CacheLineSize for a unit-stride access.

// Loop cost is the sum total of the reference penalties times the product of
// the loop bounds of the outer loops.

//===----------------------------------------------------------------------===//

#include "llvm/Pass.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/ScalarEvolutionExpressions.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Debug.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Instructions.h"

namespace llvm {

// This data is mainly used for cache resue (spatial/temporal) aware
// calculations.
// TODO: This should probably go to a separate file with separate
// implemenation if the size grows.
class CacheData {
  unsigned LineSize;   // The number of words in a cache line.
  unsigned CacheSize;  // The size of cache.
  enum CacheWay {
    DIRECT,
    WAY2,
    WAY4,
    WAY8,
    FULL
  } Associativity;

  // TODO: Need to add other cache details as required.

public:
  CacheData() {}

  CacheData(unsigned _LS, unsigned _CS, CacheData::CacheWay _Assoc) {
    LineSize = _LS;
    CacheSize = _CS;
    Associativity = _Assoc;
  }

  // Get/set member functions.
  void setLineSize(unsigned Size)  { LineSize = Size; }
  unsigned getLineSize() { return LineSize; }

  void setCacheSize(unsigned Size) { CacheSize = Size; }
  unsigned getCacheSize()  { return CacheSize; }

  void setAssociativity(CacheData::CacheWay Assoc)  { Associativity = Assoc; }
  CacheData::CacheWay getAssociativity()  { return Associativity; }

  void initCacheData() {
    // TODO: Get the cache data from the target architecture.
    // Set default values for generic data layout.
    LineSize = 4;   // Statically setting for now.
  }
};

// TODO: Add a class RegData that will be mainly used for reg reuse aware
// calculations.

// Ordered list of loops from outer loop to the innermost loop that form the
// loop nest. 
typedef SmallVector<Loop *, 2> LoopNest_t;

// This is a utility class that can be used to calculate cache aware loop costs
// for a perfectly nested loop nest.
class LoopCost {
public:
  enum Order {
    COLUMNMAJOR = 0,
    ROWMAJOR
  } AccessOrder;

private:
  CacheData Cache;
  ScalarEvolution *SCEV;
  
  /// List of loops in loopnest with their tripcounts.
  typedef std::pair<Loop*, unsigned> loopTripcount;
  SmallVector<loopTripcount, 2> LoopTripCounts;
  void setTripCounts(LoopNest_t LN);

  /// The reference groups - directly noting the GEPs instead of load/stores.
  SmallVector<GetElementPtrInst*, 2> ReferenceGroups;
  void CreateReferenceGroups(BasicBlock *bb);

  /// The loops and their calculated loop costs.
  std::map <Loop *, double> LoopCosts;

  bool ASTMatch(Value *Operand, PHINode *P);

public:
  LoopCost(ScalarEvolution *_SCEV) 
    : SCEV(_SCEV) {
    Cache.initCacheData();
    AccessOrder = ROWMAJOR;
  }

  // Given a perfect nest, calculate loop costs of the loops in the nest.
  void calculateLoopCosts(LoopNest_t LN);

  double getLoopCostOf(Loop *L);

  // Print routines.
  void printLoopCosts();
  void printTripCounts();
  void printReferenceGroups();
};


class LoopCostAnalysis : public FunctionPass {
  LoopCost *LC;

public:
  static char ID;

  LoopCostAnalysis() : FunctionPass(ID) {
    initializeLoopCostAnalysisPass(*PassRegistry::getPassRegistry());
  }

  const LoopCost &getLoopCosts() const { return *LC; }

  bool runOnFunction(Function &F) override;

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<LoopInfoWrapperPass>();
    AU.addRequired<ScalarEvolutionWrapperPass>();
    AU.setPreservesAll();
  }
};
}
