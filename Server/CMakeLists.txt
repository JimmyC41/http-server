add_library(ServerModule
    ListenerSocket.cpp
    Server.cpp
)

target_include_directories(ServerModule
    PRIVATE
        ${CMAKE_SOURCE_DIR}/Utils
        ${CMAKE_SOURCE_DIR}/HTTP
        ${CMAKE_SOURCE_DIR}/libcds/cds
    PUBLIC
        ${CMAKE_SOURCE_DIR}/Server
)

target_link_libraries(ServerModule
    PRIVATE
        TBB::tbb
        spdlog::spdlog
        HTTPModule
)