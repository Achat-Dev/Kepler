cmake_minimum_required (VERSION 3.16)

project ("kepler")

set(BUILD_ARCH "-m64")
set(TARGET_64 ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build the runtime
# Check the dependencies and fail if one of them isn't found
find_program(CLANG clang)
if(NOT CLANG)
    message(FATAL_ERROR "'clang' not found, which is a dependency of 'kepler'")
endif()

find_program(XXD xxd)
if(NOT XXD)
    message(FATAL_ERROR "'xxd' not found, which is required to embed the runtime")
endif()

# Compile the runtime to llvm bytecode
add_custom_command(
    OUTPUT runtime.bc
    COMMAND ${CLANG} -emit-llvm -O2 -c ${CMAKE_CURRENT_SOURCE_DIR}/src/runtime/runtime.c -o ${CMAKE_CURRENT_BINARY_DIR}/runtime.bc
    DEPENDS src/runtime/runtime.c
    COMMENT "Compiling the runtime")

# Create a .c file from that bytecode
add_custom_command(
    OUTPUT runtime.bc.c
    COMMAND ${XXD} -n __kepler_runtime_bc -i ${CMAKE_CURRENT_BINARY_DIR}/runtime.bc > ${CMAKE_CURRENT_BINARY_DIR}/runtime.bc.c
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/runtime.bc
    COMMENT "Embedding the runtime")

# Create a static library for the runtime
add_library(kepler-runtime STATIC ${CMAKE_CURRENT_BINARY_DIR}/runtime.bc.c)

# Import LLVM
find_package(LLVM REQUIRED CONFIG)
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

add_executable(${CMAKE_PROJECT_NAME}
    src/ast/literal_expressions/boolean_literal_expression.cpp
    src/ast/literal_expressions/floating_point_literal_expression.cpp
    src/ast/literal_expressions/integer_literal_expression.cpp
    src/ast/literal_expressions/string_literal_expression.cpp
    src/ast/binary_expression.cpp
    src/ast/call_expression.cpp
    src/ast/cast_expression.cpp
    src/ast/expression_result.cpp
    src/ast/for_expression.cpp
    src/ast/function.cpp
    src/ast/if_expression.cpp
    src/ast/llvm_value_wrapper_expression.cpp
    src/ast/negation_expression.cpp
    src/ast/prototype.cpp
    src/ast/return_expression.cpp
    src/ast/variable_expression.cpp
    src/ast/variable_definition_expression.cpp
    src/function_registry/function_registry.cpp
    src/runtime/runtime.cpp
    src/types/bool_type.cpp
    src/types/char_type.cpp
    src/types/float32_type.cpp
    src/types/float64_type.cpp
    src/types/floating_point_type.cpp
    src/types/int8_type.cpp
    src/types/int16_type.cpp
    src/types/int32_type.cpp
    src/types/int64_type.cpp
    src/types/string_type.cpp
    src/types/target_type_stack.cpp
    src/types/tmap_type.cpp
    src/types/type.cpp
    src/types/void_type.cpp
    src/variables/local_variables.cpp
    src/compiler.cpp
    src/file.cpp
    src/lexer.cpp
    src/main.cpp
    src/optimiser.cpp
    src/parser.cpp
    src/utils.cpp)

add_dependencies(${CMAKE_PROJECT_NAME} kepler-runtime)

llvm_map_components_to_libnames(llvm_libs ${LLVM_TARGETS_TO_BUILD})
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE kepler-runtime ${llvm_libs})

set_property(TARGET ${CMAKE_PROJECT_NAME} PROPERTY CXX_STANDARD 20)
