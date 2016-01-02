set(TARGET_NAME render-utils)
setup_hifi_library(OpenGL Network Qml Quick)
link_hifi_libraries(shared gpu gl)

target_nsight()
target_oglplus()
