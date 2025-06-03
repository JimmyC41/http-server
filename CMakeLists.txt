cmake_minimum_required(VERSION 4.0.2)
set(CMAKE_CXX_STANDARD 23)

project(http-server)

find_package(spdlog REQUIRED)
find_package (TBB REQUIRED)

add_subdirectory(HTTP)
add_subdirectory(Utils)
add_subdirectory(Server)

add_executable(main
    main.cpp
)

target_include_directories(main
    PRIVATE
        ${CMAKE_SOURCE_DIR}/Server
        ${CMAKE_SOURCE_DIR}/Utils
        ${CMAKE_SOURCE_DIR}/HTTP
)

target_link_libraries(main
    PRIVATE
        ServerModule
        UtilsModule
        HTTPModule
        spdlog::spdlog
)