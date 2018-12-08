cmake_minimum_required(VERSION 3.13)
set(CMAKE_C_STANDARD 99)

project(vcs_tcpip C)

find_package(Doxygen)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror -Wextra -Wstrict-prototypes -pedantic -fno-common -O3 -g -std=gnu11")

include_directories(core/simple_message_server_logic)

add_custom_target(
        simple_message_server_logic
        COMMAND $(MAKE)
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/core/simple_message_server_logic
)

add_executable(simple_message_server simple_message_server.c)

add_dependencies(simple_message_server simple_message_server_logic)

if(DOXYGEN_FOUND)
    add_custom_target(doc
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            COMMAND ${DOXYGEN_EXECUTABLE})
endif()