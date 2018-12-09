cmake_minimum_required(VERSION 3.13)
set(CMAKE_C_STANDARD 99)

project(vcs_tcpip C)

find_package(Doxygen)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror -Wextra -Wstrict-prototypes -pedantic -fno-common -O3 -g -std=gnu11")

include_directories(core/libsimple_message_client_commandline_handling)
include_directories(core/simple_message_server_logic)

add_custom_target(
        libsimple_message_client_commandline_handling
        COMMAND $(MAKE)
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/core/libsimple_message_client_commandline_handling
)

add_executable(simple_message_client simple_message_client.c)
add_executable(simple_message_server simple_message_server.c)
add_executable(simple_message_server_logic core/simple_message_server_logic.c)

add_dependencies(simple_message_client libsimple_message_client_commandline_handling)
add_dependencies(simple_message_server simple_message_server_logic)

target_link_libraries(
        simple_message_client
        ${CMAKE_SOURCE_DIR}/core/libsimple_message_client_commandline_handling/libsimple_message_client_commandline_handling.a
)

if(DOXYGEN_FOUND)
    add_custom_target(doc
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            COMMAND ${DOXYGEN_EXECUTABLE})
endif()