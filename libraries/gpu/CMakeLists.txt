set(TARGET_NAME gpu)
setup_hifi_library()
link_hifi_libraries(shared networking gl)

target_glew()
target_opengl()