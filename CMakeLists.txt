cmake_minimum_required (VERSION 3.16)

project ("kepler")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Import LLVM
find_package(LLVM REQUIRED CONFIG)

add_executable(${CMAKE_PROJECT_NAME}
    src/lexer/operator_type.cpp
    src/lexer/token.cpp
    src/lexer/token_type.cpp
    src/lexer/tokenizer.cpp
    src/parser/parser.cpp
    src/semantic_analysis/string_table.cpp
    src/type_system/data_type_kind.cpp
    src/command_line_arguments.cpp
    src/compiler.cpp
    src/main.cpp)

target_compile_features(${CMAKE_PROJECT_NAME} PRIVATE cxx_std_23)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/external/cxxopts/include)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE LLVM)

set_property(TARGET ${CMAKE_PROJECT_NAME} PROPERTY CXX_STANDARD 20)
