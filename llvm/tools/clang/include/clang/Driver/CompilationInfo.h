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

namespace clang {
namespace driver {
  class Driver;
}
}

namespace clang {
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
    static std::map<const char *, const char *> InfoMap;

    /// ExpandArg - Perform a substitution on an argument.
    static void ExpandArg(std::string &arg, const char *sub,
                          const std::string &value);
    /// ExpandArgWithAll - Perform all expansions on an argument.
    static void ExpandArgWithAll(std::string &arg, driver:: Driver &TheDriver);

    /// ExpandArgs - Expand arguments.
    /// Expand "-option value" into two entries.
    /// Make argument substitutions.
    static void ExpandArgs(StrSequence &options, driver::Driver &TheDriver);

    /// ReadInfo - Read a YAML memory buffer into the CompilerInfo object.
    static void ReadInfo(llvm::MemoryBuffer &Buffer, driver::Driver &TheDriver);

    /// CheckForAndReadInfo - Check for and read compilation info if available.
    static bool CheckForAndReadInfo(const char *target,
                                    driver::Driver &TheDriver);
    /// DefineInfo - Statically define info for a target.
    static void DefineInfo(const char *target, const char *info)
    { InfoMap[target] = info; }
  };

} // end namespace compilationinfo
} // end namespace clang

LLVM_YAML_IS_SEQUENCE_VECTOR(std::string)

namespace llvm {
namespace yaml {

template <>
struct MappingTraits<clang::compilationinfo::Compiler> {
  static void mapping(llvm::yaml::IO &io,
                      clang::compilationinfo::Compiler &compiler) {
    io.mapRequired("options", compiler.options);
  }
};

template <>
struct MappingTraits<clang::compilationinfo::Assembler> {
  static void mapping(llvm::yaml::IO &io,
                      clang::compilationinfo::Assembler &assembler) {
    io.mapOptional("exe", assembler.exe);
    io.mapOptional("options", assembler.options);
  }
};

template <>
struct MappingTraits<clang::compilationinfo::Linker> {
  static void mapping(llvm::yaml::IO &io,
                      clang::compilationinfo::Linker &linker) {
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
struct MappingTraits<clang::compilationinfo::CompilationInfo> {
  static void mapping(llvm::yaml::IO &io,
                      clang::compilationinfo::CompilationInfo &config) {
    io.mapRequired("compiler", config.compiler);
    io.mapRequired("assembler", config.assembler);
    io.mapRequired("linker", config.linker);
  }
};
} // end namespace yaml
} // end namespace llvm

#endif
