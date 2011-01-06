set(MSVC_LIB_DEPS_LLVMARMAsmParser LLVMARMCodeGen LLVMARMInfo LLVMMC LLVMMCParser LLVMSupport LLVMTarget)
set(MSVC_LIB_DEPS_LLVMARMAsmPrinter LLVMMC LLVMSupport)
set(MSVC_LIB_DEPS_LLVMARMCodeGen LLVMARMAsmPrinter LLVMARMInfo LLVMAnalysis LLVMAsmPrinter LLVMCodeGen LLVMCore LLVMMC LLVMSelectionDAG LLVMSupport LLVMTarget)
set(MSVC_LIB_DEPS_LLVMARMDisassembler LLVMARMCodeGen LLVMARMInfo LLVMMC LLVMSupport)
set(MSVC_LIB_DEPS_LLVMARMInfo LLVMMC LLVMSupport)
set(MSVC_LIB_DEPS_LLVMAlphaCodeGen LLVMAlphaInfo LLVMAsmPrinter LLVMCodeGen LLVMCore LLVMMC LLVMSelectionDAG LLVMSupport LLVMTarget)
set(MSVC_LIB_DEPS_LLVMAlphaInfo LLVMMC LLVMSupport)
set(MSVC_LIB_DEPS_LLVMAnalysis LLVMCore LLVMSupport LLVMTarget)
set(MSVC_LIB_DEPS_LLVMArchive LLVMBitReader LLVMCore LLVMSupport)
set(MSVC_LIB_DEPS_LLVMAsmParser LLVMCore LLVMSupport)
set(MSVC_LIB_DEPS_LLVMAsmPrinter LLVMAnalysis LLVMCodeGen LLVMCore LLVMMC LLVMMCParser LLVMSupport LLVMTarget)
set(MSVC_LIB_DEPS_LLVMBitReader LLVMCore LLVMSupport)
set(MSVC_LIB_DEPS_LLVMBitWriter LLVMCore LLVMSupport)
set(MSVC_LIB_DEPS_LLVMBlackfinCodeGen LLVMAsmPrinter LLVMBlackfinInfo LLVMCodeGen LLVMCore LLVMMC LLVMSelectionDAG LLVMSupport LLVMTarget)
set(MSVC_LIB_DEPS_LLVMBlackfinInfo LLVMMC LLVMSupport)
set(MSVC_LIB_DEPS_LLVMCBackend LLVMAnalysis LLVMCBackendInfo LLVMCodeGen LLVMCore LLVMMC LLVMScalarOpts LLVMSupport LLVMTarget LLVMTransformUtils LLVMipa)
set(MSVC_LIB_DEPS_LLVMCBackendInfo LLVMMC LLVMSupport)
set(MSVC_LIB_DEPS_LLVMCellSPUCodeGen LLVMAsmPrinter LLVMCellSPUInfo LLVMCodeGen LLVMCore LLVMMC LLVMSelectionDAG LLVMSupport LLVMTarget)
set(MSVC_LIB_DEPS_LLVMCellSPUInfo LLVMMC LLVMSupport)
set(MSVC_LIB_DEPS_LLVMCodeGen LLVMAnalysis LLVMCore LLVMMC LLVMScalarOpts LLVMSupport LLVMTarget LLVMTransformUtils)
set(MSVC_LIB_DEPS_LLVMCore LLVMSupport)
set(MSVC_LIB_DEPS_LLVMCppBackend LLVMCore LLVMCppBackendInfo LLVMSupport LLVMTarget)
set(MSVC_LIB_DEPS_LLVMCppBackendInfo LLVMMC LLVMSupport)
set(MSVC_LIB_DEPS_LLVMExecutionEngine LLVMCore LLVMSupport LLVMTarget)
set(MSVC_LIB_DEPS_LLVMInstCombine LLVMAnalysis LLVMCore LLVMSupport LLVMTarget LLVMTransformUtils)
set(MSVC_LIB_DEPS_LLVMInstrumentation LLVMAnalysis LLVMCore LLVMSupport LLVMTransformUtils)
set(MSVC_LIB_DEPS_LLVMInterpreter LLVMCodeGen LLVMCore LLVMExecutionEngine LLVMSupport LLVMTarget)
set(MSVC_LIB_DEPS_LLVMJIT LLVMCodeGen LLVMCore LLVMExecutionEngine LLVMMC LLVMSupport LLVMTarget)
set(MSVC_LIB_DEPS_LLVMLinker LLVMArchive LLVMBitReader LLVMCore LLVMSupport LLVMTransformUtils)
set(MSVC_LIB_DEPS_LLVMMBlazeAsmParser LLVMMBlazeCodeGen LLVMMBlazeInfo LLVMMC LLVMMCParser LLVMSupport LLVMTarget)
set(MSVC_LIB_DEPS_LLVMMBlazeAsmPrinter LLVMMC LLVMSupport)
set(MSVC_LIB_DEPS_LLVMMBlazeCodeGen LLVMAsmPrinter LLVMCodeGen LLVMCore LLVMMBlazeAsmPrinter LLVMMBlazeInfo LLVMMC LLVMSelectionDAG LLVMSupport LLVMTarget)
set(MSVC_LIB_DEPS_LLVMMBlazeDisassembler LLVMMBlazeCodeGen LLVMMBlazeInfo LLVMMC LLVMSupport)
set(MSVC_LIB_DEPS_LLVMMBlazeInfo LLVMMC LLVMSupport)
set(MSVC_LIB_DEPS_LLVMMC LLVMSupport)
set(MSVC_LIB_DEPS_LLVMMCDisassembler LLVMARMAsmParser LLVMARMCodeGen LLVMARMDisassembler LLVMARMInfo LLVMAlphaCodeGen LLVMAlphaInfo LLVMBlackfinCodeGen LLVMBlackfinInfo LLVMCBackend LLVMCBackendInfo LLVMCellSPUCodeGen LLVMCellSPUInfo LLVMCppBackend LLVMCppBackendInfo LLVMMBlazeAsmParser LLVMMBlazeCodeGen LLVMMBlazeDisassembler LLVMMBlazeInfo LLVMMC LLVMMCParser LLVMMSP430CodeGen LLVMMSP430Info LLVMMipsCodeGen LLVMMipsInfo LLVMPTXCodeGen LLVMPTXInfo LLVMPowerPCCodeGen LLVMPowerPCInfo LLVMSparcCodeGen LLVMSparcInfo LLVMSupport LLVMSystemZCodeGen LLVMSystemZInfo LLVMX86AsmParser LLVMX86CodeGen LLVMX86Disassembler LLVMX86Info LLVMXCoreCodeGen LLVMXCoreInfo)
set(MSVC_LIB_DEPS_LLVMMCJIT LLVMExecutionEngine LLVMSupport LLVMTarget)
set(MSVC_LIB_DEPS_LLVMMCParser LLVMMC LLVMSupport)
set(MSVC_LIB_DEPS_LLVMMSP430AsmPrinter LLVMMC LLVMSupport)
set(MSVC_LIB_DEPS_LLVMMSP430CodeGen LLVMAsmPrinter LLVMCodeGen LLVMCore LLVMMC LLVMMSP430AsmPrinter LLVMMSP430Info LLVMSelectionDAG LLVMSupport LLVMTarget)
set(MSVC_LIB_DEPS_LLVMMSP430Info LLVMMC LLVMSupport)
set(MSVC_LIB_DEPS_LLVMMipsCodeGen LLVMAsmPrinter LLVMCodeGen LLVMCore LLVMMC LLVMMipsInfo LLVMSelectionDAG LLVMSupport LLVMTarget)
set(MSVC_LIB_DEPS_LLVMMipsInfo LLVMMC LLVMSupport)
set(MSVC_LIB_DEPS_LLVMObject LLVMSupport)
set(MSVC_LIB_DEPS_LLVMPTXCodeGen LLVMAsmPrinter LLVMCodeGen LLVMCore LLVMMC LLVMPTXInfo LLVMSelectionDAG LLVMSupport LLVMTarget)
set(MSVC_LIB_DEPS_LLVMPTXInfo LLVMMC LLVMSupport)
set(MSVC_LIB_DEPS_LLVMPowerPCAsmPrinter LLVMMC LLVMSupport)
set(MSVC_LIB_DEPS_LLVMPowerPCCodeGen LLVMAnalysis LLVMAsmPrinter LLVMCodeGen LLVMCore LLVMMC LLVMPowerPCAsmPrinter LLVMPowerPCInfo LLVMSelectionDAG LLVMSupport LLVMTarget)
set(MSVC_LIB_DEPS_LLVMPowerPCInfo LLVMMC LLVMSupport)
set(MSVC_LIB_DEPS_LLVMScalarOpts LLVMAnalysis LLVMCore LLVMInstCombine LLVMSupport LLVMTarget LLVMTransformUtils)
set(MSVC_LIB_DEPS_LLVMSelectionDAG LLVMAnalysis LLVMCodeGen LLVMCore LLVMMC LLVMSupport LLVMTarget LLVMTransformUtils)
set(MSVC_LIB_DEPS_LLVMSparcCodeGen LLVMAsmPrinter LLVMCodeGen LLVMCore LLVMMC LLVMSelectionDAG LLVMSparcInfo LLVMSupport LLVMTarget)
set(MSVC_LIB_DEPS_LLVMSparcInfo LLVMMC LLVMSupport)
set(MSVC_LIB_DEPS_LLVMSupport )
set(MSVC_LIB_DEPS_LLVMSystemZCodeGen LLVMAsmPrinter LLVMCodeGen LLVMCore LLVMMC LLVMSelectionDAG LLVMSupport LLVMSystemZInfo LLVMTarget)
set(MSVC_LIB_DEPS_LLVMSystemZInfo LLVMMC LLVMSupport)
set(MSVC_LIB_DEPS_LLVMTarget LLVMCore LLVMMC LLVMSupport)
set(MSVC_LIB_DEPS_LLVMTransformUtils LLVMAnalysis LLVMCore LLVMSupport LLVMTarget LLVMipa)
set(MSVC_LIB_DEPS_LLVMX86AsmParser LLVMMC LLVMMCParser LLVMSupport LLVMTarget LLVMX86Info)
set(MSVC_LIB_DEPS_LLVMX86AsmPrinter LLVMMC LLVMSupport)
set(MSVC_LIB_DEPS_LLVMX86CodeGen LLVMAnalysis LLVMAsmPrinter LLVMCodeGen LLVMCore LLVMMC LLVMSelectionDAG LLVMSupport LLVMTarget LLVMX86AsmPrinter LLVMX86Info)
set(MSVC_LIB_DEPS_LLVMX86Disassembler LLVMMC LLVMSupport LLVMX86Info)
set(MSVC_LIB_DEPS_LLVMX86Info LLVMMC LLVMSupport)
set(MSVC_LIB_DEPS_LLVMXCoreCodeGen LLVMAsmPrinter LLVMCodeGen LLVMCore LLVMMC LLVMSelectionDAG LLVMSupport LLVMTarget LLVMXCoreInfo)
set(MSVC_LIB_DEPS_LLVMXCoreInfo LLVMMC LLVMSupport)
set(MSVC_LIB_DEPS_LLVMipa LLVMAnalysis LLVMCore LLVMSupport)
set(MSVC_LIB_DEPS_LLVMipo LLVMAnalysis LLVMCore LLVMScalarOpts LLVMSupport LLVMTarget LLVMTransformUtils LLVMipa)