//===--- TargetBuiltins.h - Target specific builtin IDs -------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef TARGET_BUILTINS_H
#define TARGET_BUILTINS_H

#include "Builtins.h"
#undef PPC

namespace ellcc {
  /// Alpha builtins
  namespace Alpha {
    enum {
        LastTIBuiltin = ellcc::Builtin::FirstTSBuiltin-1,
#define BUILTIN(ID, TYPE, ATTRS) BI##ID,
#include "AlphaBuiltins.def"
        LastTSBuiltin
    };
  }
    
  /// X86 builtins
  namespace X86 {
    enum {
        LastTIBuiltin = ellcc::Builtin::FirstTSBuiltin-1,
#define BUILTIN(ID, TYPE, ATTRS) BI##ID,
#include "X86Builtins.def"
        LastTSBuiltin
    };
  }

  /// PPC builtins
  namespace PPC {
    enum {
        LastTIBuiltin = ellcc::Builtin::FirstTSBuiltin-1,
#define BUILTIN(ID, TYPE, ATTRS) BI##ID,
#include "PPCBuiltins.def"
        LastTSBuiltin
    };
  }

  /// Nios2 builtins
  namespace Nios2 {
    enum {
        LastTIBuiltin = ellcc::Builtin::FirstTSBuiltin-1,
#define BUILTIN(ID, TYPE, ATTRS) BI##ID,
#include "Nios2Builtins.def"
        LastTSBuiltin
    };
  }

  /// CellSPU builtins
  namespace CellSPU {
    enum {
        LastTIBuiltin = ellcc::Builtin::FirstTSBuiltin-1,
#define BUILTIN(ID, TYPE, ATTRS) BI##ID,
#include "CellSPUBuiltins.def"
        LastTSBuiltin
    };
  }

  /// Mips builtins
  namespace Mips {
    enum {
        LastTIBuiltin = ellcc::Builtin::FirstTSBuiltin-1,
#define BUILTIN(ID, TYPE, ATTRS) BI##ID,
#include "MipsBuiltins.def"
        LastTSBuiltin
    };
  }
    
  /// Msp430 builtins
  namespace Msp430 {
    enum {
        LastTIBuiltin = ellcc::Builtin::FirstTSBuiltin-1,
#define BUILTIN(ID, TYPE, ATTRS) BI##ID,
#include "Msp430Builtins.def"
        LastTSBuiltin
    };
  }

} // end namespace ellcc.

#endif
