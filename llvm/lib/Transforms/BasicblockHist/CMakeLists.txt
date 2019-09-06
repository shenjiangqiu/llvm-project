# If we don't need RTTI or EH, there's no reason to export anything
# from the basicblockhist plugin.
if( NOT LLVM_REQUIRES_RTTI )
  if( NOT LLVM_REQUIRES_EH )
    set(LLVM_EXPORTED_SYMBOL_FILE ${CMAKE_CURRENT_SOURCE_DIR}/BasicblockHist.exports)
  endif()
endif()

if(WIN32 OR CYGWIN)
  set(LLVM_LINK_COMPONENTS Core Support)
endif()

add_llvm_library( LLVMBasicblockHist MODULE BUILDTREE_ONLY
  BasicblockHist.cpp

  DEPENDS
  intrinsics_gen
  PLUGIN_TOOL
  opt
  )
