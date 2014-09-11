#include "clang/Driver/CompilationInfo.h"
#include "clang/Driver/DriverDiagnostic.h"
#include "clang/Driver/Driver.h"

using namespace clang::compilationinfo;

void CompilationInfo::ExpandArg(std::string &arg, const char *sub,
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

void CompilationInfo::ExpandArgWithAll(std::string &arg,
                                       driver::Driver &TheDriver)
{
  ExpandArg(arg, "$E", TheDriver.Dir);
  ExpandArg(arg, "$R", TheDriver.ResourceDir);
}

    /// ExpandArgs - Expand arguments.
    /// Expand "-option value" into two entries.
    /// Make argument substitutions.
void CompilationInfo::ExpandArgs(StrSequence &options,
                                 driver::Driver &TheDriver)
{
  // Expand "-option value" into two entries.
  for (StrSequence::iterator it = options.begin(),
       ie = options.end(); it != ie; ++it) {
    // Find the first option terminator.
    size_t space = it->find_first_of(" \t='\"");
    if (space != std::string::npos && isspace((*it)[space])) {
      // Have a terminated option. Ignore trailing spaces.
      size_t nonspace = it->find_first_not_of(" \t", space);
      if (nonspace !=  std::string::npos) {
        // Need to split the option into two arguments.
        std::string s(it->substr(nonspace));
        *it = it->substr(0, space);
        ++it;
        options.insert(it, s);
      }
    }
  }

  // Expand $E and $R into the executable and resource path.
  for (auto &i : options) {
    ExpandArgWithAll(i, TheDriver);
  }
}

void CompilationInfo::ReadInfo(llvm::MemoryBuffer &Buffer,
                               driver::Driver &TheDriver)
{
  std::unique_ptr<CompilationInfo> Info(new CompilationInfo);
  llvm::yaml::Input yin(Buffer.getMemBufferRef());
  yin >> *Info;
  if (yin.error()) {
    TheDriver.Diag(diag::err_drv_malformed_compilation_info) <<
        Buffer.getBufferIdentifier();
    return;
  }
  ExpandArgs(Info->compiler.options, TheDriver);
  ExpandArgs(Info->assembler.options, TheDriver);
  ExpandArgWithAll(Info->assembler.exe, TheDriver);
  ExpandArgs(Info->linker.options, TheDriver);
  ExpandArgWithAll(Info->linker.exe, TheDriver);
  ExpandArgWithAll(Info->linker.static_crt1, TheDriver);
  ExpandArgWithAll(Info->linker.dynamic_crt1, TheDriver);
  ExpandArgWithAll(Info->linker.crtbegin, TheDriver);
  ExpandArgWithAll(Info->linker.crtend, TheDriver);
  ExpandArgWithAll(Info->linker.library_path, TheDriver);

#if RICH
  // Look at the info.
  llvm::yaml::Output yout(llvm::outs());
  yout << *Info;
#endif

  // Give the info to the driver.
  TheDriver.Info = std::move(Info);
}

bool CompilationInfo::CheckForAndReadInfo(const char *target,
                                          driver::Driver &TheDriver)
{
  std::map<const char *, const char *>::iterator it;
  it = InfoMap.find(target);
  if (it != InfoMap.end()) {
    // Predefined info exists for this target.
  } else {
    // Look for a file that contains info for this target.
    llvm::SmallString<128> P(TheDriver.ResourceDir);
    llvm::sys::path::append(P, "config", target);
    llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>> BufferOrErr =
        llvm::MemoryBuffer::getFile(P.str());
    if (BufferOrErr.getError()) {
      // Can't open as a file. Leave as an argument to -target.
      return false;
     }
    compilationinfo::CompilationInfo::ReadInfo(*BufferOrErr.get(), TheDriver);
  }
  return true;
}

