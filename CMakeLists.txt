cmake_minimum_required(VERSION 3.17)

project(godot-cpp-template
    VERSION 1.0
    DESCRIPTION "This repository serves as a quickstart template for GDExtension development with Godot 4.0+."
    HOMEPAGE_URL "https://github.com/enetheru/godot-cpp-template/tree/main"
    LANGUAGES CXX
)

set(LIBNAME "EXTENSION-NAME") 
set(GODOT_PROJECT_DIR "project" CACHE STRING "The directory of a Godot project folder")

# Находим ZLIB (системный)
find_package(ZLIB REQUIRED)

# Используем встроенный модуль FindProtobuf
find_package(Protobuf REQUIRED)

# Для gRPC используем pkg-config
find_package(PkgConfig REQUIRED)
pkg_check_modules(GRPC REQUIRED grpc++ grpc)

# Находим плагин grpc_cpp_plugin (обычно в protobuf-compiler-grpc)
find_program(GRPC_CPP_PLUGIN grpc_cpp_plugin REQUIRED)

# Проверка наличия git для подмодуля
find_program(GIT git REQUIRED)

# Ensure godot-cpp submodule has been updated
if(NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/godot-cpp/src")
    message(NOTICE "godot-cpp bindings source not found")
    message(NOTICE "initializing/updating the godot-cpp submodule...")

    execute_process(
        COMMAND git submodule update --init godot-cpp
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
        COMMAND_ERROR_IS_FATAL ANY
    )
endif()
add_subdirectory(godot-cpp SYSTEM)

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/godot-cpp/gen/include")

set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${godot-cpp_SOURCE_DIR}/cmake")
include(GodotCPPModule)

get_target_property(GODOTCPP_SUFFIX godot::cpp GODOTCPP_SUFFIX)
get_target_property(GODOTCPP_PLATFORM godot::cpp GODOTCPP_PLATFORM)

# Настройка путей генерации gRPC
set(PROTO_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/proto")
set(PROTO_FILES "${PROTO_SRC_DIR}/murder_trivia.proto")

set(GEN_PROTO_SRCS "${CMAKE_CURRENT_BINARY_DIR}/murder_trivia.pb.cc")
set(GEN_PROTO_HDRS "${CMAKE_CURRENT_BINARY_DIR}/murder_trivia.pb.h")
set(GEN_GRPC_SRCS  "${CMAKE_CURRENT_BINARY_DIR}/murder_trivia.grpc.pb.cc")
set(GEN_GRPC_HDRS  "${CMAKE_CURRENT_BINARY_DIR}/murder_trivia.grpc.pb.h")

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

add_custom_command(
    OUTPUT ${GEN_PROTO_SRCS} ${GEN_PROTO_HDRS} ${GEN_GRPC_SRCS} ${GEN_GRPC_HDRS}
    COMMAND ${PROTOBUF_PROTOC_EXECUTABLE}
    ARGS --proto_path=${PROTO_SRC_DIR}
         --cpp_out=${CMAKE_CURRENT_BINARY_DIR}
         --grpc_out=${CMAKE_CURRENT_BINARY_DIR}
         --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN}
         ${PROTO_FILES}
    DEPENDS ${PROTO_FILES}
    COMMENT "Running C++ gRPC and Protocol Buffer compiler on ${PROTO_FILES}"
    VERBATIM
)

add_library(${LIBNAME} SHARED)

target_sources(${LIBNAME}
    PRIVATE
    src/game_manager.cpp
    src/game_manager.h
    src/register_types.cpp
    src/register_types.h
    ${GEN_PROTO_SRCS}
    ${GEN_GRPC_SRCS}
)

target_include_directories(${LIBNAME} PRIVATE 
    ${CMAKE_CURRENT_BINARY_DIR}
    ${PROTOBUF_INCLUDE_DIRS}
    ${GRPC_INCLUDE_DIRS}
)

file(GLOB_RECURSE DOC_XML LIST_DIRECTORIES NO CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/doc_classes/*.xml")

if(DOC_XML)
    if(GODOTCPP_TARGET MATCHES "editor|template_debug")
        target_doc_sources(${LIBNAME} "${DOC_XML}")
    endif()
endif()

target_link_libraries(${LIBNAME} 
    PRIVATE 
    godot-cpp
    ${GRPC_LIBRARIES}
    ${PROTOBUF_LIBRARIES}
    ZLIB::ZLIB
)

set_property(TARGET ${LIBNAME} PROPERTY CXX_STANDARD 17)

set_target_properties(${LIBNAME}
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "$<1:${PROJECT_SOURCE_DIR}/bin/${GODOTCPP_PLATFORM}>"
    PREFIX "lib"
    OUTPUT_NAME "${LIBNAME}${GODOTCPP_SUFFIX}"
)

set(GODOT_PROJECT_BINARY_DIR "${PROJECT_SOURCE_DIR}/${GODOT_PROJECT_DIR}/bin/${GODOTCPP_PLATFORM}")

add_custom_command(TARGET ${LIBNAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:${LIBNAME}>" "${GODOT_PROJECT_BINARY_DIR}/$<TARGET_FILE_NAME:${LIBNAME}>"
)