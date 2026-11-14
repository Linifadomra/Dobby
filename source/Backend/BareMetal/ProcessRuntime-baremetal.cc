#include "PlatformUtil/ProcessRuntime.h"

static stl::vector<MemRegion> regions;
const stl::vector<MemRegion> &ProcessRuntime::getMemoryLayout() {
  return regions;
}

static stl::vector<RuntimeModule> modules;
const stl::vector<RuntimeModule> &ProcessRuntime::getModuleMap() {
  return modules;
}

RuntimeModule ProcessRuntime::getModule(const char *name) {
  return RuntimeModule{0};
}
