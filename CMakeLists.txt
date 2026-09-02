cmake_minimum_required (VERSION 3.16)

project ("kepler")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Import LLVM
find_package(LLVM REQUIRED CONFIG)

add_executable(${CMAKE_PROJECT_NAME}
    src/ast/prototype.cpp
    src/codegen/codegen_pass.cpp
    src/codegen/optimizer.cpp
    src/diagnostics/diagnostic.cpp
    src/diagnostics/diagnostic_sink.cpp
    src/io/file.cpp
    src/lexer/operator_type.cpp
    src/lexer/tokenizer.cpp
    src/parser/parser.cpp
    src/parser/parser.expressions.cpp
    src/parser/parser.statements.cpp
    src/parser/parser.top_level.cpp
    src/semantic_analysis/name_resolution_pass.cpp
    src/semantic_analysis/return_check_pass.cpp
    src/semantic_analysis/symbol_table.cpp
    src/type_system/type.cpp
    src/type_system/type_check_pass.cpp
    src/type_system/type_table.cpp
    src/utils/arena_allocator.cpp
    src/utils/ast_print_pass.cpp
    src/utils/string_pool.cpp
    src/compiler.cpp
    src/main.cpp)

target_compile_features(${CMAKE_PROJECT_NAME} PRIVATE cxx_std_23)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE ${LLVM_DEFINITIONS})
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${LLVM_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/external/cxxopts/include)
if(TARGET LLVM)
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE LLVM)
else()
  llvm_map_components_to_libnames(LLVM_LIBS ${LLVM_TARGETS_TO_BUILD})
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE ${LLVM_LIBS})
endif()
