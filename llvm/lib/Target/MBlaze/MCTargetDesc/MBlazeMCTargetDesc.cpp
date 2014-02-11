//===-- MBlazeMCTargetDesc.cpp - MBlaze Target Descriptions ---------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file provides MBlaze specific target descriptions.
//
//===----------------------------------------------------------------------===//

#include "MBlazeMCTargetDesc.h"
#include "InstPrinter/MBlazeInstPrinter.h"
#include "MBlazeMCAsmInfo.h"
#include "MBlazeTargetStreamer.h"
#include "llvm/MC/MCCodeGenInfo.h"
#include "llvm/MC/MCELF.h"
#include "llvm/MC/MCELFStreamer.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/TargetRegistry.h"

#define GET_INSTRINFO_MC_DESC
#include "MBlazeGenInstrInfo.inc"

#define GET_SUBTARGETINFO_MC_DESC
#include "MBlazeGenSubtargetInfo.inc"

#define GET_REGINFO_MC_DESC
#include "MBlazeGenRegisterInfo.inc"

using namespace llvm;

static MCInstrInfo *createMBlazeMCInstrInfo() {
  MCInstrInfo *X = new MCInstrInfo();
  InitMBlazeMCInstrInfo(X);
  return X;
}

static MCRegisterInfo *createMBlazeMCRegisterInfo(StringRef TT) {
  MCRegisterInfo *X = new MCRegisterInfo();
  InitMBlazeMCRegisterInfo(X, MBlaze::R15);
  return X;
}

static MCSubtargetInfo *createMBlazeMCSubtargetInfo(StringRef TT, StringRef CPU,
                                                    StringRef FS) {
  MCSubtargetInfo *X = new MCSubtargetInfo();
  InitMBlazeMCSubtargetInfo(X, TT, CPU, FS);
  return X;
}

static MCAsmInfo *createMBlazeMCAsmInfo(const MCRegisterInfo &MRI, StringRef TT) {
  Triple TheTriple(TT);
  switch (TheTriple.getOS()) {
  default:
    return new MBlazeMCAsmInfo();
  }
}

static MCCodeGenInfo *createMBlazeMCCodeGenInfo(StringRef TT, Reloc::Model RM,
                                                CodeModel::Model CM,
                                                CodeGenOpt::Level OL) {
  MCCodeGenInfo *X = new MCCodeGenInfo();
  if (RM == Reloc::Default)
    RM = Reloc::Static;
  if (CM == CodeModel::Default)
    CM = CodeModel::Small;
  X->InitMCCodeGenInfo(RM, CM, OL);
  return X;
}

#if RICH
class MBlazeTargetAsmStreamer : public MBlazeTargetStreamer {
  formatted_raw_ostream &OS;

public:
  MBlazeTargetAsmStreamer(formatted_raw_ostream &OS);
};

MBlazeTargetAsmStreamer::
MBlazeTargetAsmStreamer(MCStreamer &S, 
                        formatted_raw_ostream &OS)
    : OS(OS) {}

class MBlazeTargetELFStreamer : public MBlazeTargetStreamer {
public:
  MCELFStreamer &getStreamer();
  MBlazeTargetELFStreamer(MCStreamer &S, , const MCSubtargetInfo &STI);
};
#endif

// Pin vtable to this file.
void MBlazeTargetStreamer::anchor() {}

MBlazeTargetStreamer::MBlazeTargetStreamer(MCStreamer &S) : MCTargetStreamer(S) {}

MBlazeTargetAsmStreamer::
MBlazeTargetAsmStreamer(MCStreamer &S, formatted_raw_ostream &OS)
  : MBlazeTargetStreamer(S), OS(OS)
{
}

MBlazeTargetELFStreamer::
MBlazeTargetELFStreamer(MCStreamer &S, const MCSubtargetInfo &STI)
  : MBlazeTargetStreamer(S)
{
}

static MCStreamer *createMCStreamer(const Target &T, StringRef TT,
                                    MCContext &Ctx, MCAsmBackend &MAB,
                                    raw_ostream &OS, MCCodeEmitter *Emitter,
                                    const MCSubtargetInfo &STI,
                                    bool RelaxAll, bool NoExecStack) {
  MCStreamer *S =
    createELFStreamer(Ctx, MAB, OS, Emitter, RelaxAll, NoExecStack);
  new MBlazeTargetELFStreamer(*S, STI);
  return S;
}

static MCStreamer *
createMCAsmStreamer(MCContext &Ctx, formatted_raw_ostream &OS,
                    bool isVerboseAsm, bool useCFI, bool useDwarfDirectory,
                    MCInstPrinter *InstPrint, MCCodeEmitter *CE,
                    MCAsmBackend *TAB, bool ShowInst) {
  MCStreamer *S =
    llvm::createAsmStreamer(Ctx, OS, isVerboseAsm, useCFI, useDwarfDirectory,
                            InstPrint, CE, TAB, ShowInst);
  new MBlazeTargetAsmStreamer(*S, OS);
  return S;
}

static MCInstPrinter *createMBlazeMCInstPrinter(const Target &T,
                                                unsigned SyntaxVariant,
                                                const MCAsmInfo &MAI,
                                                const MCInstrInfo &MII,
                                                const MCRegisterInfo &MRI,
                                                const MCSubtargetInfo &STI) {
  if (SyntaxVariant == 0)
    return new MBlazeInstPrinter(MAI, MII, MRI);
  return 0;
}

// Force static initialization.
extern "C" void LLVMInitializeMBlazeTargetMC() {
  // Register the MC asm info.
  RegisterMCAsmInfoFn X(TheMBlazeTarget, createMBlazeMCAsmInfo);

  // Register the MC codegen info.
  TargetRegistry::RegisterMCCodeGenInfo(TheMBlazeTarget,
                                        createMBlazeMCCodeGenInfo);

  // Register the MC instruction info.
  TargetRegistry::RegisterMCInstrInfo(TheMBlazeTarget, createMBlazeMCInstrInfo);

  // Register the MC register info.
  TargetRegistry::RegisterMCRegInfo(TheMBlazeTarget,
                                    createMBlazeMCRegisterInfo);

  // Register the MC subtarget info.
  TargetRegistry::RegisterMCSubtargetInfo(TheMBlazeTarget,
                                          createMBlazeMCSubtargetInfo);

  // Register the MC code emitter
  TargetRegistry::RegisterMCCodeEmitter(TheMBlazeTarget,
                                        llvm::createMBlazeMCCodeEmitter);

  // Register the asm streamer
  TargetRegistry::RegisterAsmStreamer(TheMBlazeTarget,
                                       createMCAsmStreamer);
  // Register the asm backend
  TargetRegistry::RegisterMCAsmBackend(TheMBlazeTarget,
                                       createMBlazeAsmBackend);

  // Register the object streamer
  TargetRegistry::RegisterMCObjectStreamer(TheMBlazeTarget,
                                           createMCStreamer);

  // Register the MCInstPrinter.
  TargetRegistry::RegisterMCInstPrinter(TheMBlazeTarget,
                                        createMBlazeMCInstPrinter);
}
