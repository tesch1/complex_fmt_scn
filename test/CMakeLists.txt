cmake_minimum_required(VERSION 3.14.0)
include(FetchContent)

if (NOT WIN32)
  set (DOWNLOAD_DIR "$ENV{HOME}/Downloads")
else (NOT WIN32)
  set (DOWNLOAD_DIR "C:/Downloads")
endif (NOT WIN32)

#------------------------------------------------------------------------------
add_executable(example example.cc)
target_link_libraries(example fmt cfmt)

#------------------------------------------------------------------------------
# Build the google test library

FetchContent_Declare(
  fmtX
  URL https://github.com/fmtlib/fmt/archive/6.1.1.tar.gz
  URL_HASH MD5=acfb83d44afdca171ee26c597c931e7c
  DOWNLOAD_DIR ${DOWNLOAD_DIR}
  )
FetchContent_MakeAvailable(fmtX)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_Declare(
  gtestX
  #GIT_REPOSITORY https://github.com/google/googletest.git
  #GIT_TAG master
  URL https://github.com/google/googletest/archive/release-1.10.0.tar.gz
  #URL_HASH MD5=acfb83d44afdca171ee26c597c931e7c
  DOWNLOAD_DIR ${DOWNLOAD_DIR}
  )
FetchContent_MakeAvailable(gtestX)

# --------------------------------------------------------------
set(TEST_MAIN_SRC test-main.cc )
add_library(test-main STATIC ${TEST_MAIN_SRC})
target_include_directories(test-main SYSTEM PUBLIC gtest gmock)
target_link_libraries(test-main gtest gmock fmt cfmt)


function(add_fmt_executable name)
  add_executable(${name} ${ARGN})
  if (MINGW)
    target_link_libraries(${name} -static-libgcc -static-libstdc++)
  endif ()
endfunction()

# Adds a test.
# Usage: add_fmt_test(name srcs...)
function(add_fmt_test name)
  add_fmt_executable(${name} ${name}.cc ${ARGN})
  target_link_libraries(${name} test-main)

  # Define if certain C++ features can be used.
  if (FMT_PEDANTIC)
    target_compile_options(${name} PRIVATE ${PEDANTIC_COMPILE_FLAGS})
  endif ()
  if (FMT_WERROR)
    target_compile_options(${name} PRIVATE ${WERROR_FLAG})
  endif ()
  target_include_directories(${name} SYSTEM PUBLIC gtest gmock)
  add_test(NAME ${name} COMMAND ${name})
endfunction()

add_fmt_test(format-test)

if (0)
add_fmt_test(assert-test)
add_fmt_test(chrono-test)
add_fmt_test(color-test)
add_fmt_test(core-test)
add_fmt_test(grisu-test)
target_compile_definitions(grisu-test PRIVATE FMT_USE_GRISU=1)
add_fmt_test(gtest-extra-test)
add_fmt_test(format-test mock-allocator.h)
if (MSVC)
  target_compile_options(format-test PRIVATE /bigobj)
endif ()
if (NOT (MSVC AND BUILD_SHARED_LIBS))
  add_fmt_test(format-impl-test)
endif ()
add_fmt_test(locale-test)
add_fmt_test(ostream-test)
add_fmt_test(compile-test)
add_fmt_test(printf-test)
add_fmt_test(custom-formatter-test)
add_fmt_test(ranges-test)
add_fmt_test(scan-test)

if (NOT MSVC_BUILD_STATIC)
  add_fmt_executable(posix-mock-test
    posix-mock-test.cc ../src/format.cc ${TEST_MAIN_SRC})
  target_include_directories(
    posix-mock-test PRIVATE ${PROJECT_SOURCE_DIR}/include)
  target_link_libraries(posix-mock-test gmock)
  target_include_directories(posix-mock-test SYSTEM PUBLIC gtest gmock)
  if (FMT_PEDANTIC)
    target_compile_options(posix-mock-test PRIVATE ${PEDANTIC_COMPILE_FLAGS})
  endif ()
  if (HAVE_STRTOD_L)
    target_compile_definitions(posix-mock-test PRIVATE FMT_LOCALE)
  endif ()
  add_test(NAME posix-mock-test COMMAND posix-mock-test)
  add_fmt_test(posix-test)
endif ()

add_fmt_executable(header-only-test
  header-only-test.cc header-only-test2.cc test-main.cc)
target_link_libraries(header-only-test gmock)
target_include_directories(header-only-test SYSTEM PUBLIC gtest gmock)
if (TARGET fmt-header-only)
  target_link_libraries(header-only-test fmt-header-only)
else ()
  target_include_directories(
    header-only-test PRIVATE ${PROJECT_SOURCE_DIR}/include)
  target_compile_definitions(header-only-test PRIVATE FMT_HEADER_ONLY=1)
endif ()

message(STATUS "FMT_PEDANTIC: ${FMT_PEDANTIC}")

if (FMT_PEDANTIC)
  # MSVC fails to compile GMock when C++17 is enabled.
  if (FMT_HAS_VARIANT AND NOT MSVC)
    add_fmt_test(std-format-test)
    set_property(TARGET std-format-test PROPERTY CXX_STANDARD 17)
  endif ()

  # Test that the library can be compiled with exceptions disabled.
  # -fno-exception is broken in icc: https://github.com/fmtlib/fmt/issues/822.
  if (NOT CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
    check_cxx_compiler_flag(-fno-exceptions HAVE_FNO_EXCEPTIONS_FLAG)
  endif ()
  if (HAVE_FNO_EXCEPTIONS_FLAG)
    add_library(noexception-test ../src/format.cc)
    target_include_directories(
      noexception-test PRIVATE ${PROJECT_SOURCE_DIR}/include)
    target_compile_options(noexception-test PRIVATE -fno-exceptions)
    if (FMT_PEDANTIC)
      target_compile_options(noexception-test PRIVATE ${PEDANTIC_COMPILE_FLAGS})
    endif ()
  endif ()

  # Test that the library compiles without locale.
  add_library(nolocale-test ../src/format.cc)
  target_include_directories(
    nolocale-test PRIVATE ${PROJECT_SOURCE_DIR}/include)
  target_compile_definitions(
    nolocale-test PRIVATE FMT_STATIC_THOUSANDS_SEPARATOR=1)

  # Test that the library compiles without windows.h.
  if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    add_library(no-windows-h-test ../src/format.cc)
    target_include_directories(
      no-windows-h-test PRIVATE ${PROJECT_SOURCE_DIR}/include)
    target_compile_definitions(no-windows-h-test PRIVATE FMT_USE_WINDOWS_H=0)
    if (FMT_PEDANTIC)
      target_compile_options(no-windows-h-test PRIVATE ${PEDANTIC_COMPILE_FLAGS})
    endif ()
    target_include_directories(no-windows-h-test SYSTEM PUBLIC gtest gmock)
  endif ()

  add_test(compile-error-test ${CMAKE_CTEST_COMMAND}
    --build-and-test
    "${CMAKE_CURRENT_SOURCE_DIR}/compile-error-test"
    "${CMAKE_CURRENT_BINARY_DIR}/compile-error-test"
    --build-generator ${CMAKE_GENERATOR}
    --build-makeprogram ${CMAKE_MAKE_PROGRAM}
    --build-options
    "-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
    "-DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}"
    "-DCXX_STANDARD_FLAG=${CXX_STANDARD_FLAG}"
    "-DPEDANTIC_COMPILE_FLAGS=${PEDANTIC_COMPILE_FLAGS}"
    "-DSUPPORTS_USER_DEFINED_LITERALS=${SUPPORTS_USER_DEFINED_LITERALS}")
endif ()

# These tests are disabled on Windows because they take too long.
if (FMT_PEDANTIC AND NOT WIN32)
  # Test if the targets are found from the build directory.
  add_test(find-package-test ${CMAKE_CTEST_COMMAND}
    -C ${CMAKE_BUILD_TYPE}
    --build-and-test
    "${CMAKE_CURRENT_SOURCE_DIR}/find-package-test"
    "${CMAKE_CURRENT_BINARY_DIR}/find-package-test"
    --build-generator ${CMAKE_GENERATOR}
    --build-makeprogram ${CMAKE_MAKE_PROGRAM}
    --build-options
    "-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
    "-DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}"
    "-DFMT_DIR=${PROJECT_BINARY_DIR}"
    "-DPEDANTIC_COMPILE_FLAGS=${PEDANTIC_COMPILE_FLAGS}"
    "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")

  # Test if the targets are found when add_subdirectory is used.
  add_test(add-subdirectory-test ${CMAKE_CTEST_COMMAND}
    -C ${CMAKE_BUILD_TYPE}
    --build-and-test
    "${CMAKE_CURRENT_SOURCE_DIR}/add-subdirectory-test"
    "${CMAKE_CURRENT_BINARY_DIR}/add-subdirectory-test"
    --build-generator ${CMAKE_GENERATOR}
    --build-makeprogram ${CMAKE_MAKE_PROGRAM}
    --build-options
    "-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
    "-DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}"
    "-DPEDANTIC_COMPILE_FLAGS=${PEDANTIC_COMPILE_FLAGS}"
    "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")
endif ()

# Activate optional CUDA tests if CUDA is found. For version selection see
# https://docs.nvidia.com/cuda/cuda-c-programming-guide/index.html#cpp14-language-features
if (FMT_CUDA_TEST)
  if (${CMAKE_VERSION} VERSION_LESS 3.15)
    find_package(CUDA 9.0)
  else ()
    include(CheckLanguage)
    check_language(CUDA)
    if (CMAKE_CUDA_COMPILER)
      enable_language(CUDA OPTIONAL)
      set(CUDA_FOUND TRUE)
    endif ()
  endif ()

  if (CUDA_FOUND)
    add_subdirectory(cuda-test)
    add_test(NAME cuda-test COMMAND fmt-in-cuda-test)
  endif ()
endif ()


endif(0)
