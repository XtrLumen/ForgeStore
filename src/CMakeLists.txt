# CMakeLists.txt — ForgeMint hook library
cmake_minimum_required(VERSION 3.18)
project(forgemint C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(RAPLT_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/../RaPLT/src/raplt_core.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../RaPLT/src/raplt_elf.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../RaPLT/src/raplt_hash.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../RaPLT/src/raplt_signal.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../RaPLT/src/raplt_util.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../RaPLT/src/raplt_recon.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../RaPLT/src/raplt_dlopen.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../RaPLT/src/raplt_cfi.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../RaPLT/src/raplt_jni.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../RaPLT/src/raplt_mremap.c
)

# Suppress warnings for RaPLT sources (not our code)
foreach(src ${RAPLT_SRC})
    set_source_files_properties(${src} PROPERTIES COMPILE_FLAGS "-w")
endforeach()

add_library(forgemint SHARED
    main.c
    keybox.c
    hook.c
    ${RAPLT_SRC}
)

target_include_directories(forgemint PRIVATE
    .
    ${CMAKE_CURRENT_SOURCE_DIR}/../RaPLT/include
)

target_link_libraries(forgemint PUBLIC log)

target_compile_options(forgemint PRIVATE
    -Wall -Wextra -Werror
    -Wno-unused-parameter
    -Wno-gnu-zero-variadic-macro-arguments
)

set_target_properties(forgemint PROPERTIES
    C_VISIBILITY_PRESET hidden
)

add_executable(injector
    injector.c
)

target_include_directories(injector PRIVATE
    .
)

target_link_libraries(injector PUBLIC log dl)

target_compile_options(injector PRIVATE
    -Wall -Wextra
    -Wno-unused-parameter
)
