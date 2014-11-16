//===-- MBlazeDisassembler.h - Disassembler for MicroBlaze  -----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file is part of the MBlaze Disassembler. It it the header for
// MBlazeDisassembler, a subclass of MCDisassembler.
//
//===----------------------------------------------------------------------===//

#ifndef MBLAZEDISASSEMBLER_H
#define MBLAZEDISASSEMBLER_H

#include "llvm/MC/MCDisassembler.h"

namespace llvm {

class MCInst;
class MemoryObject;
class raw_ostream;

/// Disassembler for all MBlaze platforms.
class MBlazeDisassembler : public MCDisassembler {
public:
  MBlazeDisassembler(const MCSubtargetInfo &STI, MCContext &Ctx) :
    MCDisassembler(STI, Ctx) {
  }

  ~MBlazeDisassembler() {
  }

  /// getInstruction - See MCDisassembler.
  DecodeStatus getInstruction(MCInst &instr, uint64_t &Size,
                              ArrayRef<uint8_t> Bytes, uint64_t Address,
                              raw_ostream &VStream,
                              raw_ostream &CStream) const override;
};

} // namespace llvm

#endif
