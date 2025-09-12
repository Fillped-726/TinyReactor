# distributed-task-system/src/api-server/CMakeLists.txt
add_executable(api-server
    main.cpp
    api_server.cpp
    request_handler.cpp
    response_builder.cpp
)

target_include_directories(api-server PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(api-server PRIVATE
    common
    Boost::system
    Boost::filesystem
    Threads::Threads
)