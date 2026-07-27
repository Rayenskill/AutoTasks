cmake_minimum_required(VERSION 3.21)   # 3.21+ required by CMakePresets v3

project(AutoTasks
    VERSION     0.1.0
    DESCRIPTION "Desktop GUI automation: record, replay, review"
    LANGUAGES   CXX)                   # CXX only — don't enable C we never use

# =========================================================================
# MSVC hot reload / Edit and Continue  (kept from the VS template)
# Must come before targets are created.
# =========================================================================
if(POLICY CMP0141)
    cmake_policy(SET CMP0141 NEW)
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT
        "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

# =========================================================================
# Language standard  —  the "target" + "strict" of this project.
# =========================================================================
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)   # fail loudly instead of silently decaying
set(CMAKE_CXX_EXTENSIONS OFF)         # ISO C++20, not MS/GNU dialects

# Needed by clang-tidy, clangd, and most editor tooling.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Predictable binary location (matters on Windows for DLL lookup).
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# =========================================================================
# Options
# =========================================================================
option(AUTOTASKS_WARNINGS_AS_ERRORS "Treat compiler warnings as errors"      ON)
option(AUTOTASKS_ENABLE_CLANG_TIDY  "Run clang-tidy as part of the build"    OFF)
option(AUTOTASKS_ENABLE_SANITIZERS  "Enable AddressSanitizer (Debug builds)" OFF)
option(AUTOTASKS_BUILD_TESTS        "Build the test suite"                   ON)

# =========================================================================
# Warnings  —  where "strict mode" actually lives.
# An INTERFACE library: link it in and the target inherits the flags. Better
# than global flags, because third-party code shouldn't be judged by them.
# =========================================================================
add_library(autotasks_warnings INTERFACE)

if(MSVC)
    target_compile_options(autotasks_warnings INTERFACE
        /W4              # high warning level (/Wall is unusable on MSVC)
        /permissive-     # <- the real "strict": disables MS language extensions
        /w14242          # narrowing conversion, possible data loss
        /w14263          # member function hides a virtual (likely a typo)
        /w14265          # virtual functions but non-virtual destructor
        /w14555          # expression has no effect
        /w14640          # non-thread-safe static local initialisation
        /w14826          # sign-extending conversion, possible surprise
        /utf-8           # source AND execution charset are UTF-8
        /Zc:__cplusplus  # report the true __cplusplus (MSVC lies by default)
    )
    if(AUTOTASKS_WARNINGS_AS_ERRORS)
        target_compile_options(autotasks_warnings INTERFACE /WX)
    endif()
else()
    target_compile_options(autotasks_warnings INTERFACE
        -Wall -Wextra -Wpedantic
        -Wshadow              # a local shadowing an outer name — classic bug
        -Wnon-virtual-dtor
        -Wold-style-cast
        -Wcast-align
        -Wunused
        -Woverloaded-virtual
        -Wconversion
        -Wsign-conversion
        -Wnull-dereference
        -Wdouble-promotion
    )
    if(AUTOTASKS_WARNINGS_AS_ERRORS)
        target_compile_options(autotasks_warnings INTERFACE -Werror)
    endif()
endif()

# =========================================================================
# Sanitizers  —  runtime checks (use-after-free, overruns, leaks).
# MSVC supports ASan only, no UBSan. Costs ~2x speed; Debug builds.
# =========================================================================
add_library(autotasks_sanitizers INTERFACE)

if(AUTOTASKS_ENABLE_SANITIZERS)
    if(MSVC)
        target_compile_options(autotasks_sanitizers INTERFACE /fsanitize=address)
        message(STATUS "AutoTasks: AddressSanitizer enabled (MSVC)")
    else()
        target_compile_options(autotasks_sanitizers INTERFACE
            -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(autotasks_sanitizers INTERFACE
            -fsanitize=address,undefined)
        message(STATUS "AutoTasks: ASan + UBSan enabled")
    endif()
endif()

# =========================================================================
# clang-tidy  —  MUST be set before any target is created.
# See TOOLING.md if it complains about MSVC-style flags.
# =========================================================================
if(AUTOTASKS_ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY_EXE NAMES clang-tidy)
    if(CLANG_TIDY_EXE)
        set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE}")
        message(STATUS "AutoTasks: clang-tidy enabled -> ${CLANG_TIDY_EXE}")
    else()
        message(WARNING "AutoTasks: clang-tidy requested but not found on PATH")
    endif()
endif()

# =========================================================================
# Targets
# Phase 0-3 is just this one executable. Split engine/ and store/ into
# libraries with their own CMakeLists.txt at Phase 4.
# =========================================================================
add_executable(autotasks_engine
    engine/src/main.cpp
)

target_include_directories(autotasks_engine PRIVATE engine/include)

target_link_libraries(autotasks_engine PRIVATE
    autotasks_warnings
    autotasks_sanitizers
)

if(WIN32)
    # SendInput / SetWindowsHookEx live here (Phases 1-2).
    target_link_libraries(autotasks_engine PRIVATE user32)
endif()

# ---- Phase 3: OpenCV ----------------------------------------------------
# find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs)
# target_include_directories(autotasks_engine SYSTEM PRIVATE ${OpenCV_INCLUDE_DIRS})
# target_link_libraries(autotasks_engine PRIVATE ${OpenCV_LIBS})

# ---- Phase 4: split into libraries --------------------------------------
# add_subdirectory(engine)
# add_subdirectory(store)

# ---- Phase 5: Qt --------------------------------------------------------
# set(CMAKE_AUTOMOC ON)
# set(CMAKE_AUTOUIC ON)
# set(CMAKE_AUTORCC ON)
# find_package(Qt6 REQUIRED COMPONENTS Widgets Sql)
# qt_standard_project_setup()
# qt_add_executable(autotasks_app app/src/main.cpp)
# target_link_libraries(autotasks_app PRIVATE Qt6::Widgets Qt6::Sql autotasks_warnings)

# ---- Tests (GoogleTest, ~Phase 2-3) -------------------------------------
# if(AUTOTASKS_BUILD_TESTS)
#     enable_testing()
#     add_subdirectory(tests)
# endif()

# =========================================================================
# Convenience target:  cmake --build build/dev --target format
# =========================================================================
find_program(CLANG_FORMAT_EXE NAMES clang-format)
if(CLANG_FORMAT_EXE)
    file(GLOB_RECURSE AUTOTASKS_ALL_SOURCES
        ${CMAKE_SOURCE_DIR}/engine/*.cpp ${CMAKE_SOURCE_DIR}/engine/*.h
        ${CMAKE_SOURCE_DIR}/store/*.cpp  ${CMAKE_SOURCE_DIR}/store/*.h
        ${CMAKE_SOURCE_DIR}/app/*.cpp    ${CMAKE_SOURCE_DIR}/app/*.h
        ${CMAKE_SOURCE_DIR}/tests/*.cpp)
    add_custom_target(format
        COMMAND ${CLANG_FORMAT_EXE} -i --style=file ${AUTOTASKS_ALL_SOURCES}
        COMMENT "Formatting sources with clang-format"
        VERBATIM)
endif()

message(STATUS "AutoTasks ${PROJECT_VERSION} | ${CMAKE_BUILD_TYPE} | C++${CMAKE_CXX_STANDARD} | warnings-as-errors=${AUTOTASKS_WARNINGS_AS_ERRORS}")