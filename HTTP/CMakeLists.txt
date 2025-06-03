add_library(HTTPModule
    Router.cpp
)

target_include_directories(HTTPModule
    PRIVATE
        ${CMAKE_SOURCE_DIR}/Server
        ${CMAKE_SOURCE_DIR}/Utils
    PUBLIC
        ${CMAKE_SOURCE_DIR}/HTTP
)

target_link_libraries(HTTPModule
    PRIVATE
        spdlog::spdlog
)