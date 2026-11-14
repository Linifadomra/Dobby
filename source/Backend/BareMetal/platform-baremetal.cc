#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "logging/logging.h"
#include "logging/check_logging.h"
#include "PlatformUnifiedInterface/platform.h"

#ifndef DOBBY_BAREMETAL_POOL_SIZE
// trampolines, closure trampolines and relocated code all come out of this pool, so it is never freed
#define DOBBY_BAREMETAL_POOL_SIZE (64 * 1024)
#endif

#define DOBBY_BAREMETAL_PAGE_SIZE 4096

// kept in .bss so it does not bloat the executable image. on bare-metal, everything is executable
alignas(DOBBY_BAREMETAL_PAGE_SIZE) static uint8_t g_dobby_pool[DOBBY_BAREMETAL_POOL_SIZE]
    __attribute__((section(".bss.dobby_pool")));
static size_t g_dobby_pool_used = 0;

int OSMemory::PageSize() {
  return DOBBY_BAREMETAL_PAGE_SIZE;
}

void *OSMemory::Allocate(size_t size, MemoryPermission access) {
  return OSMemory::Allocate(size, access, nullptr);
}

void *OSMemory::Allocate(size_t size, MemoryPermission access, void *fixed_address) {
  size = (size + DOBBY_BAREMETAL_PAGE_SIZE - 1) & ~((size_t)DOBBY_BAREMETAL_PAGE_SIZE - 1);

  // can not map arbitrary addresses
  if (fixed_address != nullptr)
    return nullptr;

  if (g_dobby_pool_used + size > sizeof(g_dobby_pool)) {
    ERROR_LOG("dobby pool exhausted, raise DOBBY_BAREMETAL_POOL_SIZE");
    return nullptr;
  }

  void *result = g_dobby_pool + g_dobby_pool_used;
  g_dobby_pool_used += size;
  memset(result, 0, size);
  return result;
}

bool OSMemory::Free(void *address, size_t size) {
  return true;
}

bool OSMemory::Release(void *address, size_t size) {
  return true;
}

bool OSMemory::SetPermission(void *address, size_t size, MemoryPermission access) {
  return true;
}

void OSPrint::Print(const char *format, ...) {
  va_list args;
  va_start(args, format);
  VPrint(format, args);
  va_end(args);
}

void OSPrint::VPrint(const char *format, va_list args) {
  vprintf(format, args);
}
