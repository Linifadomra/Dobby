#include "dobby/dobby_internal.h"

#include <string.h>

// no page protection so we write the bytes, then make the I-cache see them
PUBLIC int DobbyCodePatch(void *address, uint8_t *buffer, uint32_t buffer_size) {
  memcpy(address, buffer, buffer_size);
  ClearCache(address, (void *)((uintptr_t)address + buffer_size));
  return 0;
}
