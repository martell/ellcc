#include "clang/Driver/CompilationInfo.h"
#include "clang/Driver/DriverDiagnostic.h"
#include "clang/Driver/Driver.h"

using namespace clang::compilationinfo;

namespace {
const char ellcc_linux[] =
  "global:\n"
  "  static_default: true\n"
  "compiler:\n"
  "  cxx_include_dirs:\n"
  "    - $R/include/c++\n"
  "assembler:\n"
  "  output:\n"
  "    - -o $O\n"
  "linker:\n"
  "  exe: $E/ecc-ld$X\n"
  "  output:\n"
  "    - -o $O\n"
  "  start:\n"
  "    - -e _start\n"
  "  opt_static:\n"
  "    - -Bstatic\n"
  "  opt_rdynamic:\n"
  "    - -export-dynamic\n"
  "  opt_dynamic:\n"
  "    - -Bdynamic\n"
  "  opt_shared:\n"
  "    - -shared\n"
  "  opt_notshared:\n"
  "    - -dynamic-linker /usr/libexec/ld.so\n"
  "  opt_pthread:\n"
  "    - -pthread\n"
  "  cxx_libraries:\n"
  "    - -lc++\n"
  "    - -lm\n"
  "  profile_libraries:\n"
  "    - -lprofile_rt\n"
  "  c_libraries:\n"
  "    - -(\n"                  // This is need for ARM and profiling.
  "    - -lc\n"
  "    - -lcompiler_rt\n"
  "    - -)\n"
  "";

const char arm_ellcc_linux[] =
  "based_on: ellcc-linux\n"
  "compiler:\n"
  "  options:\n"
  "    - -target arm-ellcc-linux\n"
  "  c_include_dirs:\n"
  "    - $R/include/arm\n"
  "    - $R/include\n"
  "assembler:\n"
  "  exe: $E/arm-elf-as$X\n"
  "linker:\n"
  "  options:\n"
  "    - -m armelf_linux_eabi\n"
  "    - --build-id\n"
  "    - --hash-style=gnu\n"
  "    - --eh-frame-hdr\n"
  "  static_crt1: $R/lib/arm/linux/crt1.o\n"
  "  dynamic_crt1: $R/lib/arm/linux/Scrt1.o\n"
  "  crtbegin: $R/lib/arm/linux/crtbegin.o\n"
  "  crtend: $R/lib/arm/linux/crtend.o\n"
  "  library_paths:\n"
  "    - -L $R/lib/arm/linux\n"
  "";

const char armeb_ellcc_linux[] =
  "based_on: ellcc-linux\n"
  "compiler:\n"
  "  options:\n"
  "    - -target armeb-ellcc-linux\n"
  "  c_include_dirs:\n"
  "    - $R/include/arm\n"
  "    - $R/include\n"
  "assembler:\n"
  "  exe: $E/arm-elf-as$X\n"
  "  options:\n"
  "    - -EB\n"
  "linker:\n"
  "  options:\n"
  "    - -m armelfb_linux_eabi\n"
  "    - --build-id\n"
  "    - --hash-style=gnu\n"
  "    - --eh-frame-hdr\n"
  "  static_crt1: $R/lib/armeb/linux/crt1.o\n"
  "  dynamic_crt1: $R/lib/armeb/linux/Scrt1.o\n"
  "  crtbegin: $R/lib/armeb/linux/crtbegin.o\n"
  "  crtend: $R/lib/armeb/linux/crtend.o\n"
  "  library_paths:\n"
  "    - -L $R/lib/armeb/linux\n"
  "";

const char i386_ellcc_linux[] =
  "based_on: ellcc-linux\n"
  "compiler:\n"
  "  options:\n"
  "    - -target i386-ellcc-linux\n"
  "  c_include_dirs:\n"
  "    - $R/include/i386\n"
  "    - $R/include\n"
  "assembler:\n"
  "  exe: $E/i386-elf-as$X\n"
  "linker:\n"
  "  options:\n"
  "    - -m elf_i386\n"
  "    - --build-id\n"
  "    - --hash-style=gnu\n"
  "    - --eh-frame-hdr\n"
  "  static_crt1: $R/lib/i386/linux/crt1.o\n"
  "  dynamic_crt1: $R/lib/i386/linux/Scrt1.o\n"
  "  crtbegin: $R/lib/i386/linux/crtbegin.o\n"
  "  crtend: $R/lib/i386/linux/crtend.o\n"
  "  library_paths:\n"
  "    - -L $R/lib/i386/linux\n"
  "";

const char microblaze_ellcc_linux[] =
  "based_on: ellcc-linux\n"
  "compiler:\n"
  "  options:\n"
  "    - -target microblaze-ellcc-linux\n"
  "  c_include_dirs:\n"
  "    - $R/include/microblaze\n"
  "    - $R/include\n"
  "assembler:\n"
  "  exe: $E/microblaze-elf-as$X\n"
  "linker:\n"
  "  options:\n"
  "    - -m elf32mb_linux\n"
  "  static_crt1: $R/lib/microblaze/linux/crt1.o\n"
  "  dynamic_crt1: $R/lib/microblaze/linux/Scrt1.o\n"
  "  crtbegin: $R/lib/microblaze/linux/crtbegin.o\n"
  "  crtend: $R/lib/microblaze/linux/crtend.o\n"
  "  library_paths:\n"
  "    - -L $R/lib/microblaze/linux\n"
  "";

const char mips_ellcc_linux[] =
  "based_on: ellcc-linux\n"
  "compiler:\n"
  "  options:\n"
  "    - -target mips-ellcc-linux\n"
  "  c_include_dirs:\n"
  "    - $R/include/mips\n"
  "    - $R/include\n"
  "assembler:\n"
  "  exe: $E/mips-elf-as$X\n"
  "linker:\n"
  "  options:\n"
  "    - -m elf32ebmip\n"
  "    - --build-id\n"
  "    - --eh-frame-hdr\n"
  "  static_crt1: $R/lib/mips/linux/crt1.o\n"
  "  dynamic_crt1: $R/lib/mips/linux/Scrt1.o\n"
  "  crtbegin: $R/lib/mips/linux/crtbegin.o\n"
  "  crtend: $R/lib/mips/linux/crtend.o\n"
  "  library_paths:\n"
  "    - -L $R/lib/mips/linux\n"
  "";

const char mipsel_ellcc_linux[] =
  "based_on: ellcc-linux\n"
  "compiler:\n"
  "  options:\n"
  "    - -target mipsel-ellcc-linux\n"
  "  c_include_dirs:\n"
  "    - $R/include/mips\n"
  "    - $R/include\n"
  "assembler:\n"
  "  exe: $E/mips-elf-as$X\n"
  "  options:\n"
  "    - -EL\n"
  "linker:\n"
  "  options:\n"
  "    - -m elf32elmip\n"
  "    - --build-id\n"
  "    - --eh-frame-hdr\n"
  "  static_crt1: $R/lib/mipsel/linux/crt1.o\n"
  "  dynamic_crt1: $R/lib/mipsel/linux/Scrt1.o\n"
  "  crtbegin: $R/lib/mipsel/linux/crtbegin.o\n"
  "  crtend: $R/lib/mipsel/linux/crtend.o\n"
  "  library_paths:\n"
  "    - -L $R/lib/mipsel/linux\n"
  "";

const char ppc_ellcc_linux[] =
  "based_on: ellcc-linux\n"
  "compiler:\n"
  "  options:\n"
  "    - -target ppc-ellcc-linux\n"
  "  c_include_dirs:\n"
  "    - $R/include/ppc\n"
  "    - $R/include\n"
  "assembler:\n"
  "  exe: $E/ppc-elf-as$X\n"
  "  options:\n"
  "    - -a32\n"
  "    - -many\n"
  "linker:\n"
  "  options:\n"
  "    - -m elf32ppc\n"
  "    - --build-id\n"
  "    - --hash-style=gnu\n"
  "    - --eh-frame-hdr\n"
  "  static_crt1: $R/lib/ppc/linux/crt1.o\n"
  "  dynamic_crt1: $R/lib/ppc/linux/Scrt1.o\n"
  "  crtbegin: $R/lib/ppc/linux/crtbegin.o\n"
  "  crtend: $R/lib/ppc/linux/crtend.o\n"
  "  library_paths:\n"
  "    - -L $R/lib/ppc/linux\n"
  "";

const char ppc64_ellcc_linux[] =
  "based_on: ellcc-linux\n"
  "compiler:\n"
  "  options:\n"
  "    - -target ppc64-ellcc-linux\n"
  "  c_include_dirs:\n"
  "    - $R/include/ppc\n"
  "    - $R/include\n"
  "assembler:\n"
  "  exe: $E/ppc-elf-as$X\n"
  "  options:\n"
  "    - -a64\n"
  "    - -many\n"
  "linker:\n"
  "  options:\n"
  "    - -m elf64ppc\n"
  "    - --build-id\n"
  "    - --hash-style=gnu\n"
  "    - --eh-frame-hdr\n"
  "  static_crt1: $R/lib/ppc64/linux/crt1.o\n"
  "  dynamic_crt1: $R/lib/ppc64/linux/Scrt1.o\n"
  "  crtbegin: $R/lib/ppc64/linux/crtbegin.o\n"
  "  crtend: $R/lib/ppc64/linux/crtend.o\n"
  "  library_paths:\n"
  "    - -L $R/lib/ppc64/linux\n"
  "";

const char x86_64_ellcc_linux[] =
  "based_on: ellcc-linux\n"
  "compiler:\n"
  "  options:\n"
  "    - -target x86_64-ellcc-linux\n"
  "  c_include_dirs:\n"
  "    - $R/include/x86_64\n"
  "    - $R/include\n"
  "assembler:\n"
  "  exe: $E/x86_64-elf-as$X\n"
  "linker:\n"
  "  options:\n"
  "    - -m elf_x86_64\n"
  "    - --build-id\n"
  "    - --hash-style=gnu\n"
  "    - --eh-frame-hdr\n"
  "  static_crt1: $R/lib/x86_64/linux/crt1.o\n"
  "  dynamic_crt1: $R/lib/x86_64/linux/Scrt1.o\n"
  "  crtbegin: $R/lib/x86_64/linux/crtbegin.o\n"
  "  crtend: $R/lib/x86_64/linux/crtend.o\n"
  "  library_paths:\n"
  "    - -L $R/lib/x86_64/linux\n"
  "";

using namespace clang::compilationinfo;

void PredefinedInfo(CompilationInfo &Info)
{
    if (Info.InfoMap.size() == 0) {
      Info.DefineInfo("ellcc-linux", ellcc_linux);
      Info.DefineInfo("arm-ellcc-linux", arm_ellcc_linux);
      Info.DefineInfo("armeb-ellcc-linux", armeb_ellcc_linux);
      Info.DefineInfo("i386-ellcc-linux", i386_ellcc_linux);
      Info.DefineInfo("microblaze-ellcc-linux",
                                  microblaze_ellcc_linux);
      Info.DefineInfo("mips-ellcc-linux", mips_ellcc_linux);
      Info.DefineInfo("mipsel-ellcc-linux", mipsel_ellcc_linux);
      Info.DefineInfo("ppc-ellcc-linux", ppc_ellcc_linux);
      Info.DefineInfo("ppc64-ellcc-linux", ppc64_ellcc_linux);
      Info.DefineInfo("x86_64-ellcc-linux", x86_64_ellcc_linux);
  }
}
}

void clang::compilationinfo::ExpandArg(std::string &arg, const char *sub,
                                       const std::string &value)
{
  // Expand sub into value.
  size_t index = 0;
  while ((index = arg.find(sub, index)) != std::string::npos) {
    if (index > 0 && arg[index - 1] == '\\') {
      // An escaped sub start character. Remove the escape
      arg.erase(index - 1, 1);
      continue;
    }
    // Make the substitution.
    arg.replace(index, strlen(sub), value);
  }
}

void clang::compilationinfo::ExpandArgWithAll(std::string &arg,
                                         const clang::driver::Driver &TheDriver,
                                         const char *Output)
{
  if (arg.find_first_of('$') == std::string::npos) {
    // No expansions to do.
    return;
  }

  // Perform expansions.
  ExpandArg(arg, "$E", TheDriver.Dir);
  ExpandArg(arg, "$R", TheDriver.ResourceDir);
#if defined(LLVM_ON_WIN32)
  ExpandArg(arg, "$X", ".exe");
#else
  ExpandArg(arg, "$X", "");
#endif
  if (Output) {
    ExpandArg(arg, "$O", Output);
  }
}

/// ExpandArgs - Expand arguments.
/// Expand "-option value" into two entries.
/// Make argument substitutions.
void clang::compilationinfo::ExpandArgs(StrSequence &options,
                                        const clang::driver::Driver &TheDriver,
                                        const char *Output)
{
  // Expand "-option value" into two entries.
  StrSequence newOptions;
  for (StrSequence::iterator it = options.begin(),
       ie = options.end(); it != ie; ++it) {
    // Find the first option terminator.
    size_t space = it->find_first_of(" \t='\"");
    if (space != std::string::npos && isspace((*it)[space])) {
      // Have a terminated option. Ignore trailing spaces.
      size_t nonspace = it->find_first_not_of(" \t", space);
      if (nonspace !=  std::string::npos) {
        // Need to split the option into two arguments.
        newOptions.push_back(it->substr(0, space));
        newOptions.push_back(it->substr(nonspace));
      } else {
        newOptions.push_back(*it);
      }
    } else {
      newOptions.push_back(*it);
    }
  }

  options = newOptions;
  // Expand $X substitutions.
  for (auto &i : options) {
    ExpandArgWithAll(i, TheDriver, Output);
  }
}

bool CompilationInfo::ReadInfo(llvm::MemoryBuffer &Buffer)
{
  llvm::yaml::Input yin(Buffer.getMemBufferRef());
  yin >> *this;
  if (yin.error()) {
    TheDriver.Diag(diag::err_drv_malformed_compilation_info) <<
        Buffer.getBufferIdentifier();
    return false;
  }

  if (this->dump) {
    // Look at the info.
    llvm::yaml::Output yout(llvm::outs());
    yout << *this;
  }

  return true;
}

bool CompilationInfo::CheckForAndReadInfo(const char *target,
                                          driver::Driver &TheDriver)
{
  // Look for a file that contains info for this target.
  std::unique_ptr<CompilationInfo> Info(new CompilationInfo(TheDriver));
  llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>> BufferOrErr =
      llvm::MemoryBuffer::getFile(target);
  bool valid = false;
  if (!BufferOrErr.getError()) {
    // Get info from the file.
    valid = Info->ReadInfo(*BufferOrErr.get());
  } 

  if (!valid) {
    // Look in the config directory.
    llvm::SmallString<128> P(TheDriver.ResourceDir);
    llvm::sys::path::append(P, "config", target);
    BufferOrErr = llvm::MemoryBuffer::getFile(P.str());
    if (!BufferOrErr.getError()) {
      // Get info from the file.
      valid = Info->ReadInfo(*BufferOrErr.get());
    }
  }

  if (!valid) {
    // Can't open as a file. Look for predefined info.
    PredefinedInfo(*Info);
    std::map<std::string, const char *>::iterator it;
    it =  Info->InfoMap.find(target);
    if (it !=  Info->InfoMap.end()) {
      // Predefined info exists for this target.
      std::unique_ptr<llvm::MemoryBuffer> Buffer =
          llvm::MemoryBuffer::getMemBuffer(it->second, target);
      valid =  Info->ReadInfo(*Buffer.get());
    }
  }

  if (valid) {
    // Give the info to the driver.
    TheDriver.Info = std::move(Info);
  }

  return valid;
}

void CompilationInfo::HandleBasedOn(CompilationInfo &config)
{
  if (config.based_on.empty()) {
    return;
  }

  if (++config.nesting > 10) {
    // Nesting level exceeded.
    config.TheDriver.Diag(diag::err_drv_compilation_info_nesting)
      << config.nesting << config.based_on;
    --config.nesting;
    return;
  }

  if (config.dump) {
    llvm::outs() << "# based_on `" << config.based_on << "'\n";
  }

  // Look for a file that contains info for this target.
  llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>> BufferOrErr =
      llvm::MemoryBuffer::getFile(config.based_on);
  if (!BufferOrErr.getError()) {
    // Get info from the file.
    llvm::yaml::Input yin(BufferOrErr.get()->getMemBufferRef());
    std::string name = config.based_on;
    config.based_on.erase();
    yin >> config;
    if (yin.error()) {
      // Failed.
      config.TheDriver.Diag(diag::err_drv_malformed_compilation_info) << name;
    }
    --config.nesting;
    return;
  }

  // Look in the config directory.
  llvm::SmallString<128> P(config.TheDriver.ResourceDir);
  llvm::sys::path::append(P, "config", config.based_on);
  BufferOrErr = llvm::MemoryBuffer::getFile(P.str());
  if (!BufferOrErr.getError()) {
    // Get info from the file.
    llvm::yaml::Input yin(BufferOrErr.get()->getMemBufferRef());
    std::string name = config.based_on;
    config.based_on.erase();
    yin >> config;
    if (yin.error()) {
      // Failed.
      config.TheDriver.Diag(diag::err_drv_malformed_compilation_info) << name;
    }
    --config.nesting;
    return;
  }

  // Look for predefined info.
  PredefinedInfo(config);
  std::map<std::string, const char *>::iterator it;
  it = clang::compilationinfo::CompilationInfo::
      InfoMap.find(config.based_on.c_str());
  if (it != clang::compilationinfo::CompilationInfo::InfoMap.end()) {
    // Predefined info exists for this target.
    std::unique_ptr<llvm::MemoryBuffer> Buffer =
        llvm::MemoryBuffer::getMemBuffer(it->second, config.based_on);
    llvm::yaml::Input yin(Buffer->getMemBufferRef());
    std::string name = config.based_on;
    config.based_on.erase();
    yin >> config;
    if (yin.error()) {
      // Failed.
      config.TheDriver.Diag(diag::err_drv_malformed_compilation_info) << name;
    }
    --config.nesting;
    return;
  } else {
    // Can't find info. Failed.
    config.TheDriver.Diag(diag::err_drv_invalid_based_on) << config.based_on;
    --config.nesting;
    return;
  }
}

