set(TARGET_NAME ${MAIN_APP_NAME})
project(${TARGET_NAME})

find_package(Qt5LinguistTools REQUIRED)
find_package(Qt5LinguistToolsMacros)

if (WIN32)
  add_definitions(-D_USE_MATH_DEFINES) # apparently needed to get M_PI and other defines from cmath/math.h		
  add_definitions(-DWINDOWS_LEAN_AND_MEAN) # needed to make sure windows doesn't go to crazy with its defines		
endif()

# grab the implementation and header files from src dirs
file(GLOB_RECURSE APP_SRCS "src/*.cpp" "src/*.h")
list(APPEND APP_SRCS "resources/ShadertoyVR.qrc")
GroupSources("src")

find_package(Qt5 COMPONENTS Gui Network OpenGL Qml Quick Svg WebEngine Xml)

if (APPLE)
  # configure CMake to use a custom Info.plist
  SET_TARGET_PROPERTIES( ${this_target} PROPERTIES MACOSX_BUNDLE_INFO_PLIST MacOSXBundleInfo.plist.in )

  set(MACOSX_BUNDLE_BUNDLE_NAME ${MAIN_APP_NAME})
  set(MACOSX_BUNDLE_GUI_IDENTIFIER io.highfidelity.Interface)
  
  if (UPPER_CMAKE_BUILD_TYPE MATCHES RELEASE OR UPPER_CMAKE_BUILD_TYPE MATCHES RELWITHDEBINFO)
    set(ICON_FILENAME "interface.icns")
  else ()
    set(ICON_FILENAME "interface-beta.icns")
  endif ()
  
  # set how the icon shows up in the Info.plist file
  SET(MACOSX_BUNDLE_ICON_FILE "${ICON_FILENAME}")

  # set where in the bundle to put the resources file
  SET_SOURCE_FILES_PROPERTIES(${CMAKE_CURRENT_SOURCE_DIR}/icon/${ICON_FILENAME} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
  
  set(DISCOVERED_RESOURCES "")

  # use the add_resources_to_os_x_bundle macro to recurse into resources
  add_resources_to_os_x_bundle("${CMAKE_CURRENT_SOURCE_DIR}/resources")

  # append the discovered resources to our list of sources
  list(APPEND APP_SRCS ${DISCOVERED_RESOURCES})
  
  set(APP_SRCS ${APP_SRCS} "${CMAKE_CURRENT_SOURCE_DIR}/icon/${ICON_FILENAME}")
endif()

# create the executable, make it a bundle on OS X
if (APPLE)
  add_executable(${TARGET_NAME} MACOSX_BUNDLE ${APP_SRCS} ${QM})
elseif(WIN32)
  add_executable(${TARGET_NAME} WIN32 ${APP_SRCS} ${QM})
else()
  add_executable(${TARGET_NAME} ${APP_SRCS} ${QM})
endif()

# These are external plugins, but we need to do the 'add dependency' here so that their 
# binary directories get added to the fixup path
add_dependency_external_projects(sixense)

add_dependency_external_projects(sdl2)

if (WIN32)
    add_dependency_external_projects(OpenVR)
endif()


# disable /OPT:REF and /OPT:ICF for the Debug builds
# This will prevent the following linker warnings
# LINK : warning LNK4075: ignoring '/INCREMENTAL' due to '/OPT:ICF' specification
if (WIN32)
  set_property(TARGET ${TARGET_NAME} APPEND_STRING PROPERTY LINK_FLAGS_DEBUG "/OPT:NOREF /OPT:NOICF")
endif()

# link required hifi libraries
link_hifi_libraries(shared gl ui gpu gpu-gl gpu-utils controllers plugins)

target_glew()
target_opengl()

message("${PROJECT_SOURCE_DIR}/src")


target_include_directories(${TARGET_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(${TARGET_NAME} Qt5::Gui Qt5::Network Qt5::OpenGL Qt5::Qml Qt5::Quick Qt5::Svg Qt5::WebEngine Qt5::Xml)

# Issue causes build failure unless we add this directory.  
# See https://bugreports.qt.io/browse/QTBUG-43351  
if (WIN32)
    add_paths_to_fixup_libs(${Qt5_DIR}/../../../plugins/qtwebengine)
endif()

# assume we are using a Qt build without bearer management
add_definitions(-DQT_NO_BEARERMANAGEMENT)

if (APPLE)

  # link in required OS X frameworks and include the right GL headers
  find_library(OpenGL OpenGL)
  find_library(AppKit AppKit)

  target_link_libraries(${TARGET_NAME} ${OpenGL} ${AppKit})

else ()

  # copy the resources files beside the executable
  add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
    "${PROJECT_SOURCE_DIR}/resources"
    $<TARGET_FILE_DIR:${TARGET_NAME}>/resources
  )

  # link target to external libraries
  if (WIN32)
    target_link_libraries(${TARGET_NAME} wsock32.lib Winmm.lib)
  else ()
    # Nothing else required on linux apparently
  endif()

endif (APPLE)

if (WIN32)
    set(EXTRA_DEPLOY_OPTIONS "--qmldir  ${PROJECT_SOURCE_DIR}/resources/qml")
endif()

package_libraries_for_deployment()
