cmake_minimum_required(VERSION 3.22.1)
project("codeeditor")

add_library(
    codeeditor-native
    SHARED
    native-lib.cpp
    git_engine.cpp
    extension_engine.cpp
)

find_library(log-lib log)

target_link_libraries(
    codeeditor-native
    ${log-lib}
)