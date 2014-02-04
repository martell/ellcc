//===-- MBlazeTargetStreamer.h - MBlaze Target Streamer ------------*- C++ -*--===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef MBLAZETARGETSTREAMER_H
#define MBLAZETARGETSTREAMER_H

#include "llvm/MC/MCELFStreamer.h"
#include "llvm/MC/MCStreamer.h"

namespace llvm {
class MBlazeTargetStreamer : public MCTargetStreamer {
  virtual void anchor();

public:
  MBlazeTargetStreamer(MCStreamer &S);
};

// This part is for ascii assembly output
class MBlazeTargetAsmStreamer : public MBlazeTargetStreamer {
  formatted_raw_ostream &OS;

public:
  MBlazeTargetAsmStreamer(MCStreamer &S, formatted_raw_ostream &OS);
};

// This part is for ELF object output
class MBlazeTargetELFStreamer : public MBlazeTargetStreamer {

public:
  MCELFStreamer &getStreamer();
  MBlazeTargetELFStreamer(MCStreamer &S, const MCSubtargetInfo &STI);
};
}

#endif
