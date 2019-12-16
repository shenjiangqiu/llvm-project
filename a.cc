Index: include/llvm/Analysis/LoopCostAnalysis.h
===================================================================
--- /dev/null
++ include/llvm/Analysis/LoopCostAnalysis.h
@@ -0,0 +1,159 @@
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
Index: include/llvm/Analysis/Passes.h
===================================================================
--- include/llvm/Analysis/Passes.h
++ include/llvm/Analysis/Passes.h
@@ -74,6 +74,13 @@
 
   //===--------------------------------------------------------------------===//
   //
  // createLoopCostAnalysisPass - This pass assigns a numerical cost to each
  // loop in the loop nest considering cache and register data.
  //
  FunctionPass *createLoopCostAnalysisPass();

  //===--------------------------------------------------------------------===//
  //
   // createRegionInfoPass - This pass finds all single entry single exit regions
   // in a function and builds the region hierarchy.
   //
Index: include/llvm/InitializePasses.h
===================================================================
--- include/llvm/InitializePasses.h
++ include/llvm/InitializePasses.h
@@ -122,6 +122,7 @@
 void initializeEarlyIfConverterPass(PassRegistry&);
 void initializeEdgeBundlesPass(PassRegistry&);
 void initializeExpandPostRAPass(PassRegistry&);
void initializeLoopCostAnalysisPass(PassRegistry&);
 void initializeAAResultsWrapperPassPass(PassRegistry &);
 void initializeGCOVProfilerLegacyPassPass(PassRegistry&);
 void initializePGOInstrumentationGenLegacyPassPass(PassRegistry&);
Index: include/llvm/LinkAllPasses.h
===================================================================
--- include/llvm/LinkAllPasses.h
++ include/llvm/LinkAllPasses.h
@@ -109,6 +109,7 @@
       (void) llvm::createLCSSAPass();
       (void) llvm::createLICMPass();
       (void) llvm::createLazyValueInfoPass();
      (void) llvm::createLoopCostAnalysisPass();
       (void) llvm::createLoopExtractorPass();
       (void) llvm::createLoopInterchangePass();
       (void) llvm::createLoopSimplifyPass();
Index: lib/Analysis/Analysis.cpp
===================================================================
--- lib/Analysis/Analysis.cpp
++ lib/Analysis/Analysis.cpp
@@ -45,6 +45,7 @@
   initializeDomOnlyViewerPass(Registry);
   initializePostDomViewerPass(Registry);
   initializeDomOnlyPrinterPass(Registry);
  initializeLoopCostAnalysisPass(Registry);
   initializePostDomPrinterPass(Registry);
   initializePostDomOnlyViewerPass(Registry);
   initializePostDomOnlyPrinterPass(Registry);
Index: lib/Analysis/CMakeLists.txt
===================================================================
--- lib/Analysis/CMakeLists.txt
++ lib/Analysis/CMakeLists.txt
@@ -40,6 +40,7 @@
   Lint.cpp
   Loads.cpp
   LoopAccessAnalysis.cpp
  LoopCostAnalysis.cpp
   LoopUnrollAnalyzer.cpp
   LoopInfo.cpp
   LoopPass.cpp
Index: lib/Analysis/LoopCostAnalysis.cpp
===================================================================
--- /dev/null
++ lib/Analysis/LoopCostAnalysis.cpp
@@ -0,0 +1,382 @@
//===----- LoopCostAnalysis.cpp - Analyse a loop for its cost -------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements a set of routines that does the loop cost analysis.
// TODO: Extend the Loop Cost calculation to imperfect nests and more than one
// basicblock in the innermost loop.
//
//===----------------------------------------------------------------------===//

#include "llvm/Analysis/LoopCostAnalysis.h"

using namespace llvm;

#define DEBUG_TYPE "loop-cost"

/* - Build/test perfect loop nests - should probably be moved to Loop Utils - */
// TODO: Rotated loops forming perfect nests cannot be relied on and hence not
// considered for now.
bool IsRotatedLoop(Loop *L) {
  return (L->getHeader() != L->getExitingBlock()); // Probably rotated loop.
}

// Return true if all blocks of @L are perfectly nested under @L's subloop. @L's
// header and latch are exempted though.
bool BlocksPerfectlyNestedUnder(Loop *L) {
  assert ((L->getSubLoops().size() == 1) &&
            "Expected loop containing a single subloop!");
  Loop *Subloop = *L->begin();
  for (auto bb = L->block_begin(), bbe = L->block_end(); bb != bbe; ++bb) {
    if (*bb == L->getHeader() || *bb == L->getLoopLatch())
      continue;
    if (!Subloop->contains(*bb)) {
      // Ignore empty blocks.
      auto bbBr = dyn_cast<BranchInst>((*bb)->getTerminator());
      if ((*bb)->size() == 1 && (bbBr && bbBr->isUnconditional()))
        continue;
      return false;
    }
  }
  return true;
}

bool hasSimpleHeaderLatch(Loop *L) {
  // TODO: Unrotated loops form perfect nests only if loop's header and latch
  // contain only loop control updates and exit check. Any other instruction is
  // deemed to violate perfect nesting.
  return true;
}

bool
PopulatePerfectLoopNestsUnder(Loop *L,
                              SmallVectorImpl<LoopNest_t> &PerfectLoopNests) {
  // Base case: the innermost loop.
  if (L->begin() == L->end()) {
    if (!IsRotatedLoop(L) && hasSimpleHeaderLatch(L)) {
      LoopNest_t LN; 
      LN.push_back(L);
      PerfectLoopNests.push_back(LN);
      return true;
    }
    return false;
  }

  // Iterate over subloops.
  bool PerfectSubnest = true;
  for (Loop::iterator i = L->begin(), e = L->end(); i != e; i++)
    PerfectSubnest &= PopulatePerfectLoopNestsUnder(*i, PerfectLoopNests);

  // If perfect and single subnest.
  if (PerfectSubnest && (L->getSubLoops().size() == 1) && !IsRotatedLoop(L) &&
      hasSimpleHeaderLatch(L) && BlocksPerfectlyNestedUnder(L)) {
    // Add this loop to perfect nest.
    LoopNest_t *LN = &PerfectLoopNests[PerfectLoopNests.size() - 1]; 
    LN->insert(LN->begin(), L);
    return true;
  }
  return false;
}

// Check if the loops in @LN form a perfect nest. Loops in @LN should be listed
// from outermost loop to innermost loop of the loop nest.
bool IsPerfectNest(LoopNest_t LN) {
  auto l = LN.rbegin(); // Start with innermost loop.
  Loop *Innermost = *l;
  if ((Innermost->begin() != Innermost->end()) || IsRotatedLoop(Innermost) ||
      !hasSimpleHeaderLatch(Innermost))
    return false;

  Loop *Subloop = Innermost;
  ++l;
  for (auto le = LN.rend(); l != le; ++l) {
    Loop *L = *l;
    if ((L->getSubLoops().size() != 1) || (*L->begin() != Subloop) ||
        IsRotatedLoop(L) || !hasSimpleHeaderLatch(L) ||
        !BlocksPerfectlyNestedUnder(L))
      return false;
    Subloop = L;
  }
  return true;
}

// LoopCost implementation.
// Print the loop costs of all the loops in the loopnest.
void LoopCost::printLoopCosts() {
  DEBUG(
  dbgs() << "Printing Loop Costs: ";
  if (LoopCosts.empty())
    dbgs() << "(empty)";
  dbgs() << "\n";
  for (auto lc : LoopCosts) {
    dbgs() << "Loop: " << lc.first->getHeader()->getName();
    dbgs() << "\tCosts: " << lc.second << "\n";
  }
  );
}

// Print the reference groups.
void LoopCost::printReferenceGroups() {
  DEBUG(
  dbgs() << "Printing Reference Groups (GEPs): ";
  if (ReferenceGroups.begin() == ReferenceGroups.end())
    dbgs() << "(empty)";
  dbgs() << "\n";
  for (auto ri : ReferenceGroups)
    dbgs() << "Ref group: " << *ri << "\n";
  );
}

// Print the loop trip counts of each of the loops in the loopnest.
void LoopCost::printTripCounts() {
  DEBUG(
  dbgs() << "Printing Trip Counts: ";
  if (LoopTripCounts.empty())
    dbgs() << "(empty)";
  dbgs() << "\n";
  for (auto ltc : LoopTripCounts) {
    dbgs() << "Loop: " << ltc.first->getHeader()->getName();
    dbgs() << "\tTripCount: " << ltc.second << "\n";
  }
  );
}

// Return loop cost of @L. Return -1.0 if loop is not found in map.
double LoopCost::getLoopCostOf(Loop *L) {
  auto LC = LoopCosts.find(L);
  if (LC == LoopCosts.end())
    return -1.0;
  return LC->second;
}

// Set the trip counts for each of the loops in the loopnest. If trip count for
// a loop is not found, a normalized value based on the surrounding loops is
// set or statically set to STATIC_TRIP_COUNT.
void LoopCost::setTripCounts(LoopNest_t LN) {
#define STATIC_TRIP_COUNT 1000

  for (auto L : LN) {
    unsigned TripCount = SCEV->getSmallConstantTripCount(L, L->getExitingBlock());
    LoopTripCounts.push_back(std::make_pair(L, (TripCount)));
  }

  // Normalize trip counts: reset trip counts if 0.
  // TODO: Can do better?
  for (auto li = LoopTripCounts.begin(), le = LoopTripCounts.end();
            li != le; li++) {
    unsigned TC = (*li).second;
    if (TC)
      continue;
    // TC is 0, reset to average of the surrounding loop's tripcount.
    auto prev = li, next = li;
    if (li == LoopTripCounts.begin()) {
      ++next;
      (*li).second = (*next).second;
    } else if (li == (LoopTripCounts.end() - 1)) {
      --prev;
      (*li).second = (*prev).second;
    } else {
      --prev; ++next;
      (*li).second = ((*prev).second + (*next).second) / 2;
    }
    if ((*li).second == 0)  // Still 0? Statically set!
      (*li).second = STATIC_TRIP_COUNT;
  }
}

// This routine creates different Reference (GEP) Groups based on GEP's access
// w.r.t. CacheLine. A GEP access is added to an existing group if it's access
// lies in the same cache line; otherwise this GEP will form a new RefGroup.
// Groups can be made based on the following:
// a. A GEP having different number of operands will form a different group.
// b. Only the last operand of GEP is analysed as that is what matters with the
//    cache line.
// c. Two GEPs with their last operands - i and i + 4 falls into same RefGroup
//    if SCEV(i) - SCEV(i + 4) is a constant that is less than the cache line.
// TODO: Add alignment info.
void LoopCost::CreateReferenceGroups(BasicBlock *bb) {
  assert(bb);
  for (auto &i : *bb) {
    if (auto *GEP = dyn_cast<GetElementPtrInst>(&i)) {
      bool UniqueGEP = true;
      auto NumOps = GEP->getNumOperands();
      auto LastOp = GEP->getOperand(NumOps - 1);
      if (SCEV->isSCEVable(LastOp->getType())) {
        for (auto RG : ReferenceGroups) {
          auto RGNumOps = RG->getNumOperands();
          if (RGNumOps != NumOps)
            continue;

          // All n - 1 GEP operands must be equal.
          bool EqualOps = true;
          for (unsigned i = 0, e = NumOps - 1; (EqualOps && i != e); ++i)
            if (GEP->getOperand(i) != RG->getOperand(i))
              EqualOps = false;
          if (!EqualOps)
            continue;

          auto RGLastOp = RG->getOperand(RGNumOps - 1);
          if (!SCEV->isSCEVable(RGLastOp->getType()))
            continue;

          // Check if (LastOp - RGLastOp) lies within cache line.
          auto GEPSCEV = SCEV->getSCEV(LastOp);
          auto RGSCEV = SCEV->getSCEV(RGLastOp);
          auto DiffSCEV =
                  dyn_cast<SCEVConstant>(SCEV->getMinusSCEV(GEPSCEV, RGSCEV));
          if (DiffSCEV->getValue()->getSExtValue() < Cache.getLineSize()) {
            UniqueGEP = false;
            break;
          }
        }
      }
      if (UniqueGEP)
        ReferenceGroups.push_back(GEP);
    }
  }
}

// Return true if the phinode is used in the computation of the operand.
// TODO: Use SCEVTraversal or some similar thing.
bool LoopCost::ASTMatch(Value *Operand, PHINode *P) {
  if (Operand == P)
    return true;

  auto I = dyn_cast<Instruction>(Operand);
  if (!I || isa<PHINode>(I))
    return false;

  for (unsigned i = 0, e = I->getNumOperands(); i != e; ++i)
    if (ASTMatch(I->getOperand(i), P))
      return true;
  return false;
}

BasicBlock *getInnerSingleBB(Loop *L) {
  BasicBlock *BB = nullptr;
  for (auto bb = L->block_begin(), bbe = L->block_end(); bb != bbe; ++bb) {
    if (*bb == L->getHeader() || *bb == L->getLoopLatch())
      continue;

    // Ignore empty blocks.
    auto bbBr = dyn_cast<BranchInst>((*bb)->getTerminator());
    if ((*bb)->size() == 1 && (bbBr && bbBr->isUnconditional()))
      continue;

    if (BB) // A BB was already found.
      return nullptr; // There are multiple BBs.

    BB = *bb;
  }
  return BB;
}

void LoopCost::calculateLoopCosts(LoopNest_t LN) {
  // Initialize cost of all loops in nest to -1.
  for (auto L : LN)
    LoopCosts[L] = -1.0;

  if (!IsPerfectNest(LN))
    return;

  // For now, restricting to a single bb in innermost loop. This will be
  // relaxed soon.
  BasicBlock *innermostBB = getInnerSingleBB(*LN.rbegin());
  if (!innermostBB)
    return;

  CreateReferenceGroups(innermostBB);
  setTripCounts(LN);

  // Calculate loop costs for each of the loop in the loopnest assuming loop to
  // be the innermost loop.
  for (auto L : LN) {
    double ThisLoopCost = 0.0;

    PHINode *P = L->getCanonicalInductionVariable();
    if (!P) {
      DEBUG(dbgs() << "Could not find induction variable\n");
      continue;
    }
    DEBUG(dbgs() << "Loop: " << L->getHeader()->getName()
                 << " phi: " << *P << "\n");

    // Calculate penalties from other loops.
    double ThisLoopPenalty = 1.0;
    double OtherLoopPenalties = 1.0;
    for (auto ltc : LoopTripCounts) {
      if (ltc.first == L)
        ThisLoopPenalty = ltc.second;
      else
        OtherLoopPenalties = OtherLoopPenalties * ltc.second;
    }
    assert((ThisLoopPenalty > 0 && OtherLoopPenalties > 0)
            && "Incorrect loop penalty");
    DEBUG(dbgs() << "ThisLoopPenalty: " << ThisLoopPenalty << "\n");
    DEBUG(dbgs() << "OtherLoopPenalties: " << OtherLoopPenalties << "\n");

    for (auto GEP : ReferenceGroups) {
      // Check stride of this GEP access. The position of use of 'P' is
      // checked in the operands of GEP. TODO: If the operands of GEP do not
      // use 'P' directly, the AST has to be traversed starting from each
      // operand of GEP in order to find the use of 'P'.
      DEBUG(dbgs() << "GEP: " << *GEP << "\n");
      double ThisRefPenalty = 1.0;  // Is 1 for an invariant reference.
      for (unsigned ni = 1, ne = GEP->getNumIndices(); ni <= ne; ni++) {
        if (ASTMatch(GEP->getOperand(ni), P)) {
          // Check stride access.
          if ((AccessOrder == ROWMAJOR && ni == ne)
           || (AccessOrder == COLUMNMAJOR && ni == 2)) { // ni == 1 is primary offset.
            // Contiguous locations.
            ThisRefPenalty = ThisLoopPenalty / Cache.getLineSize();
          } else {
            // Non-contiguous locations.
            ThisRefPenalty = ThisLoopPenalty;
          }
          DEBUG(dbgs() << "ThisRefPenalty: " << ThisRefPenalty << "\n");
        }
      }
      ThisLoopCost = ThisLoopCost + ThisRefPenalty * OtherLoopPenalties;
      DEBUG(dbgs() << "Accum cost of this loop: " << ThisLoopCost << "\n\n");
    }
    LoopCosts[L] = ThisLoopCost;
  }
}

bool LoopCostAnalysis::runOnFunction(Function &F) {
  auto LI = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
  auto SCEV = &getAnalysis<ScalarEvolutionWrapperPass>().getSE();
  LC = new LoopCost(SCEV);

  DEBUG(dbgs() << "Calculate LoopCosts in Function: " << F.getName() << "\n");
  SmallVector <LoopNest_t, 2> PerfectLoopNests;
  // Build perfect loop nests.
  for (auto L = LI->begin(), Le = LI->end(); L != Le; ++L)
    PopulatePerfectLoopNestsUnder(*L, PerfectLoopNests);

  for (auto PN : PerfectLoopNests)
    LC->calculateLoopCosts(PN);

  LC->printLoopCosts();
  LC->printTripCounts();
  return false;
}

char LoopCostAnalysis::ID = 0;

auto PassDesc = "Experimental, Cache aware Loop Cost Analysis";
INITIALIZE_PASS_BEGIN(LoopCostAnalysis, "loop-cost", PassDesc, false, true)
INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
INITIALIZE_PASS_DEPENDENCY(ScalarEvolutionWrapperPass)
INITIALIZE_PASS_END(LoopCostAnalysis, "loop-cost", PassDesc, false, true)

namespace llvm {
FunctionPass *createLoopCostAnalysisPass() {
  return new LoopCostAnalysis();
}
}
Index: test/Analysis/CostModel/X86/matmul-perfect-loopCost.ll
===================================================================
--- /dev/null
++ test/Analysis/CostModel/X86/matmul-perfect-loopCost.ll
@@ -0,0 +1,102 @@
; RUN: opt -loop-cost -debug %s -o /dev/null 2>&1 | FileCheck %s
; Test Loop Cost Analysis of perfectly nested matrix multiplication code having
; global arrays.
; Generated from matmul-perfect.c:
; #include <stdio.h>
; #define SIZE 5000
; int a[SIZE][SIZE], b[SIZE][SIZE], c[SIZE][SIZE];
; void matmul() {
;   int i, j, k;
;   for (i = 0; i < SIZE; i++) {
;     for (j = 0; j < SIZE; j++) {
;       for (k = 0; k < SIZE; k++) {
;         c[i][j] = c[i][j] + a[i][k] * b[k][j];
;       }   
;     }   
;   }
; }
; with clang -emit-llvm matmul-perfect.c -S -o matmul-perfect.clang.ll
; then opt -mem2reg -loop-simplify -instcombine -instnamer -indvars -S -o matmul-perfect.opt.ll

; CHECK:      Loop: for.cond4 Costs: 1.563688e+11
; CHECK-NEXT: Loop: for.cond1 Costs: 6.256252e+10
; CHECK-NEXT: Loop: for.cond  Costs: 2.501750e+11
; CHECK:      Loop: for.cond  TripCount: 5001
; CHECK-NEXT: Loop: for.cond1 TripCount: 5001
; CHECK-NEXT: Loop: for.cond4 TripCount: 5001

; ModuleID = 'matmul-perfect.clang.bc'
source_filename = "matmul-perfect.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@c = common global [5000 x [5000 x i32]] zeroinitializer, align 16
@a = common global [5000 x [5000 x i32]] zeroinitializer, align 16
@b = common global [5000 x [5000 x i32]] zeroinitializer, align 16

; Function Attrs: nounwind uwtable
define void @matmul() #0 {
entry:
  br label %for.cond

for.cond:                                         ; preds = %for.inc24, %entry
  %indvars.iv6 = phi i64 [ %indvars.iv.next7, %for.inc24 ], [ 0, %entry ]
  %exitcond8 = icmp ne i64 %indvars.iv6, 5000
  br i1 %exitcond8, label %for.body, label %for.end26

for.body:                                         ; preds = %for.cond
  br label %for.cond1

for.cond1:                                        ; preds = %for.inc21, %for.body
  %indvars.iv3 = phi i64 [ %indvars.iv.next4, %for.inc21 ], [ 0, %for.body ]
  %exitcond5 = icmp ne i64 %indvars.iv3, 5000
  br i1 %exitcond5, label %for.body3, label %for.end23

for.body3:                                        ; preds = %for.cond1
  br label %for.cond4

for.cond4:                                        ; preds = %for.inc, %for.body3
  %indvars.iv = phi i64 [ %indvars.iv.next, %for.inc ], [ 0, %for.body3 ]
  %exitcond = icmp ne i64 %indvars.iv, 5000
  br i1 %exitcond, label %for.body6, label %for.end

for.body6:                                        ; preds = %for.cond4
  %arrayidx8 = getelementptr inbounds [5000 x [5000 x i32]], [5000 x [5000 x i32]]* @c, i64 0, i64 %indvars.iv6, i64 %indvars.iv3
  %tmp = load i32, i32* %arrayidx8, align 4
  %arrayidx12 = getelementptr inbounds [5000 x [5000 x i32]], [5000 x [5000 x i32]]* @a, i64 0, i64 %indvars.iv6, i64 %indvars.iv
  %tmp1 = load i32, i32* %arrayidx12, align 4
  %arrayidx16 = getelementptr inbounds [5000 x [5000 x i32]], [5000 x [5000 x i32]]* @b, i64 0, i64 %indvars.iv, i64 %indvars.iv3
  %tmp2 = load i32, i32* %arrayidx16, align 4
  %mul = mul nsw i32 %tmp1, %tmp2
  %add = add nsw i32 %tmp, %mul
  %arrayidx20 = getelementptr inbounds [5000 x [5000 x i32]], [5000 x [5000 x i32]]* @c, i64 0, i64 %indvars.iv6, i64 %indvars.iv3
  store i32 %add, i32* %arrayidx20, align 4
  br label %for.inc

for.inc:                                          ; preds = %for.body6
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  br label %for.cond4

for.end:                                          ; preds = %for.cond4
  br label %for.inc21

for.inc21:                                        ; preds = %for.end
  %indvars.iv.next4 = add nuw nsw i64 %indvars.iv3, 1
  br label %for.cond1

for.end23:                                        ; preds = %for.cond1
  br label %for.inc24

for.inc24:                                        ; preds = %for.end23
  %indvars.iv.next7 = add nuw nsw i64 %indvars.iv6, 1
  br label %for.cond

for.end26:                                        ; preds = %for.cond
  ret void
}

attributes #0 = { nounwind uwtable }

!llvm.ident = !{!0}

!0 = !{!""}