cmake_minimum_required(VERSION 3.21)
project(marty_varint CXX)

# Руководство по CMake для разработчиков C++ библиотек - https://habr.com/ru/articles/683204/
# https://github.com/pananton/cpp-lib-template/blob/main/CMakeLists.txt

# Сохранение древовидной структуры в Visual Studio с CMake или создание папок в проекте - https://habr.com/ru/articles/754272/


# string(COMPARE EQUAL "${CMAKE_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}" is_top_level)
# use PROJECT_IS_TOP_LEVEL instead (requires CMake v3.21)


if(NOT DEFINED CMAKE_BUILD_TYPE AND NOT DEFINED CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()


### End of common part


set(MODULE_ROOT "${CMAKE_CURRENT_LIST_DIR}")

file(GLOB sources "${MODULE_ROOT}/*.cpp")
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Source Files" FILES ${sources})

file(GLOB_RECURSE headers "${MODULE_ROOT}/*.h")
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Header Files" FILES ${headers})

file(GLOB_RECURSE all_docs "${MODULE_ROOT}/*.md" "${MODULE_ROOT}/*.txt")
list(FILTER all_docs EXCLUDE REGEX ".*CMakeLists\.txt$")
list(FILTER all_docs EXCLUDE REGEX ".*md_/.*")
list(FILTER all_docs EXCLUDE REGEX ".*_md/.*")
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Docs" FILES ${all_docs})

file(GLOB_RECURSE all_docs_src "${MODULE_ROOT}/*.md_" "${MODULE_ROOT}/_*.md")
list(FILTER all_docs_src EXCLUDE REGEX ".*md_/.*")
list(FILTER all_docs_src EXCLUDE REGEX ".*_md/.*")
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Docs.src" FILES ${all_docs_src})


add_library(${PROJECT_NAME} OBJECT ${sources} ${headers} ${all_docs} ${all_docs_src})
add_library(marty::varint ALIAS ${PROJECT_NAME})

target_compile_definitions(${PROJECT_NAME} PRIVATE WIN32_LEAN_AND_MEAN)
