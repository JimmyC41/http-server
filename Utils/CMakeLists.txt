add_library(UtilsModule
    ServerUtils.cpp
    HTTPUtils.cpp
)

target_include_directories(UtilsModule
    PRIVATE
        ${CMAKE_SOURCE_DIR}/HTTP
    PUBLIC
        ${CMAKE_SOURCE_DIR}/Utils
)

target_link_libraries(UtilsModule
    PRIVATE
        spdlog::spdlog
)