
include("${MACRO_DIR}/HifiLibrarySearchHints.cmake")
hifi_library_search_hints("glew")

find_path(GLFW_INCLUDE_DIRS GLFW/glfw3.h PATH_SUFFIXES include HINTS ${GLFW_SEARCH_DIRS})
find_library(GLFW_LIBRARY_RELEASE glfw3 PATH_SUFFIXES "lib/Release/Win32" "lib" HINTS ${GLFW_SEARCH_DIRS})

include(SelectLibraryConfigurations)
select_library_configurations(GLFW)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GLFW DEFAULT_MSG GLFW_INCLUDE_DIRS GLFW_LIBRARIES)