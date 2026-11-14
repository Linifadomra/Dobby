cmake_minimum_required(VERSION 3.13)
project(Dobby)
enable_language(ASM)
set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -x assembler-with-cpp")

# bare-metal / PowerPC options
option(DOBBY_BAREMETAL "Target bare-metal / console homebrew: static RWX memory pool, no OS services (auto-enabled when CMAKE_SYSTEM_NAME is Generic)" OFF)
option(DOBBY_TARGET_PPC "Force 32-bit big-endian PowerPC target, for toolchains that do not set CMAKE_SYSTEM_PROCESSOR" OFF)
option(DOBBY_PPC_NO_FPU "PowerPC: do not save FPRs in the register context (FPU disabled, MSR[FP] = 0)" OFF)
set(DOBBY_BAREMETAL_POOL_SIZE "65536" CACHE STRING "Bare-metal: size in bytes of the static pool trampolines and relocated code are carved from")

include(cmake/Util.cmake)
include(cmake/Macros.cmake)
include(cmake/build_environment_check.cmake)
include(cmake/auto_source_group.cmake)
include(cmake/xcode_generator_helper.cmake)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 17)

auto_source_group("." "auto-source-group" "\\.(cc|cpp|c|h)$")

# --- options

option(DOBBY_DEBUG "Enable debug logging" OFF)

option(NearBranch "Enable near branch trampoline" ON)

option(FullFloatingPointRegisterPack "Save and pack all floating-point registers" OFF)

option(Plugin.SymbolResolver "Enable symbol resolver" ON)

option(Plugin.ImportTableReplace "Enable import table replace " OFF)

option(Plugin.Android.BionicLinkerUtil "Enable android bionic linker util" OFF)

option(DOBBY_BUILD_EXAMPLE "Build example" OFF)

option(DOBBY_BUILD_TEST "Build test" OFF)

option(BUILD_SHARED_LIBS "Build as a shared library" OFF)

# --- private
option(DOBBY_BUILD_KERNEL_MODE "Build xnu kernel mode" OFF)

option(Private.Obfuscation "Enable llvm obfuscation" OFF)

if ((NOT DEFINED CMAKE_BUILD_TYPE) OR (CMAKE_BUILD_TYPE STREQUAL "Debug"))
  set(DOBBY_DEBUG ON)
endif ()


set(compile_definitions "")

# for arm64, allow access q8 - q31
if (FullFloatingPointRegisterPack)
  set(compile_definitions "${compile_definitions} -DFULL_FLOATING_POINT_REGISTER_PACK")
endif ()

if (DOBBY_BUILD_KERNEL_MODE)
  set(compile_definitions "${compile_definitions} -DBUILDING_KERNEL")
endif ()

if (DOBBY_DEBUG)
  set(compile_definitions "${compile_definitions} -DDOBBY_DEBUG")
else ()
  set(compile_definitions "${compile_definitions} -DDOBBY_LOGGING_DISABLE")
endif ()

set(compile_definitions "${compile_definitions} -DBUILD_WITH_TRAMPOLINE_ASM")

if (DOBBY_PPC_NO_FPU)
  set(compile_definitions "${compile_definitions} -DDOBBY_PPC_NO_FPU")
endif ()

if (CMAKE_GENERATOR STREQUAL Xcode)
endif ()

include(cmake/compiler_and_linker.cmake)

message(STATUS "[Dobby] CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
message(STATUS "[Dobby] DOBBY_DEBUG: ${DOBBY_DEBUG}")
message(STATUS "[Dobby] NearBranch: ${NearBranch}")
message(STATUS "[Dobby] FullFloatingPointRegisterPack: ${FullFloatingPointRegisterPack}")
message(STATUS "[Dobby] Plugin.SymbolResolver: ${Plugin.SymbolResolver}")
message(STATUS "[Dobby] Plugin.ImportTableReplace: ${Plugin.ImportTableReplace}")
message(STATUS "[Dobby] Plugin.Android.BionicLinkerUtil: ${Plugin.Android.BionicLinkerUtil}")
message(STATUS "[Dobby] DOBBY_BUILD_EXAMPLE: ${DOBBY_BUILD_EXAMPLE}")
message(STATUS "[Dobby] BUILD_SHARED_LIBS: ${BUILD_SHARED_LIBS}")
message(STATUS "[Dobby] DOBBY_BUILD_TEST: ${DOBBY_BUILD_TEST}")
message(STATUS "[Dobby] DOBBY_BUILD_KERNEL_MODE: ${DOBBY_BUILD_KERNEL_MODE}")
message(STATUS "[Dobby] Private.Obfuscation: ${Private.Obfuscation}")

# ---

include_directories(
  .
  ./include
  ./source
  source/dobby

  ./external
  ./external/logging

  ./builtin-plugin
)

if (SYSTEM.Darwin AND (NOT DOBBY_BUILD_KERNEL_MODE))
  include_directories(
    source/Backend/UserMode
  )
endif ()

# ---

set(DOBBY_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(dobby.SOURCE_FILE_LIST ${dobby.SOURCE_FILE_LIST}
  # cpu

  # assembler
  source/core/assembler/assembler-arm.cc
  source/core/assembler/assembler-ia32.cc
  source/core/assembler/assembler-x64.cc
  source/core/assembler/assembler-ppc.h

  # codegen
  source/core/codegen/codegen-arm.cc
  source/core/codegen/codegen-ia32.cc

  # memory kit

  # instruction relocation
  source/InstructionRelocation/arm/InstructionRelocationARM.cc
  source/InstructionRelocation/arm64/InstructionRelocationARM64.cc
  source/InstructionRelocation/x86/InstructionRelocationX86.cc
  source/InstructionRelocation/x86/InstructionRelocationX86Shared.cc
  source/InstructionRelocation/x64/InstructionRelocationX64.cc
  source/InstructionRelocation/ppc/InstructionRelocationPPC.cc
  source/InstructionRelocation/x86/x86_insn_decode/x86_insn_decode.c

  # intercept routing
  source/InterceptRouting/InterceptRouting.h
  source/InterceptRouting/InlineHookRouting.h
  source/InterceptRouting/InstrumentRouting.h
  source/InterceptRouting/RoutingPlugin.h
  source/InterceptRouting/InstrumentRouting/instrument_routing_handler.cpp
  source/InterceptRouting/NearBranchTrampoline/NearBranchTrampoline.h

  # intercept routing trampoline
  source/TrampolineBridge/Trampoline/trampoline_arm.cc
  source/TrampolineBridge/Trampoline/trampoline_arm64.cc
  source/TrampolineBridge/Trampoline/trampoline_x86.cc
  source/TrampolineBridge/Trampoline/trampoline_x64.cc
  source/TrampolineBridge/Trampoline/trampoline_ppc.cc

  # closure trampoline bridge - arm
  source/TrampolineBridge/ClosureTrampolineBridge/arm/helper_arm.cc
  source/TrampolineBridge/ClosureTrampolineBridge/arm/closure_bridge_arm.cc
  source/TrampolineBridge/ClosureTrampolineBridge/arm/ClosureTrampolineARM.cc
  # closure trampoline bridge - arm64
  source/TrampolineBridge/ClosureTrampolineBridge/arm64/helper_arm64.cc
  source/TrampolineBridge/ClosureTrampolineBridge/arm64/closure_bridge_arm64.cc
  source/TrampolineBridge/ClosureTrampolineBridge/arm64/ClosureTrampolineARM64.cc
  source/TrampolineBridge/ClosureTrampolineBridge/arm64/closure_bridge_arm64.S
  source/TrampolineBridge/ClosureTrampolineBridge/arm64/closure_trampoline_arm64.S
  # closure trampoline bridge - x86
  source/TrampolineBridge/ClosureTrampolineBridge/x86/helper_x86.cc
  source/TrampolineBridge/ClosureTrampolineBridge/x86/closure_bridge_x86.cc
  source/TrampolineBridge/ClosureTrampolineBridge/x86/ClosureTrampolineX86.cc
  # closure trampoline bridge - x64
  source/TrampolineBridge/ClosureTrampolineBridge/x64/helper_x64.cc
  source/TrampolineBridge/ClosureTrampolineBridge/x64/closure_bridge_x64.cc
  source/TrampolineBridge/ClosureTrampolineBridge/x64/ClosureTrampolineX64.cc
  source/TrampolineBridge/ClosureTrampolineBridge/x64/closure_bridge_x64.S
  source/TrampolineBridge/ClosureTrampolineBridge/x64/closure_trampoline_x64.S
  # closure trampoline bridge - ppc
  source/TrampolineBridge/ClosureTrampolineBridge/ppc/closure_bridge_ppc.cc
  source/TrampolineBridge/ClosureTrampolineBridge/ppc/ClosureTrampolinePPC.cc
  source/TrampolineBridge/ClosureTrampolineBridge/ppc/closure_bridge_ppc.S

  # plugin register

  # main
  source/dobby.cpp
  )


if (SYSTEM.BareMetal)
  set(dobby.SOURCE_FILE_LIST ${dobby.SOURCE_FILE_LIST}
    source/Backend/BareMetal/ProcessRuntime-baremetal.cc

    source/Backend/BareMetal/platform-baremetal.cc

    source/Backend/BareMetal/code-patch-tool-baremetal.cc
    source/Backend/UserMode/ExecMemory/clear-cache-tool-all.c
    )
  # no dlopen / /proc / import tables
  set(Plugin.SymbolResolver OFF)
  set(Plugin.ImportTableReplace OFF)
  set(compile_definitions "${compile_definitions} -DDOBBY_BAREMETAL -DDOBBY_BAREMETAL_POOL_SIZE=${DOBBY_BAREMETAL_POOL_SIZE}")
elseif (SYSTEM.Darwin AND NOT DOBBY_BUILD_KERNEL_MODE)
  set(dobby.SOURCE_FILE_LIST ${dobby.SOURCE_FILE_LIST}
    source/Backend/UserMode/PlatformUtil/Darwin/ProcessRuntime.cc

    source/Backend/UserMode/UnifiedInterface/platform-posix.cc

    source/Backend/UserMode/ExecMemory/code-patch-tool-darwin.cc
    source/Backend/UserMode/ExecMemory/clear-cache-tool-all.c
    )

elseif (SYSTEM.Linux OR SYSTEM.Android)
  set(dobby.SOURCE_FILE_LIST ${dobby.SOURCE_FILE_LIST}
    source/Backend/UserMode/PlatformUtil/Linux/ProcessRuntime.cc

    source/Backend/UserMode/UnifiedInterface/platform-posix.cc

    source/Backend/UserMode/ExecMemory/code-patch-tool-posix.cc
    source/Backend/UserMode/ExecMemory/clear-cache-tool-all.c
    )
elseif (SYSTEM.Windows)
  set(dobby.SOURCE_FILE_LIST ${dobby.SOURCE_FILE_LIST}
    source/Backend/UserMode/PlatformUtil/Windows/ProcessRuntime.cc

    source/Backend/UserMode/UnifiedInterface/platform-windows.cc

    source/Backend/UserMode/ExecMemory/code-patch-tool-windows.cc
    source/Backend/UserMode/ExecMemory/clear-cache-tool-all.c
    )
endif ()

if (PROCESSOR.X86_64 OR PROCESSOR.X86)
  set(NearBranch ON)
endif ()

# ---

if (0 AND SYSTEM.iOS AND (NOT DOBBY_BUILD_KERNEL_MODE))
  include_directories(
    source/Backend/UserMode/ExecMemory/substrated
  )
  set(compile_definitions "${compile_definitions} -DCODE_PATCH_WITH_SUBSTRATED")
  set(dobby.SOURCE_FILE_LIST ${dobby.SOURCE_FILE_LIST}
    source/Backend/UserMode/ExecMemory/substrated/mach_interface_support
    )
endif ()

# ----- instrument -----

if (FunctionWrapper)
  set(dobby.SOURCE_FILE_LIST ${dobby.SOURCE_FILE_LIST}
    # user mode - multi thread support
    # source/UserMode/MultiThreadSupport/ThreadSupport.cpp
    # source/UserMode/Thread/PlatformThread.cc
    # source/UserMode/Thread/platform-thread-${platform1}.cc
    )
  message(FATAL_ERROR "[!] FunctionWrapper plugin is not supported")
endif ()

# ---

if (NearBranch)
  set(dobby.SOURCE_FILE_LIST ${dobby.SOURCE_FILE_LIST}
    source/InterceptRouting/NearBranchTrampoline/near_trampoline_arm64.cc
    source/InterceptRouting/NearBranchTrampoline/near_trampoline_ppc.cc
    )
endif ()

# ---

# add logging library
add_subdirectory(external/logging)
get_target_property(logging.SOURCE_FILE_LIST logging SOURCES)

# add osbase library
if(NOT WIN32 AND NOT SYSTEM.BareMetal)
  add_subdirectory(external/osbase)
endif()
# ---

if (Plugin.SymbolResolver)
  include_directories(builtin-plugin/SymbolResolver)
  add_subdirectory(builtin-plugin/SymbolResolver)
  get_target_property(symbol_resolver.SOURCE_FILE_LIST dobby_symbol_resolver SOURCES)
  set(dobby.plugin.SOURCE_FILE_LIST ${dobby.plugin.SOURCE_FILE_LIST}
    ${symbol_resolver.SOURCE_FILE_LIST}
    )
endif ()

# ---

set(dobby.HEADER_FILE_LIST
  include/dobby.h
  )

# ---

# add build version
string(TIMESTAMP TODAY "%Y%m%d")
set(VERSION_REVISION "-${TODAY}")
if (EXISTS "${CMAKE_SOURCE_DIR}/.git")
  execute_process(
    COMMAND git rev-parse --short --verify HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE VERSION_COMMIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  if (VERSION_COMMIT_HASH)
    set(VERSION_REVISION "${VERSION_REVISION}-${VERSION_COMMIT_HASH}")
  endif ()
endif ()
set(DOBBY_BUILD_VERSION "Dobby${VERSION_REVISION}")
set(compile_definitions "${compile_definitions} -D__DOBBY_BUILD_VERSION__=\"${DOBBY_BUILD_VERSION}\"")
message(STATUS "[Dobby] ${DOBBY_BUILD_VERSION}")

# ---

set(SOURCE_FILE_LIST
  ${dobby.HEADER_FILE_LIST}
  ${dobby.SOURCE_FILE_LIST}
  ${logging.SOURCE_FILE_LIST}
  ${dobby.plugin.SOURCE_FILE_LIST}
  )

get_absolute_path_list(SOURCE_FILE_LIST SOURCE_FILE_LIST_)
set(SOURCE_FILE_LIST ${SOURCE_FILE_LIST_})

if (BUILD_SHARED_LIBS)
  add_library(dobby SHARED ${SOURCE_FILE_LIST})
else ()
  add_library(dobby STATIC ${SOURCE_FILE_LIST})
endif ()
add_library(dobby::dobby ALIAS dobby)

target_include_directories(dobby PUBLIC include)

# ---

if (Private.Obfuscation)
  set(linker_flags "${linker_flags} -Wl,-mllvm -Wl,-obfuscator-conf=all")
endif ()

# ---

set_target_properties(dobby PROPERTIES COMPILE_FLAGS "${compiler_flags}")
target_link_options(dobby PRIVATE $<$<BOOL:${BUILD_SHARED_LIBS}>:${linker_flags}>)

target_compile_definitions(dobby PRIVATE
  "COMPILE_DEFINITIONS ${compile_definitions}"
  $<IF:$<BOOL:${BUILD_SHARED_LIBS}>,-DBUILD_WITH_TRAMPOLINE_ASM,>
)

if (SYSTEM.BareMetal)
  target_compile_definitions(dobby PRIVATE
    DOBBY_BAREMETAL
    DOBBY_BAREMETAL_POOL_SIZE=${DOBBY_BAREMETAL_POOL_SIZE}
    BUILD_WITH_TRAMPOLINE_ASM
    $<$<BOOL:${DOBBY_PPC_NO_FPU}>:DOBBY_PPC_NO_FPU>
    $<$<BOOL:${FullFloatingPointRegisterPack}>:FULL_FLOATING_POINT_REGISTER_PACK>
    $<$<NOT:$<BOOL:${DOBBY_DEBUG}>>:DOBBY_LOGGING_DISABLE>
    $<$<BOOL:${DOBBY_DEBUG}>:DOBBY_DEBUG>
  )
endif ()

if (SYSTEM.Android)
  target_link_libraries(dobby log)
  if (PROCESSOR.ARM)
    set_target_properties(dobby
      PROPERTIES
      ANDROID_ARM_MODE arm
      )
  endif ()
endif ()

if (SYSTEM.Linux)
  target_link_libraries(dobby dl)
endif ()

# ---

if (DOBBY_BUILD_EXAMPLE AND (NOT DOBBY_BUILD_KERNEL_MODE))
  add_subdirectory(examples)
endif ()

if (DOBBY_BUILD_TEST AND (NOT DOBBY_BUILD_KERNEL_MODE))
  add_subdirectory(tests)
endif ()

# ---

if (SYSTEM.Darwin AND (NOT DOBBY_BUILD_KERNEL_MODE))
  include(cmake/platform/platform-darwin.cmake)
endif ()