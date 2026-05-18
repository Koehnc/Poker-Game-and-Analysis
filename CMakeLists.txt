cmake_minimum_required(VERSION 3.16)
project(Poker CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(src)

# Core sources shared by both targets (no Qt dependency)
file(GLOB_RECURSE CORE_SOURCES
    "src/core/*.cpp"
    "src/game/*.cpp"
    "src/strategies/*.cpp"
    "src/evaluators/*.cpp"
    "src/ranges/*.cpp"
    "src/ga/*.cpp"
)

# Training binary (no Qt required)
add_executable(poker-train src/main.cpp ${CORE_SOURCES})

# UI binary (requires Qt6)
find_package(Qt6 QUIET COMPONENTS Widgets)
if(Qt6_FOUND)
    set(CMAKE_AUTOMOC ON)

    file(GLOB UI_SOURCES "src/ui/*.cpp")
    list(FILTER UI_SOURCES EXCLUDE REGEX ".*main_ui\\.cpp")

    add_executable(poker-ui src/ui/main_ui.cpp ${UI_SOURCES} ${CORE_SOURCES})
    target_link_libraries(poker-ui Qt6::Widgets)
    message(STATUS "Qt6 found — poker-ui target enabled")
else()
    message(STATUS "Qt6 not found — poker-ui target skipped")
endif()

# Genome serializer test
add_executable(test-genome tests/test_genome_serializer.cpp ${CORE_SOURCES})
