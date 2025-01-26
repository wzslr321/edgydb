cmake_minimum_required(VERSION 3.30)
project(edgydb)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(edgydb main.cpp Database.cpp Database.hpp
        Serialization.hpp
        Deserialization.hpp
        Logger.hpp
        Utils.hpp
        Condition.hpp
)

include(FetchContent)

FetchContent_Declare(
        fmt
        GIT_REPOSITORY https://github.com/fmtlib/fmt
        GIT_TAG e69e5f977d458f2650bb346dadf2ad30c5320281)
FetchContent_MakeAvailable(fmt)

target_link_libraries(edgydb PRIVATE fmt::fmt)
