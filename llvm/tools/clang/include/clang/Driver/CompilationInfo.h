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

  /// ExpandArg - Perform a substitution on an argument.
  void ExpandArg(std::string &arg, const char *sub, const std::string &value);

  /// ExpandArgWithAll - Perform all expansions on an argument.
  void ExpandArgWithAll(std::string &arg, const driver::Driver &TheDriveri,
                        const char *Output = NULL);

  /// ExpandArgs - Expand arguments.
  /// Expand "-option value" into two entries.
  /// Make argument substitutions.
  void ExpandArgs(StrSequence &options, const driver::Driver &TheDriver,
                  const char *Output = NULL);

  struct Global {
    bool static_default;
    Global() : static_default(false) { }
  };

  struct Compiler {
    StrSequence options;
    StrSequence c_include_dirs;
    StrSequence cxx_include_dirs;

    void Expand(const driver::Driver &TheDriver) {
      ExpandArgs(options, TheDriver);
      ExpandArgs(c_include_dirs, TheDriver);
      ExpandArgs(cxx_include_dirs, TheDriver);
    }
  };

  struct Assembler {
    std::string exe;
    StrSequence output;
    StrSequence options;

    void Expand(const driver::Driver &TheDriver, const char *Output) {
      ExpandArgWithAll(exe, TheDriver);
      ExpandArgs(output, TheDriver, Output);
      ExpandArgs(options, TheDriver);
    }
  };

  struct Linker {
    std::string exe;
    StrSequence output;
    StrSequence start;
    StrSequence opt_static;
    StrSequence opt_rdynamic;
    StrSequence opt_dynamic;
    StrSequence opt_shared;
    StrSequence opt_notshared;
    StrSequence opt_pthread;
    StrSequence options;
    std::string static_crt1;
    std::string dynamic_crt1;
    std::string crtbegin;
    std::string crtend;
    StrSequence library_paths;
    StrSequence cxx_libraries;
    StrSequence profile_libraries;
    StrSequence c_libraries;

    void Expand(const driver::Driver &TheDriver, const char *Output) {
      ExpandArgWithAll(exe, TheDriver, Output);
      ExpandArgs(output, TheDriver, Output);
      ExpandArgs(start, TheDriver, Output);
      ExpandArgs(opt_static, TheDriver, Output);
      ExpandArgs(opt_rdynamic, TheDriver, Output);
      ExpandArgs(opt_dynamic, TheDriver, Output);
      ExpandArgs(opt_shared, TheDriver, Output);
      ExpandArgs(opt_notshared, TheDriver, Output);
      ExpandArgs(opt_pthread, TheDriver, Output);
      ExpandArgs(options, TheDriver, Output);
      ExpandArgWithAll(static_crt1, TheDriver, Output);
      ExpandArgWithAll(dynamic_crt1, TheDriver, Output);
      ExpandArgWithAll(crtbegin, TheDriver, Output);
      ExpandArgWithAll(crtend, TheDriver, Output);
      ExpandArgs(library_paths, TheDriver, Output);
      ExpandArgs(cxx_libraries, TheDriver, Output);
      ExpandArgs(profile_libraries, TheDriver, Output);
      ExpandArgs(c_libraries, TheDriver, Output);
    }
  };

  struct CompilationInfo {
    bool dump;
    std::string based_on;
    Global global;
    Compiler compiler;
    Assembler assembler;
    Linker linker;
    driver::Driver &TheDriver;
    /// nesting - Current nesting level for compilation info.
    int nesting;

    CompilationInfo(driver::Driver &TheDriver)
      : dump(false), TheDriver(TheDriver), nesting(0) { }

    /// The target info database.
    std::map<std::string, const char *> InfoMap;

    /// ReadInfo - Read a YAML memory buffer into the CompilerInfo object.
    bool ReadInfo(llvm::MemoryBuffer &Buffer);

    /// CheckForAndReadInfo - Check for and read compilation info if available.
    static bool CheckForAndReadInfo(const char *target,
                                    driver::Driver &TheDriver);
    /// DefineInfo - Statically define info for a target.
    void DefineInfo(const char *target, const char *info)
    { InfoMap[target] = info; }

    /// HandleBasedOn - Handle nested info files.
    void HandleBasedOn(CompilationInfo &config);
  };

} // end namespace compilationinfo
} // end namespace clang

LLVM_YAML_IS_SEQUENCE_VECTOR(std::string)

namespace llvm {
namespace yaml {

template <>
struct MappingTraits<clang::compilationinfo::Global> {
  static void mapping(llvm::yaml::IO &io,
                      clang::compilationinfo::Global &global) {
    io.mapOptional("static_default", global.static_default);
  }
};

template <>
struct MappingTraits<clang::compilationinfo::Compiler> {
  static void mapping(llvm::yaml::IO &io,
                      clang::compilationinfo::Compiler &compiler) {
    io.mapOptional("options", compiler.options);
    io.mapOptional("c_include_dirs", compiler.c_include_dirs);
    io.mapOptional("cxx_include_dirs", compiler.cxx_include_dirs);
  }
};

template <>
struct MappingTraits<clang::compilationinfo::Assembler> {
  static void mapping(llvm::yaml::IO &io,
                      clang::compilationinfo::Assembler &assembler) {
    io.mapOptional("exe", assembler.exe);
    io.mapOptional("output", assembler.output);
    io.mapOptional("options", assembler.options);
  }
};

template <>
struct MappingTraits<clang::compilationinfo::Linker> {
  static void mapping(llvm::yaml::IO &io,
                      clang::compilationinfo::Linker &linker) {
    io.mapOptional("exe", linker.exe);
    io.mapOptional("output", linker.output);
    io.mapOptional("start", linker.start);
    io.mapOptional("opt_static", linker.opt_static);
    io.mapOptional("opt_rdynamic", linker.opt_rdynamic);
    io.mapOptional("opt_dynamic", linker.opt_dynamic);
    io.mapOptional("opt_shared", linker.opt_shared);
    io.mapOptional("opt_notshared", linker.opt_notshared);
    io.mapOptional("opt_pthread", linker.opt_pthread);
    io.mapOptional("options", linker.options);
    io.mapOptional("static_crt1", linker.static_crt1);
    io.mapOptional("dynamic_crt1", linker.dynamic_crt1);
    io.mapOptional("crtbegin", linker.crtbegin);
    io.mapOptional("crtend", linker.crtend);
    io.mapOptional("library_paths", linker.library_paths);
    io.mapOptional("cxx_libraries", linker.cxx_libraries);
    io.mapOptional("profile_libraries", linker.profile_libraries);
    io.mapOptional("c_libraries", linker.c_libraries);
  }
};

template <>
struct MappingTraits<clang::compilationinfo::CompilationInfo> {
  static void mapping(llvm::yaml::IO &io,
                      clang::compilationinfo::CompilationInfo &config) {
    io.mapOptional("dump", config.dump);
    io.mapOptional("based_on", config.based_on);
    // Check for nested config files.
    config.HandleBasedOn(config);
    io.mapOptional("global", config.global);
    io.mapOptional("compiler", config.compiler);
    io.mapOptional("assembler", config.assembler);
    io.mapOptional("linker", config.linker);
  }
};
} // end namespace yaml
} // end namespace llvm

#endif
