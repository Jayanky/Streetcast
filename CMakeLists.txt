cmake_minimum_required(VERSION 3.16)
project(Streetcast
    LANGUAGES C
    VERSION 0.0.1
    DESCRIPTION "Dreamcast emulator."
)

# For language servers like clangd.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(SC_SCIR_BLOCK_MAX_OPS 128 CACHE STRING "Max allowed Scir Ops. Decrease to reduce memory usage.")

set(SC_SCIR_BLOCK_MAX_USES 256 CACHE STRING "Max allowed Scir Uses. Decrease to reduce memory usage.")

set(SC_SCIR_BLOCK_MAX_CONSTS 128 CACHE STRING "Max allowed Scir Consts. Decrease to reduce memory usage.")

set(SC_PLATFORM "NATIVE" CACHE STRING "Platform to build for.")

option(SC_RELEASE "Enable release build" OFF)

if(SC_PLATFORM STREQUAL "PSP")
    if(NOT DEFINED ENV{PSPDEV})
        message(FATAL ERROR "$PSPDEV environment variable not set!")
    endif()
endif()

add_executable(${CMAKE_PROJECT_NAME})

target_precompile_headers(${CMAKE_PROJECT_NAME}
    PRIVATE
    include/allocate.h
    include/common.h
    include/debug.h
    include/defines.h
    include/scir.h
    include/vm.h
    include/emit/mips.h
)

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
    src/main.c
    src/scir/block.c
    src/allocate/allocate.c
    src/vm/compile.c
    src/call_convention/mips.c
    src/compile/mips.c
)

target_compile_options(${CMAKE_PROJECT_NAME}
    PRIVATE -std=c11 -Wextra -Werror
)

target_include_directories(${CMAKE_PROJECT_NAME}
    PRIVATE include
)

target_compile_definitions(${CMAKE_PROJECT_NAME}
    PRIVATE
    SC_SCIR_BLOCK_MAX_OPS_INT_=${SC_SCIR_BLOCK_MAX_OPS}
    SC_SCIR_BLOCK_MAX_USES_INT_=${SC_SCIR_BLOCK_MAX_USES}
    SC_SCIR_BLOCK_MAX_CONSTS_INT_=${SC_SCIR_BLOCK_MAX_CONSTS}
)

if(SC_RELEASE)
    target_compile_definitions(${CMAKE_PROJECT_NAME}
        PRIVATE
        SC_DEBUG_BOOL_OPTION_
    )

    target_compile_options(${CMAKE_PROJECT_NAME}
        PRIVATE -O2
    )
else()
    target_compile_options(${CMAKE_PROJECT_NAME}
        PRIVATE -O0 -g
    )
endif()

if(SC_PLATFORM STREQUAL "PSP")
    target_include_directories(${CMAKE_PROJECT_NAME}
        PRIVATE
        $ENV{PSPDEV}/psp/sdk/include
    )

    target_compile_definitions(
        ${CMAKE_PROJECT_NAME}
        PUBLIC
        SC_PLATFORM_PSP_OPTION_
        SC_ARCH_MIPS_ALLEGREX_OPTION_
    )

    target_link_libraries(
        ${CMAKE_PROJECT_NAME}
        PRIVATE
        pspdebug
        pspdisplay
        pspge
    )

    create_pbp_file(
        TARGET ${CMAKE_PROJECT_NAME}
        ICON_PATH NULL
        BACKGROUND_PATH NULL
        PREVIEW_PATH NULL
        TITLE ${CMAKE_PROJECT_NAME}
        VERSION 00.01
    )
endif()