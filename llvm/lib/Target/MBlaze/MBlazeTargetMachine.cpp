//===-- MBlazeTargetMachine.cpp - Define TargetMachine for MBlaze ---------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Implements the info about MBlaze target spec.
//
//===----------------------------------------------------------------------===//

#include "MBlazeTargetMachine.h"
#include "MBlaze.h"
#include "MBlazeTargetObjectFile.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/PassManager.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Target/TargetOptions.h"
using namespace llvm;

extern "C" void LLVMInitializeMBlazeTarget() {
  // Register the target.
  RegisterTargetMachine<MBlazeTargetMachine> X(TheMBlazeTarget);
}

static std::string computeDataLayout(bool BigEndian=true) {
  // Endian.
  std::string Ret = BigEndian ? "E" : "e";

  Ret += "-m:e";

  // Pointers are 32 bits and aligned to 32 bits.
  Ret += "-p:32:32";

  // Integer registers are 32 bits.
  Ret += "-n32";

  // The stack is 32 bit aligned.
  Ret += "-S32";

  return Ret;
}

// DataLayout --> Big-endian, 32-bit pointer/ABI/alignment
// The stack is always 8 byte aligned
// On function prologue, the stack is created by decrementing
// its pointer. Once decremented, all references are done with positive
// offset from the stack/frame pointer, using StackGrowsUp enables
// an easier handling.
MBlazeTargetMachine::
MBlazeTargetMachine(const Target &T, StringRef TT,
                    StringRef CPU, StringRef FS, const TargetOptions &Options,
                    Reloc::Model RM, CodeModel::Model CM,
                    CodeGenOpt::Level OL)
  : LLVMTargetMachine(T, TT, CPU, FS, Options, RM, CM, OL),
    TLOF(make_unique<MBlazeTargetObjectFile>()),
    DL(computeDataLayout()), Subtarget(nullptr),
    DefaultSubtarget(TT, CPU, FS, /* isL:ittle */ false, *this) {
  Subtarget = &DefaultSubtarget;
  initAsmInfo();
}

MBlazeTargetMachine::~MBlazeTargetMachine() {}

namespace {
/// MBlaze Code Generator Pass Configuration Options.
class MBlazePassConfig : public TargetPassConfig {
public:
  MBlazePassConfig(MBlazeTargetMachine *TM, PassManagerBase &PM)
    : TargetPassConfig(TM, PM) {}

  MBlazeTargetMachine &getMBlazeTargetMachine() const {
    return getTM<MBlazeTargetMachine>();
  }

  virtual bool addInstSelector();
  virtual void addPreEmitPass();
};
} // namespace

TargetPassConfig *MBlazeTargetMachine::createPassConfig(PassManagerBase &PM) {
  return new MBlazePassConfig(this, PM);
}

// Install an instruction selector pass using
// the ISelDag to gen MBlaze code.
bool MBlazePassConfig::addInstSelector() {
  addPass(createMBlazeISelDag(getMBlazeTargetMachine()));
  return false;
}

// Implemented by targets that want to run passes immediately before
// machine code is emitted. return true if -print-machineinstrs should
// print out the code after the passes.
void MBlazePassConfig::addPreEmitPass() {
  addPass(createMBlazeDelaySlotFillerPass(getMBlazeTargetMachine()));
}
