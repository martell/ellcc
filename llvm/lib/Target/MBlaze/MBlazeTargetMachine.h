//===-- MBlazeTargetMachine.h - Define TargetMachine for MBlaze -*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file declares the MBlaze specific subclass of TargetMachine.
//
//===----------------------------------------------------------------------===//

#ifndef MBLAZE_TARGETMACHINE_H
#define MBLAZE_TARGETMACHINE_H

#include "MBlazeIntrinsicInfo.h"
#include "MBlazeSubtarget.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/Target/TargetFrameLowering.h"
#include "llvm/Target/TargetMachine.h"

namespace llvm {
  class formatted_raw_ostream;

  class MBlazeTargetMachine : public LLVMTargetMachine {
    std::unique_ptr<TargetLoweringObjectFile> TLOF;
    MBlazeSubtarget        *Subtarget;
    MBlazeSubtarget        DefaultSubtarget;
    MBlazeIntrinsicInfo    IntrinsicInfo;

  public:
    MBlazeTargetMachine(const Target &T, StringRef TT,
                        StringRef CPU, StringRef FS,
                        const TargetOptions &Options,
                        Reloc::Model RM, CodeModel::Model CM,
                        CodeGenOpt::Level OL);
  ~MBlazeTargetMachine() override;

    const MBlazeSubtarget *getSubtargetImpl() const override {
      if (Subtarget)
        return Subtarget;
      return &DefaultSubtarget;
    }

    const TargetIntrinsicInfo *getIntrinsicInfo() const override
    { return &IntrinsicInfo; }

    // Pass Pipeline Configuration
    TargetPassConfig *createPassConfig(PassManagerBase &PM) override;

    TargetLoweringObjectFile *getObjFileLowering() const override {
      return TLOF.get();
    }
  };
} // End llvm namespace

#endif
