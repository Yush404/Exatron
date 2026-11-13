cmake_minimum_required(VERSION 3.20)
project(Exatron VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(-Wall -Wextra -Wpedantic)

file(GLOB_RECURSE EXATRON_SOURCES CONFIGURE_DEPENDS "src/*.cpp")

set(EXATRON_CXX "g++" CACHE STRING "C++ compiler used by exatron backend")

add_executable(exatron ${EXATRON_SOURCES})
target_include_directories(exatron PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_definitions(exatron PRIVATE
    EXATRON_DEFAULT_CXX="${EXATRON_CXX}"
    EXATRON_VERSION="${PROJECT_VERSION}"
)

install(TARGETS exatron RUNTIME DESTINATION bin)
