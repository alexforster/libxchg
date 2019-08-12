cmake_minimum_required(VERSION 3.13)
project(libxchg C CXX)
set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# lib/libxchg.so & lib/libxchg_static.a

file(GLOB LIBXCHG_SRC ${CMAKE_SOURCE_DIR}/src/*.c)

add_library(xchg SHARED ${LIBXCHG_SRC})
set_target_properties(xchg PROPERTIES LINKER_LANGUAGE "C")
target_include_directories(xchg PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_library(xchg_static STATIC ${LIBXCHG_SRC})
set_target_properties(xchg_static PROPERTIES LINKER_LANGUAGE "C")
target_include_directories(xchg_static PRIVATE ${CMAKE_SOURCE_DIR}/include)

# bin/xchg_tests

file(GLOB LIBXCHG_TESTS ${CMAKE_SOURCE_DIR}/tests/*.cpp)
add_executable(xchg_tests EXCLUDE_FROM_ALL ${LIBXCHG_SRC} ${LIBXCHG_TESTS})
set_target_properties(xchg_tests PROPERTIES LINKER_LANGUAGE "CXX")
target_include_directories(xchg_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(xchg_tests pthread xchg_static)
if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
  if(${CMAKE_C_COMPILER_ID} MATCHES "(Apple)?[Cc]lang")
    target_compile_options(xchg_tests PRIVATE -fprofile-instr-generate -fcoverage-mapping)
    target_link_options(xchg_tests PRIVATE -fprofile-instr-generate -fcoverage-mapping)
  elseif(${CMAKE_C_COMPILER_ID} MATCHES "GNU")
    target_compile_options(xchg_tests PRIVATE --coverage)
    target_link_options(xchg_tests PRIVATE --coverage)
  endif()
endif()

add_custom_target(test COMMAND ${CMAKE_SOURCE_DIR}/bin/xchg_tests -s -r compact)
add_dependencies(test xchg_tests)

# bin/xchg_fuzz

file(GLOB LIBXCHG_FUZZ ${CMAKE_SOURCE_DIR}/tests/fuzz/*.cpp)
add_executable(xchg_fuzz EXCLUDE_FROM_ALL ${LIBXCHG_SRC} ${LIBXCHG_FUZZ})
set_target_properties(xchg_fuzz PROPERTIES LINKER_LANGUAGE "CXX")
target_include_directories(xchg_fuzz PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(xchg_fuzz pthread xchg_static)

add_custom_target(fuzz COMMAND AFL_HARDEN=1 afl-fuzz -i ${CMAKE_SOURCE_DIR}/tests/fuzz/testcases -o ${CMAKE_SOURCE_DIR}/tests/fuzz/findings -- ${CMAKE_SOURCE_DIR}/bin/xchg_fuzz)
add_dependencies(fuzz xchg_fuzz)

# bin/examples_serialization

add_executable(xchg_examples_serialization EXCLUDE_FROM_ALL ${CMAKE_SOURCE_DIR}/examples/serialization.c)
set_target_properties(xchg_examples_serialization PROPERTIES LINKER_LANGUAGE "C")
target_include_directories(xchg_examples_serialization PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(xchg_examples_serialization pthread xchg_static)
