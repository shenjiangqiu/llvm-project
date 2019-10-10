//===- BasicBlockHist.cpp - Example code from "Writing an LLVM Pass" ---------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements two versions of the LLVM "BasicBlockHist World" pass described
// in docs/WritingAnLLVMPass.html
//
//===----------------------------------------------------------------------===//

#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"
#include <map>
#include<iomanip>
#include<iostream>
using namespace llvm;


#define DEBUG_TYPE "basicblockhist"

namespace {
  struct BasicBlockHist: public ModulePass{
    static char ID; // Pass identification, replacement for typeid
    BasicBlockHist():ModulePass(ID){}
    bool runOnModule(Module& M){
      std::map<size_t,unsigned> module_size_map;//store inst size:count
      std::map<size_t,unsigned> module_load_map;//store load :count
      unsigned module_total_block=0;
      unsigned module_total_inst=0;
      unsigned module_total_load=0;
      for(const auto&F:M.getFunctionList()){

      
        const auto& basic_blocks=F.getBasicBlockList();//get all basicblocks
        errs() << "Function: "<<F.getName()<<"\n";
        std::map<size_t,unsigned> size_map;//store inst size:count
        std::map<size_t,unsigned> load_map;//store load :count
        unsigned total_block=0;
        unsigned total_inst=0;
        unsigned total_load=0;
        for(auto &basic_block : basic_blocks){//for each basic block in this function
          total_block++;
          module_total_block++;
          module_total_inst+=basic_block.size();
          total_inst+=basic_block.size();
          size_map[basic_block.size()]++;//update the histo
          module_size_map[basic_block.size()]++;

          unsigned local_loads=0;
          for(auto& inst:basic_block.getInstList()){
            if(inst.getOpcode()==Instruction::Load){
                local_loads++;//current basic block loads num
                total_load++;//current function loads
                module_total_load++;
            }
          }
          load_map[local_loads]++;//update the load histo
          module_load_map[local_loads]++;
        }
        errs() << "All instructions\n";
        for(auto map_element:size_map){
          errs() << map_element.first<<" : "<<map_element.second<<"\n";
        }
        if(total_block==0) std::cerr<<"no block"<<std::endl;
        else
        std::cerr<<"avg :"<<std::fixed<<std::setprecision(2)<<(double)total_inst/(double)total_block<<"\n\n";

        errs() << "Load only\n";
        for(auto map_element:load_map){

          errs() << map_element.first<<" : "<<map_element.second<<"\n";
        }
        if(total_block==0) std::cerr<<"no block"<<std::endl;
        else
        std::cerr<<"avg :"<<(double)total_load/(double)total_block<<"\n\n";

    
      }
      errs() << "\n\nThis file's histogram All \n";
      errs() << "All instructions\n";

      for(auto map_element:module_size_map){
        errs() << map_element.first<<" : "<<map_element.second<<"\n";
      }
      if(module_total_block==0) std::cerr<<"no block!"<<std::endl;
      else
      std::cerr<<"avg :"<<std::fixed<<std::setprecision(2)<<(double)module_total_inst/(double)module_total_block<<"\n\n";
      errs() << "Load only\n";
      for(auto map_element:module_load_map){
        errs() << map_element.first<<" : "<<map_element.second<<"\n";
      }
      if(module_total_block==0) std::cerr<<"no block!"<<std::endl;
      else
      std::cerr<<"avg :"<<(double)module_total_load/(double)module_total_block<<"\n\n";

      return false;

    }

  };
  char BasicBlockHist::ID = 0;
  static RegisterPass<BasicBlockHist> X("basicblockhist", "BasicBlockHist Pass");

    
  struct BasicBlockHist2 : public FunctionPass {
    static char ID; // Pass identification, replacement for typeid
    BasicBlockHist2() : FunctionPass(ID) {}

    bool runOnFunction(Function &F) override {
      
      auto& basic_blocks=F.getBasicBlockList();//get all basicblocks
      errs() << "Function: "<<F.getName()<<"\n";
      std::map<size_t,unsigned> size_map;//store inst size:count
      std::map<size_t,unsigned> load_map;//store load :count
      unsigned total_block=0;
      unsigned total_inst=0;
      unsigned total_load=0;
      for(auto &basic_block : basic_blocks){//for each basic block in this function
        total_block++;
        total_inst+=basic_block.size();
        size_map[basic_block.size()]++;//update the histo

        unsigned local_loads=0;
        for(auto& inst:basic_block.getInstList()){
          if(inst.getOpcode()==Instruction::Load){
              local_loads++;//current basic block loads num
              total_load++;//current function loads
          }
        }
        load_map[local_loads]++;//update the load histo
      }
      errs() << "All instructions\n";
      for(auto map_element:size_map){
        
        errs() << map_element.first<<" : "<<map_element.second<<"\n";
      }
      std::cerr<<"avg :"<<std::fixed<<std::setprecision(2)<<(double)total_inst/(double)total_block<<"\n\n";

      errs() << "Load only\n";
      for(auto map_element:load_map){
        
        errs() << map_element.first<<" : "<<map_element.second<<"\n";
      }
      std::cerr<<"avg :"<<(double)total_load/(double)total_block<<"\n\n";
      
      return false;
    }
  };
}

char BasicBlockHist2::ID = 0;
static RegisterPass<BasicBlockHist2> X("basicblockhist2", "BasicBlockHist Pass");
