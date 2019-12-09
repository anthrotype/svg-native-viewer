add_executable(testJson TestJson.cpp)

target_link_libraries(testJson PUBLIC SVGNativeViewerLib)
target_link_libraries(testJson PRIVATE nlohmann_json::nlohmann_json)
