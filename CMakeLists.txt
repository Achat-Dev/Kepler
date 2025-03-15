cmake_minimum_required (VERSION 3.8)

project ("kepler")

set(BUILD_ARCH "-m64")
set(TARGET_64 ON)

#import LLVM
find_package(LLVM REQUIRED CONFIG)
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

add_executable (${CMAKE_PROJECT_NAME} "src/main.cpp" "src/lexer.cpp" )

llvm_map_components_to_libnames(llvm_libs
	support
	core
	irreader
)

target_link_libraries(${CMAKE_PROJECT_NAME} ${llvm_libs})

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET ${CMAKE_PROJECT_NAME} PROPERTY CXX_STANDARD 20)
endif()
