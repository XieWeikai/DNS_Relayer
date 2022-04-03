cmake_minimum_required(VERSION 3.17)
project(dns C)

set(CMAKE_C_STANDARD 11)

include_directories(include)

aux_source_directory(tool TOOL)
aux_source_directory(message MES)

add_executable(server
        server.c ${TOOL} ${MES})

add_executable(cli
        cli.c ${TOOL})
