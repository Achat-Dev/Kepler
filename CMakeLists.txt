cmake_minimum_required (VERSION 3.16)

project ("kepler")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Import LLVM
find_package(LLVM REQUIRED CONFIG)

add_executable(${CMAKE_PROJECT_NAME}
    src/ast/expressions/literals/boolean_literal_expression.cpp
    src/ast/expressions/literals/floating_point_literal_expression.cpp
    src/ast/expressions/literals/integer_literal_expression.cpp
    src/ast/expressions/literals/string_literal_expression.cpp
    src/ast/expressions/binary_expression.cpp
    src/ast/expressions/call_expression.cpp
    src/ast/expressions/cast_expression.cpp
    src/ast/expressions/negation_expression.cpp
    src/ast/expressions/variable_expression.cpp
    src/ast/function.cpp
    src/ast/prototype.cpp
    src/ast/statements/assignment_statement.cpp
    src/ast/statements/if_statement.cpp
    src/ast/statements/for_statement.cpp
    src/ast/statements/return_statement.cpp
    src/ast/statements/variable_definition_statement.cpp
    src/diagnostics/diagnostic_code.cpp
    src/diagnostics/diagnostic_sink.cpp
    src/io/file.cpp
    src/lexer/tokenizer.cpp
    src/parser/parser.cpp
    src/semantic_analysis/string_table.cpp
    src/semantic_analysis/symbol_table.cpp
    src/type_system/data_type_kind.cpp
    src/compilation_context.cpp
    src/compiler.cpp
    src/main.cpp)

target_compile_features(${CMAKE_PROJECT_NAME} PRIVATE cxx_std_23)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/external/cxxopts/include)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE LLVM)

set_property(TARGET ${CMAKE_PROJECT_NAME} PROPERTY CXX_STANDARD 20)
