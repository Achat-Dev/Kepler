cmake_minimum_required (VERSION 3.8)

project ("kepler")

set(BUILD_ARCH "-m64")
set(TARGET_64 ON)

#import LLVM
find_package(LLVM REQUIRED CONFIG)
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

set(AST_SRC
    "src/ast/binary_expression.cpp"
    "src/ast/call_expression.cpp"
    "src/ast/cast_expression.cpp"
    "src/ast/expression_result.cpp"
    "src/ast/for_expression.cpp"
    "src/ast/function.cpp"
    "src/ast/if_expression.cpp"
    "src/ast/negation_expression.cpp"
    "src/ast/prototype.cpp"
    "src/ast/return_expression.cpp"
    "src/ast/variable_expression.cpp"
    "src/ast/variable_definition_expression.cpp"
    "src/ast/literal_expressions/boolean_literal_expression.cpp"
    "src/ast/literal_expressions/floating_point_literal_expression.cpp"
    "src/ast/literal_expressions/integer_literal_expression.cpp"
    "src/ast/literal_expressions/string_literal_expression.cpp")

set(FUNCTION_REGISTRY_SRC
    "src/function_registry/function_registry.cpp")

set(TYPES_SRC
    "src/types/bool_type.cpp"
    "src/types/float32_type.cpp"
    "src/types/float64_type.cpp"
    "src/types/int8_type.cpp"
    "src/types/int16_type.cpp"
    "src/types/int32_type.cpp"
    "src/types/int64_type.cpp"
    "src/types/string_type.cpp"
    "src/types/target_type_stack.cpp"
    "src/types/type.cpp"
    "src/types/type_helper.cpp")

set(VARIABLES_SRC
    "src/variables/local_variables.cpp")

add_executable (${CMAKE_PROJECT_NAME}
    "src/main.cpp"
    "src/compiler.cpp"
    "src/file.cpp"
    "src/lexer.cpp"
    "src/optimiser.cpp"
    "src/parser.cpp"
    "src/utils.cpp"
    ${AST_SRC}
    ${FUNCTION_REGISTRY_SRC}
    ${TYPES_SRC}
    ${VARIABLES_SRC})

llvm_map_components_to_libnames(llvm_libs ${LLVM_TARGETS_TO_BUILD})

target_link_libraries(${CMAKE_PROJECT_NAME} ${llvm_libs})

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET ${CMAKE_PROJECT_NAME} PROPERTY CXX_STANDARD 20)
endif()
