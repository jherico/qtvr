#
#  Created by Bradley Austin Davis on 2015/11/18
#  Copyright 2015 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http:#www.apache.org/licenses/LICENSE-2.0.html
#

if (WIN32)

    # we're using static GLEW, so define GLEW_STATIC
    add_definitions(-DGLEW_STATIC)

    set(TARGET_NAME plugOpenVR)
    setup_hifi_plugin()
    link_hifi_libraries(shared gl ui plugins)

    add_dependency_external_projects(OpenVR)
    find_package(OpenVR REQUIRED)
    target_include_directories(${TARGET_NAME} PRIVATE ${OPENVR_INCLUDE_DIRS})
    target_link_libraries(${TARGET_NAME} ${OPENVR_LIBRARIES})
endif()
