cmake_minimum_required(VERSION 3.2)

if (WIN32)
  cmake_policy(SET CMP0020 NEW)
endif (WIN32)

if (POLICY CMP0028)
  cmake_policy(SET CMP0028 OLD)
endif ()

if (POLICY CMP0043)
  cmake_policy(SET CMP0043 OLD)
endif ()

if (POLICY CMP0042)
  cmake_policy(SET CMP0042 OLD)
endif ()

set(MAIN_APP_NAME exampleApp)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CMakeTargets")

project(core)
add_definitions(-DGLM_FORCE_RADIANS)
set(CMAKE_CXX_FLAGS_DEBUG  "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")

if (WIN32)
  add_definitions(-DNOMINMAX -D_CRT_SECURE_NO_WARNINGS)

  if (NOT WINDOW_SDK_PATH)
    set(DEBUG_DISCOVERED_SDK_PATH TRUE)
  endif()

  # sets path for Microsoft SDKs
  # if you get build error about missing 'glu32' this path is likely wrong
  if (MSVC10)
    set(WINDOW_SDK_PATH "C:\\Program Files\\Microsoft SDKs\\Windows\\v7.1 " CACHE PATH "Windows SDK PATH")
  elseif (MSVC12)
    if ("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
      set(WINDOW_SDK_FOLDER "x64")
    else()
      set(WINDOW_SDK_FOLDER "x86")
    endif()
    set(WINDOW_SDK_PATH "C:\\Program Files (x86)\\Windows Kits\\8.1\\Lib\\winv6.3\\um\\${WINDOW_SDK_FOLDER}" CACHE PATH "Windows SDK PATH")
  endif ()

  if (DEBUG_DISCOVERED_SDK_PATH)
    message(STATUS "The discovered Windows SDK path is ${WINDOW_SDK_PATH}")
  endif ()

  set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${WINDOW_SDK_PATH})
  # /wd4351 disables warning C4351: new behavior: elements of array will be default initialized
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP /wd4351")
  # /LARGEADDRESSAWARE enables 32-bit apps to use more than 2GB of memory.
  # Caveats: http://stackoverflow.com/questions/2288728/drawbacks-of-using-largeaddressaware-for-32-bit-windows-executables
  # TODO: Remove when building 64-bit.
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /LARGEADDRESSAWARE")
  # always produce symbols as PDB files
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zi")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /DEBUG /OPT:REF /OPT:ICF")
else ()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -fno-strict-aliasing -Wno-unused-parameter")
  if (CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ggdb -Woverloaded-virtual -Wdouble-promotion")
  endif ()
endif(WIN32)

if ((NOT MSVC12) AND (NOT MSVC14))
  include(CheckCXXCompilerFlag)
  CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
  CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)

  if (COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
  elseif(COMPILER_SUPPORTS_CXX0X)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
  else()
    message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
  endif()
endif ()

if (APPLE)
  set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++0x")
  set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --stdlib=libc++")
endif ()

if (NOT QT_CMAKE_PREFIX_PATH)
  set(QT_CMAKE_PREFIX_PATH $ENV{QT_CMAKE_PREFIX_PATH})
endif ()
if (NOT QT_CMAKE_PREFIX_PATH)
  get_filename_component(QT_CMAKE_PREFIX_PATH "${Qt5_DIR}/.." REALPATH)
endif ()

set(QT_DIR $ENV{QT_DIR})

if (WIN32)
  if (NOT EXISTS ${QT_CMAKE_PREFIX_PATH})
      message(FATAL_ERROR "Could not determine QT_CMAKE_PREFIX_PATH.")
  endif ()
endif()

get_filename_component(QT_DIR "${QT_CMAKE_PREFIX_PATH}/../../" ABSOLUTE)

set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${QT_CMAKE_PREFIX_PATH})

if (APPLE)
  exec_program(sw_vers ARGS -productVersion  OUTPUT_VARIABLE OSX_VERSION)
  string(REGEX MATCH "^[0-9]+\\.[0-9]+" OSX_VERSION ${OSX_VERSION})
  message(STATUS "Detected OS X version = ${OSX_VERSION}")

  set(OSX_SDK "${OSX_VERSION}" CACHE String "OS X SDK version to look for inside Xcode bundle or at OSX_SDK_PATH")

  # set our OS X deployment target
  set(CMAKE_OSX_DEPLOYMENT_TARGET 10.8)

  # find the SDK path for the desired SDK
  find_path(
    _OSX_DESIRED_SDK_PATH
    NAME MacOSX${OSX_SDK}.sdk
    HINTS ${OSX_SDK_PATH}
    PATHS /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/
          /Applications/Xcode-beta.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/
  )

  if (NOT _OSX_DESIRED_SDK_PATH)
    message(STATUS "Could not find OS X ${OSX_SDK} SDK. Will fall back to default. If you want a specific SDK, please pass OSX_SDK and optionally OSX_SDK_PATH to CMake.")
  else ()
    message(STATUS "Found OS X ${OSX_SDK} SDK at ${_OSX_DESIRED_SDK_PATH}/MacOSX${OSX_SDK}.sdk")

    # set that as the SDK to use
    set(CMAKE_OSX_SYSROOT ${_OSX_DESIRED_SDK_PATH}/MacOSX${OSX_SDK}.sdk)
  endif ()

endif ()

# Hide automoc folders (for IDEs)
set(AUTOGEN_TARGETS_FOLDER "hidden/generated")

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)
# Instruct CMake to run rcc automatically when needed
set(CMAKE_AUTORCC ON)

set(HIFI_LIBRARY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libraries")

# setup for find modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/")

if (CMAKE_BUILD_TYPE)
  string(TOUPPER ${CMAKE_BUILD_TYPE} UPPER_CMAKE_BUILD_TYPE)
else ()
  set(UPPER_CMAKE_BUILD_TYPE DEBUG)
endif ()

set(HF_CMAKE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set(MACRO_DIR "${HF_CMAKE_DIR}/macros")
set(EXTERNAL_PROJECT_DIR "${HF_CMAKE_DIR}/externals")

file(GLOB HIFI_CUSTOM_MACROS "cmake/macros/*.cmake")
foreach(CUSTOM_MACRO ${HIFI_CUSTOM_MACROS})
  include(${CUSTOM_MACRO})
endforeach()

set(EXTERNAL_PROJECT_PREFIX "project")
set_property(DIRECTORY PROPERTY EP_PREFIX ${EXTERNAL_PROJECT_PREFIX})
setup_externals_binary_dir()

option(USE_NSIGHT "Attempt to find the nSight libraries" 1)

if (WIN32)
  add_paths_to_fixup_libs("${QT_DIR}/bin")
endif ()

# add subdirectories for all top level targets 
# (libraries get pulled in automatically via dependencies)
add_subdirectory(app)
set_target_properties(${MAIN_APP_NAME} PROPERTIES FOLDER "Apps")
add_subdirectory(plugins)

