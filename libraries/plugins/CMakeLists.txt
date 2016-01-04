set(TARGET_NAME plugins)
setup_hifi_library(OpenGL)
link_hifi_libraries(shared gl controllers)

GroupSources("src/plugins")

