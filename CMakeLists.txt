cmake_minimum_required(VERSION 3.16)
project(jetbrains_unreal_editor)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if (MSVC)
    add_compile_options(/W4 /WX)
    # Do not specify /O2 because there are Debug/Release modes.
else()
    if (NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE Debug CACHE STRING "Default build type from CMakeLists.txt" FORCE)  # Add debug information
    endif (NOT CMAKE_BUILD_TYPE)

    add_compile_options(-Wall -Wextra -Werror -O2)
    if (UNIX AND NOT CMAKE_CXX_FLAGS)  # Do not add if -DCMAKE_CXX_FLAGS is passed
        add_compile_options(-fsanitize=address -fsanitize=undefined)
        add_link_options(-fsanitize=address -fsanitize=undefined)
    endif (UNIX AND NOT CMAKE_CXX_FLAGS)
endif (MSVC)


find_package(Curses REQUIRED)

add_executable(jetbrains_unreal_editor main.cpp)

target_include_directories(jetbrains_unreal_editor PUBLIC ${CURSES_INCLUDE_DIR})
target_link_libraries(jetbrains_unreal_editor ${CURSES_LIBRARIES})
