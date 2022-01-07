#include <bela/env.hpp>
#include <bela/path.hpp>
#include <bela/io.hpp>
#include <baulk/fs.hpp>
#include <baulk/vs.hpp>
#include "compiler.hpp"
#include "baulk.hpp"

namespace baulk::compiler {

bool Executor::Initialize(bela::error_code &init_ec) {
  simulator.InitializeCleanupEnv();
  if (!baulk::env::InitializeVisualStudioEnv(simulator, baulk::env::HostArch, false, init_ec)) {
    return false;
  }
  initialized = true;
  return true;
}
} // namespace baulk::compiler