#include "clang/Driver/CompilationInfo.h"
#include "clang/Driver/DriverDiagnostic.h"
#include "clang/Driver/Driver.h"

using namespace clang::compilationinfo;

// The target info database.
std::map<std::string, const char *> CompilationInfo::InfoMap;

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

bool CompilationInfo::ReadInfo(llvm::MemoryBuffer &Buffer,
                               driver::Driver &TheDriver)
{
  std::unique_ptr<CompilationInfo> Info(new CompilationInfo);
  llvm::yaml::Input yin(Buffer.getMemBufferRef());
  yin >> *Info;
  if (yin.error()) {
    TheDriver.Diag(diag::err_drv_malformed_compilation_info) <<
        Buffer.getBufferIdentifier();
    return false;
  }

#if RICH
  // Look at the info.
  llvm::yaml::Output yout(llvm::outs());
  yout << *Info;
#endif

  // Give the info to the driver.
  TheDriver.Info = std::move(Info);
  return true;
}

bool CompilationInfo::CheckForAndReadInfo(const char *target,
                                          driver::Driver &TheDriver)
{
  // Look for a file that contains info for this target.
  llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>> BufferOrErr =
      llvm::MemoryBuffer::getFile(target);
  if (!BufferOrErr.getError()) {
    // Get info from the file.
    return ReadInfo(*BufferOrErr.get(), TheDriver);
  }

  // Look in the config directory.
  llvm::SmallString<128> P(TheDriver.ResourceDir);
  llvm::sys::path::append(P, "config", target);
  BufferOrErr = llvm::MemoryBuffer::getFile(P.str());
  if (!BufferOrErr.getError()) {
    // Get info from the file.
    return ReadInfo(*BufferOrErr.get(), TheDriver);
  }

  // Can't open as a file. Look for predefined info.
  std::map<std::string, const char *>::iterator it;
  it = InfoMap.find(target);
  if (it != InfoMap.end()) {
    // Predefined info exists for this target.
    std::unique_ptr<llvm::MemoryBuffer> Buffer =
        llvm::MemoryBuffer::getMemBuffer(it->second, target);
    return ReadInfo(*Buffer.get(), TheDriver);
  }

  // No info exists. Leave as an argument to -target.
  return false;
}
