# Using the same minimum as the godot-cpp project
cmake_minimum_required(VERSION 3.17)

# Silence unused variable warning when specified from toolchain
if(CMAKE_C_COMPILER)
endif()

set(LIBNAME "EXTENSION-NAME") # "The name of the library"
set(GODOT_PROJECT_DIR "project" CACHE STRING "The directory of a Godot project folder")

# Make sure all the dependencies are satisfied
find_package(Python3 3.4 REQUIRED)
find_program(GIT git REQUIRED)

# Use pkg-config for gRPC and Protobuf
find_package(PkgConfig REQUIRED)
pkg_check_modules(GRPC REQUIRED grpc++ grpc)
pkg_check_modules(PROTOBUF REQUIRED protobuf)

# Find protoc and grpc_cpp_plugin
find_program(PROTOC protoc REQUIRED)
find_program(GRPC_CPP_PLUGIN grpc_cpp_plugin REQUIRED)

# Ensure godot-cpp submodule has been updated
if(NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/godot-cpp/src")
    message(NOTICE "godot-cpp bindings source not found")
    message(NOTICE "initializing/updating the godot-cpp submodule...")

    # update the c++ bindings submodule to populate it with the necessary source for the library
    execute_process(
        COMMAND git submodule update --init godot-cpp
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
        COMMAND_ERROR_IS_FATAL ANY
    )
endif()
add_subdirectory(godot-cpp SYSTEM)

# When compiling for emscripten, this toolchainhack needs to be included.
# set(CMAKE_PROJECT_<project-name>_INCLUDE ${godot-cpp_SOURCE_DIR}/cmake/emsdkHack.cmake)

# Create the gen directory at configure time so IDEs can see generated headers immediately after generate_bindings
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/godot-cpp/gen/include")

# Add godot-cpp's module path and include the exported functions.
# This is made available for documentation generation
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${godot-cpp_SOURCE_DIR}/cmake")
include(GodotCPPModule)

# The godot-cpp target has some of useful properties attached that can be retrieved like so.
get_target_property(GODOTCPP_SUFFIX godot::cpp GODOTCPP_SUFFIX)
get_target_property(GODOTCPP_PLATFORM godot::cpp GODOTCPP_PLATFORM)

# Now we can specify our own project which will inherit any global cmake properties or variables that have been defined.
project(godot-cpp-template
    VERSION 1.0
    DESCRIPTION "This repository serves as a quickstart template for GDExtension development with Godot 4.0+."
    HOMEPAGE_URL "https://github.com/enetheru/godot-cpp-template/tree/main"
    LANGUAGES CXX
)

# Set protobuf and gRPC directories
set(PROTO_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/proto")
set(PROTO_GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/proto_gen")

# Create directories for generated protobuf files
file(MAKE_DIRECTORY ${PROTO_GEN_DIR})
file(MAKE_DIRECTORY "${PROTO_GEN_DIR}/include")
file(MAKE_DIRECTORY "${PROTO_GEN_DIR}/src")

# Set protobuf files
set(PROTO_FILES
    "${PROTO_SRC_DIR}/murder_trivia.proto"
)

# Generate C++ files from protobuf
set(PROTO_SRCS)
set(PROTO_HDRS)
set(GRPC_SRCS)
set(GRPC_HDRS)

foreach(PROTO_FILE ${PROTO_FILES})
    get_filename_component(PROTO_NAME ${PROTO_FILE} NAME_WE)
    get_filename_component(PROTO_PATH ${PROTO_FILE} DIRECTORY)
    
    # Generate protobuf C++ files
    list(APPEND PROTO_SRCS "${PROTO_GEN_DIR}/${PROTO_NAME}.pb.cc")
    list(APPEND PROTO_HDRS "${PROTO_GEN_DIR}/${PROTO_NAME}.pb.h")
    
    # Generate gRPC C++ files
    list(APPEND GRPC_SRCS "${PROTO_GEN_DIR}/${PROTO_NAME}.grpc.pb.cc")
    list(APPEND GRPC_HDRS "${PROTO_GEN_DIR}/${PROTO_NAME}.grpc.pb.h")
    
    add_custom_command(
        OUTPUT 
            "${PROTO_GEN_DIR}/${PROTO_NAME}.pb.cc"
            "${PROTO_GEN_DIR}/${PROTO_NAME}.pb.h"
            "${PROTO_GEN_DIR}/${PROTO_NAME}.grpc.pb.cc"
            "${PROTO_GEN_DIR}/${PROTO_NAME}.grpc.pb.h"
        COMMAND ${PROTOC}
            ARGS 
                --proto_path=${PROTO_SRC_DIR}
                --cpp_out=${PROTO_GEN_DIR}
                --grpc_out=${PROTO_GEN_DIR}
                --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN}
                ${PROTO_FILE}
        DEPENDS ${PROTO_FILE}
        COMMENT "Generating C++ code from ${PROTO_FILE}"
        VERBATIM
    )
endforeach()

# Create a target for proto generation
add_custom_target(proto_gen DEPENDS ${PROTO_SRCS} ${PROTO_HDRS} ${GRPC_SRCS} ${GRPC_HDRS})

add_library(${LIBNAME} SHARED)

target_sources(${LIBNAME}
    PRIVATE
    src/game_manager.cpp
    src/game_manager.h
    src/register_types.cpp
    src/register_types.h
    ${PROTO_SRCS}
    ${GRPC_SRCS}
)

# Include directories for protobuf and gRPC
target_include_directories(${LIBNAME}
    PRIVATE
    ${PROTO_GEN_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${PROTOBUF_INCLUDE_DIRS}
    ${GRPC_INCLUDE_DIRS}
)

# Fetch a list of the xml files to use for documentation and add to our target
file(GLOB_RECURSE DOC_XML LIST_DIRECTORIES NO CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/doc_classes/*.xml")

# conditionally add doc data to compile output
if(DOC_XML)
    if(GODOTCPP_TARGET MATCHES "editor|template_debug")
        target_doc_sources(${LIBNAME} "${DOC_XML}")
    endif()
endif()

# Link gRPC and Protobuf libraries
target_link_libraries(${LIBNAME} 
    PRIVATE 
    godot-cpp
    ${GRPC_LIBRARIES}
    ${PROTOBUF_LIBRARIES}
)

# Add dependency on proto_gen target
add_dependencies(${LIBNAME} proto_gen)

# Require at least C++17 for this target
set_property(TARGET ${LIBNAME} PROPERTY CXX_STANDARD 17)

# Enable necessary compiler flags for gRPC/Protobuf
target_compile_definitions(${LIBNAME}
    PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:_WIN32_WINNT=0x0601>
)

# Add necessary compile options for gRPC/Protobuf
target_compile_options(${LIBNAME}
    PRIVATE
    ${GRPC_CFLAGS_OTHER}
    ${PROTOBUF_CFLAGS_OTHER}
)

set_target_properties(${LIBNAME}
    PROPERTIES
    # The generator expression here prevents msvc from adding a Debug or Release subdir.
    RUNTIME_OUTPUT_DIRECTORY "$<1:${PROJECT_SOURCE_DIR}/bin/${GODOTCPP_PLATFORM}>"

    PREFIX "lib"
    OUTPUT_NAME "${LIBNAME}${GODOTCPP_SUFFIX}"
)

set(GODOT_PROJECT_BINARY_DIR "${PROJECT_SOURCE_DIR}/${GODOT_PROJECT_DIR}/bin/${GODOTCPP_PLATFORM}")

add_custom_command(TARGET ${LIBNAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:${LIBNAME}>" "${GODOT_PROJECT_BINARY_DIR}/$<TARGET_FILE_NAME:${LIBNAME}>"
)
