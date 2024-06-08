cmake_minimum_required (VERSION 3.8)

if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("Kepler")

set(BUILD_ARCH "-m64")
set(TARGET_64 ON)

#import LLVM
find_package(LLVM REQUIRED CONFIG)
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

add_executable (Kepler "src/main.cpp" "src/lexing/lexer.cpp" "src/lexing/tokens.cpp")

llvm_map_components_to_libnames(llvm_libs
	support
	core
	irreader
)

target_link_libraries(Kepler ${llvm_libs})

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Kepler PROPERTY CXX_STANDARD 20)
endif()
