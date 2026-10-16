cmake_minimum_required(VERSION 3.22.1)
project("codeeditor")

# Set C++ standard to 23
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(
    codeeditor-native
    SHARED
    main.cpp
)

find_library(
    log-lib
    log
)

target_link_libraries(
    codeeditor-native
    ${log-lib}
)
