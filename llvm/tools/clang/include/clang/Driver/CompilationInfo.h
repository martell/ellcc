//===--- CompilationInfo.h - Clang Compilation Information ------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_DRIVER_COMPILATION_INFO_H
#define LLVM_CLANG_DRIVER_COMPILATION_INFO_H

#include "llvm/MC/YAML.h"

namespace llvm {
namespace compilationinfo {

  typedef std::vector<std::string> StrSequence;
  struct Compiler {
    StrSequence options;
  };

  struct Assembler {
    std::string exe;
    StrSequence options;
  };

  struct Linker {
    std::string exe;
    StrSequence options;
    std::string static_crt1;
    std::string dynamic_crt1;
    std::string crtbegin;
    std::string crtend;
    std::string library_path;
  };

  struct CompilationInfo {
    Compiler compiler;
    Assembler assembler;
    Linker linker;
  };
} // end namespace compilationinfo
} // end namespace llvm

LLVM_YAML_IS_SEQUENCE_VECTOR(std::string)

namespace llvm {
namespace yaml {
#if RICH
template <>
struct MappingTraits<llvm::compilationinfo::Opt> {
  static void mapping(llvm::yaml::IO &io, llvm::compilationinfo::Opt &option) {
    io.mapOptional("value", option.value);
  }
};
#endif

template <>
struct MappingTraits<llvm::compilationinfo::Compiler> {
  static void mapping(llvm::yaml::IO &io,
                      llvm::compilationinfo::Compiler &compiler) {
    io.mapRequired("options", compiler.options);
  }
};

template <>
struct MappingTraits<llvm::compilationinfo::Assembler> {
  static void mapping(llvm::yaml::IO &io,
                      llvm::compilationinfo::Assembler &assembler) {
    io.mapOptional("exe", assembler.exe);
    io.mapOptional("options", assembler.options);
  }
};

template <>
struct MappingTraits<llvm::compilationinfo::Linker> {
  static void mapping(llvm::yaml::IO &io,
                      llvm::compilationinfo::Linker &linker) {
    io.mapRequired("exe", linker.exe);
    io.mapOptional("options", linker.options);
    io.mapOptional("static_crt1", linker.static_crt1);
    io.mapOptional("dynamic_crt1", linker.dynamic_crt1);
    io.mapOptional("crtbegin", linker.crtbegin);
    io.mapOptional("crtend", linker.crtend);
    io.mapOptional("library_path", linker.library_path);
  }
};

template <>
struct MappingTraits<llvm::compilationinfo::CompilationInfo> {
  static void mapping(llvm::yaml::IO &io,
                      llvm::compilationinfo::CompilationInfo &config) {
    io.mapRequired("compiler", config.compiler);
    io.mapRequired("assembler", config.assembler);
    io.mapRequired("linker", config.linker);
  }
};
} // end namespace yaml
} // end namespace llvm

#endif
