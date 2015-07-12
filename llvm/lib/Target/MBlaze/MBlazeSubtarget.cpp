//===-- MBlazeSubtarget.cpp - MBlaze Subtarget Information ----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the MBlaze specific subclass of TargetSubtargetInfo.
//
//===----------------------------------------------------------------------===//

#include "MBlazeSubtarget.h"
#include "MBlazeTargetMachine.h"
#include "MBlaze.h"
#include "MBlazeRegisterInfo.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/TargetRegistry.h"

#define DEBUG_TYPE "mb-subtarget"
#define GET_SUBTARGETINFO_TARGET_DESC
#define GET_SUBTARGETINFO_CTOR
#include "MBlazeGenSubtargetInfo.inc"

using namespace llvm;

MBlazeSubtarget::MBlazeSubtarget(const Triple &TT,
                                 const std::string &CPU,
                                 const std::string &FS,
                                 bool little,
                                 MBlazeTargetMachine &TM):
  MBlazeGenSubtargetInfo(TT, CPU, FS),
  HasBarrel(false), HasDiv(false), HasMul(false), HasPatCmp(false),
  HasFPU(false), HasMul64(false), HasSqrt(false),
  TargetTriple(TT), TSInfo(),
  InstrInfo(MBlazeInstrInfo::create(*this)),
  FrameLowering(MBlazeFrameLowering::create(*this)),
  TLInfo(MBlazeTargetLowering::create(TM, *this))
{
  // Parse features string.
  std::string CPUName = CPU;
  if (CPUName.empty())
    CPUName = "mblaze";
  ParseSubtargetFeatures(CPUName, FS);

  // Only use instruction scheduling if the selected CPU has an instruction
  // itinerary (the default CPU is the only one that doesn't).
  HasItin = CPUName != "mblaze";
  DEBUG(dbgs() << "CPU " << CPUName << "(" << HasItin << ")\n");

  // Initialize scheduling itinerary for the specified CPU.
  InstrItins = getInstrItineraryForCPU(CPUName);
}

bool MBlazeSubtarget:: enablePostRAScheduler() const { return true; }

void
MBlazeSubtarget::getCriticalPathRCs(RegClassVector& CriticalPathRCs) const {
  CriticalPathRCs.clear();
  CriticalPathRCs.push_back(&MBlaze::GPRRegClass);
}
