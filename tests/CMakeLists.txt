cmake_minimum_required(VERSION 3.30)
project(refactor_tool VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wfatal-errors)
endif()

#----------------------------------------------------------------------------------------------------------------------
# testing framework
#----------------------------------------------------------------------------------------------------------------------

include(FetchContent)
FetchContent_Declare(googletest URL https://github.com/google/googletest/archive/refs/tags/release-1.12.1.tar.gz)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE) # do not override parent project's runtime settings on Windows
set(INSTALL_GTEST OFF)

# For simplicity, always build googletest as static library. This prevents mylib-tests executable from
# complaining about missing googletest DLLs on Windows.
set(BUILD_SHARED_LIBS OFF)

FetchContent_MakeAvailable(googletest)

#
# Тесты
#

# Рекурсивно ищем все используемые в тестах .cpp файлы
file(GLOB TEST_SRC_FILES 
    "${CMAKE_SOURCE_DIR}/tests/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/RefactorTool.cpp"
)

add_executable(refactor_tool_tests "${TEST_SRC_FILES}")
target_include_directories(refactor_tool_tests
    PRIVATE
        ${gtest_SOURCE_DIR}/include
        ${gtest_SOURCE_DIR}
)

target_include_directories(refactor_tool_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(refactor_tool_tests PRIVATE 
    GTest::gtest_main
    clangTooling
    clangBasic
    clangASTMatchers
    clangAST
    clangFrontend
    clangLex
    clangParse
    clangSema
    clangEdit
    clangRewrite
    LLVMCore
    LLVMSupport
    LLVMAnalysis
)
target_include_directories(refactor_tool_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_test(NAME refactor_tool_tests COMMAND refactor_tool_tests)