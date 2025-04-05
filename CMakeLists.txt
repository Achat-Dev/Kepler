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
    "src/ast/for_expression.cpp"
    "src/ast/function.cpp"
    "src/ast/if_expression.cpp"
    "src/ast/number_expression.cpp"
    "src/ast/prototype.cpp"
    "src/ast/variable_expression.cpp")

add_executable (${CMAKE_PROJECT_NAME}
    "src/main.cpp"
    "src/compiler.cpp"
    "src/lexer.cpp"
    "src/log.cpp"
    "src/optimiser.cpp"
    "src/parser.cpp"
    ${AST_SRC})

llvm_map_components_to_libnames(llvm_libs ${LLVM_TARGETS_TO_BUILD})

target_link_libraries(${CMAKE_PROJECT_NAME} ${llvm_libs})

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET ${CMAKE_PROJECT_NAME} PROPERTY CXX_STANDARD 20)
endif()
