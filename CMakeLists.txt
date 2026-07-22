cmake_minimum_required (VERSION 3.16)

project ("kepler")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Import LLVM
find_package(LLVM REQUIRED CONFIG)

add_executable(${CMAKE_PROJECT_NAME}
    src/ast/ast_printer.cpp
    src/diagnostics/diagnostic.cpp
    src/diagnostics/diagnostic_sink.cpp
    src/io/file.cpp
    src/lexer/tokenizer.cpp
    src/parser/parser.cpp
    src/parser/parser.expressions.cpp
    src/parser/parser.statements.cpp
    src/parser/parser.top_level.cpp
    src/semantic_analysis/name_resolution_pass.cpp
    src/semantic_analysis/symbol_table.cpp
    src/type_system/data_type_kind.cpp
    src/compilation_context.cpp
    src/compiler.cpp
    src/main.cpp
    src/string_pool.cpp)

target_compile_features(${CMAKE_PROJECT_NAME} PRIVATE cxx_std_23)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/external/cxxopts/include)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE LLVM)

set_property(TARGET ${CMAKE_PROJECT_NAME} PROPERTY CXX_STANDARD 20)
