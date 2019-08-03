cmake_minimum_required(VERSION 3.0)
project(Ionia VERSION "0.3.1")

# set CMake module path
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# # set compiler path
# set(CMAKE_C_COMPILER "/usr/local/opt/gcc/bin/gcc-8")
# set(CMAKE_CXX_COMPILER "/usr/local/opt/gcc/bin/g++-8")

# C++17 standard support
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# enable all warnings and treat them as errors
if(MSVC)
  add_compile_options(/W3 /WX)
else()
  add_compile_options(-Wall -Werror)
endif()

# some definitions
add_compile_definitions(APP_NAME="${PROJECT_NAME}")
add_compile_definitions(APP_VERSION="${PROJECT_VERSION}")
add_compile_definitions(APP_VERSION_MAJOR=${PROJECT_VERSION_MAJOR})
add_compile_definitions(APP_VERSION_MINOR=${PROJECT_VERSION_MINOR})
add_compile_definitions(APP_VERSION_PATCH=${PROJECT_VERSION_PATCH})

# find package 'readline'
find_package(Readline)

# project include directories
include_directories(src)
include_directories(${Readline_INCLUDE_DIR})

# all of C++ source files
file(GLOB_RECURSE SOURCES "src/*.cpp")

# source files required by the library
set(LIB_SRC ${SOURCES})
list(REMOVE_ITEM LIB_SRC "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")
list(REMOVE_ITEM LIB_SRC "${CMAKE_CURRENT_SOURCE_DIR}/util/argparse.cpp")
list(REMOVE_ITEM LIB_SRC "${CMAKE_CURRENT_SOURCE_DIR}/back/interpreter/repl.cpp")

# executable
add_executable(ionia-bin ${SOURCES})
target_link_libraries(ionia-bin ${Readline_LIBRARY})
set_target_properties(ionia-bin PROPERTIES OUTPUT_NAME ionia)

# library
add_library(ionia ${LIB_SRC})
