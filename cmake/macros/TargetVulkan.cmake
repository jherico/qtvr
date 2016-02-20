# 
#  Created by Bradley Austin Davis on 2016/02/16
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 
macro(TARGET_VULKAN)
    add_dependency_external_projects(vkcpp)

    set(VULKAN_SDK "C:/VulkanSDK/1.0.3.1")
    list(APPEND INCLUDE_DIRS ${VULKAN_SDK}/Include)
    list(APPEND INCLUDE_DIRS ${VULKAN_SDK}/glslang)
    set(VULKAN_INCLUDE_DIRS ${INCLUDE_DIRS} CACHE TYPE INTERNAL)
    message("VK includes: " ${VULKAN_INCLUDE_DIRS})
    
    find_library(VULKAN_LOADER NAMES vulkan-1 vulkan HINTS "${VULKAN_SDK}/Bin" )
    target_link_libraries(${TARGET_NAME} ${VULKAN_LOADER})
    find_library(GLSLANG_LIB NAMES glslang HINTS "${VULKAN_SDK}/glslang/lib" )
    target_link_libraries(${TARGET_NAME} ${GLSLANG_LIB})
    find_library(OGLCOMPILER_LIB NAMES OGLCompiler HINTS "${VULKAN_SDK}/glslang/lib" )
    target_link_libraries(${TARGET_NAME} ${OGLCOMPILER_LIB})
    find_library(OSDEPENDENT_LIB NAMES OSDependent HINTS "${VULKAN_SDK}/glslang/lib" )
    target_link_libraries(${TARGET_NAME} ${OSDEPENDENT_LIB})
    find_library(SPIRV_LIB NAMES SPIRV HINTS "${VULKAN_SDK}/glslang/lib" )
    target_link_libraries(${TARGET_NAME} ${SPIRV_LIB})
    
    target_include_directories(${TARGET_NAME} PUBLIC ${VULKAN_INCLUDE_DIRS})
    target_include_directories(${TARGET_NAME} PUBLIC ${VKCPP_INCLUDE_DIRS})
endmacro()