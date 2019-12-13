cmake_minimum_required(VERSION 3.1.0)

# Use newer policies if available, up to most recent tested version of CMake.
if(${CMAKE_VERSION} VERSION_LESS 3.11)
  cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
else()
  cmake_policy(VERSION 3.11)
endif()

# Determine if cfmt is built as a subproject (using add_subdirectory)
# or if it is the master project.
set(MASTER_PROJECT OFF)
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  set(MASTER_PROJECT ON)
  message(STATUS "CMake version: ${CMAKE_VERSION}")
endif ()

# Joins arguments and places the results in ${result_var}.
function(join result_var)
  set(result )
  foreach (arg ${ARGN})
    set(result "${result}${arg}")
  endforeach ()
  set(${result_var} "${result}" PARENT_SCOPE)
endfunction()

# Set the default CMAKE_BUILD_TYPE to Release.
# This should be done before the project command since the latter can set
# CMAKE_BUILD_TYPE itself (it does so for nmake).
if (MASTER_PROJECT AND NOT CMAKE_BUILD_TYPE)
  join(doc "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or "
           "CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel.")
  set(CMAKE_BUILD_TYPE Release CACHE STRING ${doc})
endif ()

option(CFMT_PEDANTIC "Enable extra warnings and expensive tests." OFF)
option(CFMT_WERROR "Halt the compilation with an error on compiler warnings."
       OFF)

# Options that control generation of various targets.
#option(CFMT_DOC "Generate the doc target." ${MASTER_PROJECT})
#option(CFMT_INSTALL "Generate the install target." ${MASTER_PROJECT})
option(CFMT_DOC "Generate the doc target." OFF)
option(CFMT_INSTALL "Generate the install target." OFF)
option(CFMT_TEST "Generate the test target." ${MASTER_PROJECT})
option(CFMT_FUZZ "Generate the fuzz target." OFF)
option(CFMT_CUDA_TEST "Generate the cuda-test target." OFF)

project(CFMT CXX)

# Get version from core.h
file(READ include/cfmt/complex_fmt.h core_h)
if (NOT core_h MATCHES "CFMT_VERSION ([0-9]+)([0-9][0-9])([0-9][0-9])")
  message(FATAL_ERROR "Cannot get CFMT_VERSION from complex_fmt.h.")
endif ()
# Use math to skip leading zeros if any.
math(EXPR CPACK_PACKAGE_VERSION_MAJOR ${CMAKE_MATCH_1})
math(EXPR CPACK_PACKAGE_VERSION_MINOR ${CMAKE_MATCH_2})
math(EXPR CPACK_PACKAGE_VERSION_PATCH ${CMAKE_MATCH_3})
join(CFMT_VERSION ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.
                 ${CPACK_PACKAGE_VERSION_PATCH})
message(STATUS "Version: ${CFMT_VERSION}")

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
  "${CMAKE_CURRENT_SOURCE_DIR}/support/cmake")

#include(cxx14)
include(CheckCXXCompilerFlag)

set(CFMT_REQUIRED_FEATURES cxx_auto_type cxx_variadic_templates)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  set(PEDANTIC_COMPILE_FLAGS -pedantic-errors -Wall -Wextra -pedantic
      -Wold-style-cast -Wundef
      -Wredundant-decls -Wwrite-strings -Wpointer-arith
      -Wcast-qual -Wformat=2 -Wmissing-include-dirs
      -Wcast-align -Wnon-virtual-dtor
      -Wctor-dtor-privacy -Wdisabled-optimization
      -Winvalid-pch -Woverloaded-virtual
      -Wconversion -Wswitch-enum
      -Wno-ctor-dtor-privacy -Wno-format-nonliteral -Wno-shadow)
  if (NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.6)
      set(PEDANTIC_COMPILE_FLAGS ${PEDANTIC_COMPILE_FLAGS} -Wnoexcept
         -Wno-dangling-else -Wno-unused-local-typedefs)
  endif ()
  if (NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 5.0)
      set(PEDANTIC_COMPILE_FLAGS ${PEDANTIC_COMPILE_FLAGS} -Wdouble-promotion
          -Wtrampolines -Wzero-as-null-pointer-constant -Wuseless-cast
          -Wvector-operation-performance -Wsized-deallocation)
  endif ()
  if (NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 6.0)
      set(PEDANTIC_COMPILE_FLAGS ${PEDANTIC_COMPILE_FLAGS} -Wshift-overflow=2
          -Wnull-dereference -Wduplicated-cond)
  endif ()
  set(WERROR_FLAG -Werror)
endif ()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(PEDANTIC_COMPILE_FLAGS -Wall -Wextra -pedantic -Wconversion
      -Wno-sign-conversion -Wdeprecated -Wweak-vtables)
  check_cxx_compiler_flag(-Wzero-as-null-pointer-constant HAS_NULLPTR_WARNING)
  if (HAS_NULLPTR_WARNING)
    set(PEDANTIC_COMPILE_FLAGS ${PEDANTIC_COMPILE_FLAGS}
        -Wzero-as-null-pointer-constant)
  endif ()
  set(WERROR_FLAG -Werror)
endif ()

if (MSVC)
  set(PEDANTIC_COMPILE_FLAGS /W3)
  set(WERROR_FLAG /WX)
endif ()

if (MASTER_PROJECT AND CMAKE_GENERATOR MATCHES "Visual Studio")
  # If Microsoft SDK is installed create script run-msbuild.bat that
  # calls SetEnv.cmd to set up build environment and runs msbuild.
  # It is useful when building Visual Studio projects with the SDK
  # toolchain rather than Visual Studio.
  include(FindSetEnv)
  if (WINSDK_SETENV)
    set(MSBUILD_SETUP "call \"${WINSDK_SETENV}\"")
  endif ()
  # Set FrameworkPathOverride to get rid of MSB3644 warnings.
  set(netfxpath "C:\\Program Files\\Reference Assemblies\\Microsoft\\Framework\\.NETFramework\\v4.0")
  file(WRITE run-msbuild.bat "
    ${MSBUILD_SETUP}
    ${CMAKE_MAKE_PROGRAM} -p:FrameworkPathOverride=\"${netfxpath}\" %*")
endif ()

set(strtod_l_headers stdlib.h)
if (APPLE)
  set(strtod_l_headers ${strtod_l_headers} xlocale.h)
endif ()

include(CheckSymbolExists)
if (WIN32)
  check_symbol_exists(_strtod_l "${strtod_l_headers}" HAVE_STRTOD_L)
else ()
  check_symbol_exists(strtod_l "${strtod_l_headers}" HAVE_STRTOD_L)
endif ()

function(add_headers VAR)
  set(headers ${${VAR}})
  foreach (header ${ARGN})
    set(headers ${headers} include/cfmt/${header})
  endforeach()
  set(${VAR} ${headers} PARENT_SCOPE)
endfunction()

# Define the fmt library, its includes and the needed defines.
add_headers(CFMT_HEADERS complex_fmt.h)
set(CFMT_SOURCES src/complex_fmt.cc)

add_library(cfmt ${CFMT_SOURCES} ${CFMT_HEADERS} README.rst ChangeLog.rst)
add_library(cfmt::cfmt ALIAS cfmt)

if (HAVE_STRTOD_L)
  target_compile_definitions(cfmt PUBLIC CFMT_LOCALE)
endif ()

if (CFMT_WERROR)
  target_compile_options(cfmt PRIVATE ${WERROR_FLAG})
endif ()
if (CFMT_PEDANTIC)
  target_compile_options(cfmt PRIVATE ${PEDANTIC_COMPILE_FLAGS})
endif ()

target_compile_features(cfmt INTERFACE ${CFMT_REQUIRED_FEATURES})

target_include_directories(cfmt PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)

set_target_properties(cfmt PROPERTIES
  VERSION ${CFMT_VERSION} SOVERSION ${CPACK_PACKAGE_VERSION_MAJOR}
  DEBUG_POSTFIX d)

if (BUILD_SHARED_LIBS)
  if (UNIX AND NOT APPLE AND NOT ${CMAKE_SYSTEM_NAME} MATCHES "SunOS")
    # Fix rpmlint warning:
    # unused-direct-shlib-dependency /usr/lib/libformat.so.1.1.0 /lib/libm.so.6.
    target_link_libraries(cfmt -Wl,--as-needed)
  endif ()
  target_compile_definitions(cfmt PRIVATE CFMT_EXPORT INTERFACE CFMT_SHARED)
endif ()
if (CFMT_SAFE_DURATION_CAST)
  target_compile_definitions(cfmt PUBLIC CFMT_SAFE_DURATION_CAST)
endif()

add_library(cfmt-header-only INTERFACE)
add_library(cfmt::cfmt-header-only ALIAS cfmt-header-only)

target_compile_definitions(cfmt-header-only INTERFACE CFMT_HEADER_ONLY=1)

target_compile_features(cfmt-header-only INTERFACE ${CFMT_REQUIRED_FEATURES})

target_include_directories(cfmt-header-only INTERFACE
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)

# Install targets.
if (CFMT_INSTALL)
  include(GNUInstallDirs)
  include(CMakePackageConfigHelpers)
  set(CFMT_CMAKE_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/cfmt CACHE STRING
      "Installation directory for cmake files, relative to ${CMAKE_INSTALL_PREFIX}.")
  set(version_config ${PROJECT_BINARY_DIR}/cfmt-config-version.cmake)
  set(project_config ${PROJECT_BINARY_DIR}/cfmt-config.cmake)
  set(pkgconfig ${PROJECT_BINARY_DIR}/cfmt.pc)
  set(targets_export_name cfmt-targets)

  set (INSTALL_TARGETS cfmt)
  if (TARGET cfmt-header-only)
    set(INSTALL_TARGETS ${INSTALL_TARGETS} cfmt-header-only)
  endif ()

  set(CFMT_LIB_DIR ${CMAKE_INSTALL_LIBDIR} CACHE STRING
      "Installation directory for libraries, relative to ${CMAKE_INSTALL_PREFIX}.")

  set(CFMT_INC_DIR ${CMAKE_INSTALL_INCLUDEDIR}/cfmt CACHE STRING
      "Installation directory for include files, relative to ${CMAKE_INSTALL_PREFIX}.")

  set(CFMT_PKGCONFIG_DIR ${CMAKE_INSTALL_LIBDIR}/pkgconfig CACHE PATH
      "Installation directory for pkgconfig (.pc) files, relative to ${CMAKE_INSTALL_PREFIX}.")

  # Generate the version, config and target files into the build directory.
  write_basic_package_version_file(
    ${version_config}
    VERSION ${CFMT_VERSION}
    COMPATIBILITY AnyNewerVersion)
  #configure_file(
  #  "${PROJECT_SOURCE_DIR}/support/cmake/cfmt.pc.in"
  #  "${pkgconfig}"
  #  @ONLY)
  #configure_package_config_file(
  #  ${PROJECT_SOURCE_DIR}/support/cmake/cfmt-config.cmake.in
  #  ${project_config}
  #  INSTALL_DESTINATION ${CFMT_CMAKE_DIR})
  # Use a namespace because CMake provides better diagnostics for namespaced
  # imported targets.
  #export(TARGETS ${INSTALL_TARGETS} NAMESPACE cfmt::
  #       FILE ${PROJECT_BINARY_DIR}/${targets_export_name}.cmake)

  # Install version, config and target files.
  install(
    FILES ${project_config} ${version_config}
    DESTINATION ${CFMT_CMAKE_DIR})
  install(EXPORT ${targets_export_name} DESTINATION ${CFMT_CMAKE_DIR}
          NAMESPACE cfmt::)

  # Install the library and headers.
  install(TARGETS ${INSTALL_TARGETS} EXPORT ${targets_export_name}
          LIBRARY DESTINATION ${CFMT_LIB_DIR}
          ARCHIVE DESTINATION ${CFMT_LIB_DIR}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

  install(FILES $<TARGET_PDB_FILE:${INSTALL_TARGETS}>
          DESTINATION ${CFMT_LIB_DIR} OPTIONAL)
  install(FILES ${CFMT_HEADERS} DESTINATION ${CFMT_INC_DIR})
  install(FILES "${pkgconfig}" DESTINATION "${CFMT_PKGCONFIG_DIR}")
endif ()

if (CFMT_DOC)
  add_subdirectory(doc)
endif ()

if (CFMT_TEST)
  enable_testing()
  add_subdirectory(test)
endif ()

# Control fuzzing independent of the unit tests.
if (CFMT_FUZZ)
  add_subdirectory(test/fuzzing)
endif ()

set(gitignore ${PROJECT_SOURCE_DIR}/.gitignore)
if (MASTER_PROJECT AND EXISTS ${gitignore})
  # Get the list of ignored files from .gitignore.
  file (STRINGS ${gitignore} lines)
  LIST(REMOVE_ITEM lines /doc/html)
  foreach (line ${lines})
    string(REPLACE "." "[.]" line "${line}")
    string(REPLACE "*" ".*" line "${line}")
    set(ignored_files ${ignored_files} "${line}$" "${line}/")
  endforeach ()
  set(ignored_files ${ignored_files}
    /.git /breathe /format-benchmark sphinx/ .buildinfo .doctrees)

  set(CPACK_SOURCE_GENERATOR ZIP)
  set(CPACK_SOURCE_IGNORE_FILES ${ignored_files})
  set(CPACK_SOURCE_PACKAGE_FILE_NAME cfmt-${CFMT_VERSION})
  set(CPACK_PACKAGE_NAME cfmt)
  set(CPACK_RESOURCE_FILE_README ${PROJECT_SOURCE_DIR}/README.rst)
  include(CPack)
endif ()

