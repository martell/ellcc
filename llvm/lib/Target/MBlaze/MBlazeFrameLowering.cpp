//===-- MBlazeFrameLowering.cpp - MBlaze Frame Information ---------------====//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the MBlaze implementation of TargetFrameLowering class.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "mblaze-frame-lowering"

#include "MBlazeFrameLowering.h"
#include "InstPrinter/MBlazeInstPrinter.h"
#include "MBlazeInstrInfo.h"
#include "MBlazeMachineFunction.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetOptions.h"

using namespace llvm;

static cl::opt<bool> MBDisableStackAdjust(
  "disable-mblaze-stack-adjust",
  cl::init(false),
  cl::desc("Disable MBlaze stack layout adjustment."),
  cl::Hidden);

//===----------------------------------------------------------------------===//
//
// Stack Frame Processing methods
// +----------------------------+
//
// The stack is allocated decrementing the stack pointer on
// the first instruction of a function prologue. Once decremented,
// all stack references are are done through a positive offset
// from the stack/frame pointer, so the stack is considered
// to grow up.
//
//===----------------------------------------------------------------------===//

static void determineFrameLayout(MachineFunction &MF) {
  MachineFrameInfo &MFI = MF.getFrameInfo();
  MBlazeFunctionInfo *MBlazeFI = MF.getInfo<MBlazeFunctionInfo>();

  // Replace the dummy '0' SPOffset by the negative offsets, as explained on
  // LowerFORMAL_ARGUMENTS. Leaving '0' for while is necessary to avoid
  // the approach done by calculateFrameObjectOffsets to the stack frame.
  MBlazeFI->adjustLoadArgsFI(&MFI);
  MBlazeFI->adjustStoreVarArgsFI(&MFI);

  // Get the number of bytes to allocate from the FrameInfo
  unsigned FrameSize = MFI.getStackSize();
  DEBUG(dbgs() << "Original Frame Size: " << FrameSize << "\n" );
  DEBUG(dbgs() << "Adjusted Frame Size: " << FrameSize << "\n" );

  // Get the alignments provided by the target, and the maximum alignment
  // (if any) of the fixed frame objects.
  // unsigned MaxAlign = MFI.getMaxAlignment();
  unsigned TargetAlign =
    MF.getSubtarget().getFrameLowering()->getStackAlignment();
  unsigned AlignMask = TargetAlign - 1;

  // Make sure the frame is aligned.
  FrameSize = (FrameSize + AlignMask) & ~AlignMask;
  MFI.setStackSize(FrameSize);
  DEBUG(dbgs() << "Aligned Frame Size: " << FrameSize << "\n" );
}

int MBlazeFrameLowering::getFrameIndexReference(const MachineFunction &MF,
                                                int FI,
                                                unsigned &FrameReg) const {
  const MBlazeFunctionInfo *MBlazeFI = MF.getInfo<MBlazeFunctionInfo>();
  if (MBlazeFI->hasReplacement(FI))
    FI = MBlazeFI->getReplacement(FI);
  return TargetFrameLowering::getFrameIndexReference(MF,FI, FrameReg);
}

// hasFP - Return true if the specified function should have a dedicated frame
// pointer register.  This is true if the function has variable sized allocas or
// if frame pointer elimination is disabled.
bool MBlazeFrameLowering::hasFP(const MachineFunction &MF) const {
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  return MF.getTarget().Options.DisableFramePointerElim(MF) ||
         MFI.hasVarSizedObjects();
}

void MBlazeFrameLowering::emitPrologue(MachineFunction &MF,
                                       MachineBasicBlock &MBB) const {
  MachineFrameInfo &MFI = MF.getFrameInfo();
  const MBlazeInstrInfo &TII =
    *static_cast<const MBlazeInstrInfo*>(MF.getSubtarget().getInstrInfo());
  MBlazeFunctionInfo *MBlazeFI = MF.getInfo<MBlazeFunctionInfo>();
  MachineBasicBlock::iterator MBBI = MBB.begin();
  DebugLoc DL = MBBI != MBB.end() ? MBBI->getDebugLoc() : DebugLoc();

  CallingConv::ID CallConv = MF.getFunction()->getCallingConv();
  bool requiresRA = CallConv == CallingConv::MBLAZE_INTR;

  // Determine the correct frame layout
  determineFrameLayout(MF);

  // Get the number of bytes to allocate from the FrameInfo.
  unsigned StackSize = MFI.getStackSize();

  // No need to allocate space on the stack.
  if (StackSize == 0 && !MFI.adjustsStack() && !requiresRA) return;

  int FPOffset = MBlazeFI->getFPStackOffset();
  int RAOffset = MBlazeFI->getRAStackOffset();

  // Adjust stack : addi R1, R1, -imm
  BuildMI(MBB, MBBI, DL, TII.get(MBlaze::ADDIK), MBlaze::R1)
      .addReg(MBlaze::R1).addImm(-StackSize);

  // swi  R15, R1, stack_loc
  if (MFI.adjustsStack() || requiresRA) {
    BuildMI(MBB, MBBI, DL, TII.get(MBlaze::SWI))
        .addReg(MBlaze::R15).addReg(MBlaze::R1).addImm(RAOffset);
  }

  if (hasFP(MF)) {
    // swi  R19, R1, stack_loc
    BuildMI(MBB, MBBI, DL, TII.get(MBlaze::SWI))
      .addReg(MBlaze::R19).addReg(MBlaze::R1).addImm(FPOffset);

    // add R19, R1, R0
    BuildMI(MBB, MBBI, DL, TII.get(MBlaze::ADD), MBlaze::R19)
      .addReg(MBlaze::R1).addReg(MBlaze::R0);
  }
}

void MBlazeFrameLowering::emitEpilogue(MachineFunction &MF,
                                   MachineBasicBlock &MBB) const {
  MachineBasicBlock::iterator MBBI = MBB.getLastNonDebugInstr();
  MachineFrameInfo &MFI = MF.getFrameInfo();
  MBlazeFunctionInfo *MBlazeFI = MF.getInfo<MBlazeFunctionInfo>();
  const MBlazeInstrInfo &TII =
    *static_cast<const MBlazeInstrInfo*>(MF.getSubtarget().getInstrInfo());

  DebugLoc dl = MBBI->getDebugLoc();

  CallingConv::ID CallConv = MF.getFunction()->getCallingConv();
  bool requiresRA = CallConv == CallingConv::MBLAZE_INTR;

  // Get the FI's where RA and FP are saved.
  int FPOffset = MBlazeFI->getFPStackOffset();
  int RAOffset = MBlazeFI->getRAStackOffset();

  if (hasFP(MF)) {
    // add R1, R19, R0
    BuildMI(MBB, MBBI, dl, TII.get(MBlaze::ADD), MBlaze::R1)
      .addReg(MBlaze::R19).addReg(MBlaze::R0);

    // lwi  R19, R1, stack_loc
    BuildMI(MBB, MBBI, dl, TII.get(MBlaze::LWI), MBlaze::R19)
      .addReg(MBlaze::R1).addImm(FPOffset);
  }

  // lwi R15, R1, stack_loc
  if (MFI.adjustsStack() || requiresRA) {
    BuildMI(MBB, MBBI, dl, TII.get(MBlaze::LWI), MBlaze::R15)
      .addReg(MBlaze::R1).addImm(RAOffset);
  }

  // Get the number of bytes from FrameInfo
  int StackSize = (int) MFI.getStackSize();

  // addi R1, R1, imm
  if (StackSize) {
    BuildMI(MBB, MBBI, dl, TII.get(MBlaze::ADDIK), MBlaze::R1)
      .addReg(MBlaze::R1).addImm(StackSize);
  }
}

// Eliminate ADJCALLSTACKDOWN/ADJCALLSTACKUP pseudo instructions
MachineBasicBlock::iterator MBlazeFrameLowering::
eliminateCallFramePseudoInstr(MachineFunction &MF, MachineBasicBlock &MBB,
                              MachineBasicBlock::iterator I) const {
  const MBlazeInstrInfo &TII =
    *static_cast<const MBlazeInstrInfo*>(MF.getSubtarget().getInstrInfo());
  if (!hasReservedCallFrame(MF)) {
    // If we have a frame pointer, turn the adjcallstackup instruction into a
    // 'addi r1, r1, -<amt>' and the adjcallstackdown instruction into
    // 'addi r1, r1, <amt>'
    MachineInstr *Old = &*I;
    int Amount = Old->getOperand(0).getImm() + 4;
    if (Amount != 0) {
      // We need to keep the stack aligned properly.  To do this, we round the
      // amount of space needed for the outgoing arguments up to the next
      // alignment boundary.
      unsigned Align = getStackAlignment();
      Amount = (Amount+Align-1)/Align*Align;

      MachineInstr *New;
      if (Old->getOpcode() == MBlaze::ADJCALLSTACKDOWN) {
        New = BuildMI(MF,Old->getDebugLoc(), TII.get(MBlaze::ADDIK),MBlaze::R1)
                .addReg(MBlaze::R1).addImm(-Amount);
      } else {
        assert(Old->getOpcode() == MBlaze::ADJCALLSTACKUP);
        New = BuildMI(MF,Old->getDebugLoc(), TII.get(MBlaze::ADDIK),MBlaze::R1)
                .addReg(MBlaze::R1).addImm(Amount);
      }

      // Replace the pseudo instruction with a new instruction...
      MBB.insert(I, New);
    }
  }

  // Simply discard ADJCALLSTACKDOWN, ADJCALLSTACKUP instructions.
  return MBB.erase(I);
}


const MBlazeFrameLowering *MBlazeFrameLowering::
create(const MBlazeSubtarget &ST) {
  return new MBlazeFrameLowering(ST);
}
