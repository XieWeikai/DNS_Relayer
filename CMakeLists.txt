cmake_minimum_required(VERSION 3.16)
project(dns C)

set(CMAKE_C_STANDARD 11)

include_directories(include)

aux_source_directory(tool TOOL)
aux_source_directory(message MES)
aux_source_directory(cache CACHE)
aux_source_directory(pool POOL)

SET(CMAKE_BUILD_TYPE "Debug")
SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g2 -ggdb")
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")

add_executable(server
        server.c ${TOOL} ${MES} ${CACHE} ${POOL})

add_executable(cli
        cli.c ${TOOL} ${CACHE} ${MES})

target_link_libraries(server PUBLIC pthread)
target_link_libraries(cli PUBLIC pthread)



