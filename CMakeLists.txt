cmake_minimum_required(VERSION 3.0)
project(elera C)
set(CMAKE_C_STANDARD 99)

# Adding Raylib
include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)

set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE) # don't build the supplied examples
set(BUILD_GAMES    OFF CACHE BOOL "" FORCE) # don't build the supplied example games
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

FetchContent_Declare(
  raylib
  GIT_REPOSITORY "https://github.com/raysan5/raylib.git"
  GIT_TAG "master"
  GIT_PROGRESS TRUE
)

FetchContent_Declare(
  ZeroMQ
  GIT_REPOSITORY "https://github.com/zeromq/libzmq.git"
  GIT_TAG "master"
  GIT_PROGRESS TRUE
)

FetchContent_MakeAvailable(raylib ZeroMQ)


# Adding our source files
file(GLOB_RECURSE PROJECT_SOURCES_CLIENT CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/moon/sources/*.c") # Define PROJECT_SOURCES as a list of all source files
file(GLOB_RECURSE PROJECT_SOURCES_SERVER CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/sun/sources/*.c") # Define PROJECT_SOURCES as a list of all source files
set(PROJECT_INCLUDE_CLIENT "${CMAKE_CURRENT_LIST_DIR}/moon/inc/") # Define PROJECT_INCLUDE to be the path to the include directory of the project
set(PROJECT_INCLUDE_SERVER "${CMAKE_CURRENT_LIST_DIR}/sun/inc/") # Define PROJECT_INCLUDE to be the path to the include directory of the project


# Declaring our executable
add_executable(${PROJECT_NAME}_server)
target_sources(${PROJECT_NAME}_server PRIVATE ${PROJECT_SOURCES_SERVER})
target_include_directories(${PROJECT_NAME}_server PRIVATE ${PROJECT_INCLUDE_SERVER} ${zeromq_SOURCE_DIR}/include)
target_link_libraries(${PROJECT_NAME}_server PRIVATE libzmq  raylib "-framework Cocoa" "-framework IOKit" "-framework GLUT" "-framework OpenGL")

# Setting ASSETS_PATH
target_compile_definitions(${PROJECT_NAME}_server PUBLIC ASSETS_PATH="${CMAKE_CURRENT_SOURCE_DIR}/assets/") # Set the asset path macro to the absolute path on the dev machine
add_executable(${PROJECT_NAME}_client)
target_sources(${PROJECT_NAME}_client PRIVATE ${PROJECT_SOURCES_CLIENT})
target_include_directories(${PROJECT_NAME}_client PRIVATE ${PROJECT_INCLUDE_CLIENT} ${zeromq_SOURCE_DIR}/include)
target_link_libraries(${PROJECT_NAME}_client PRIVATE libzmq  raylib "-framework Cocoa" "-framework IOKit" "-framework GLUT" "-framework OpenGL")

# Setting ASSETS_PATH
target_compile_definitions(${PROJECT_NAME}_client PUBLIC ASSETS_PATH="${CMAKE_CURRENT_SOURCE_DIR}/assets/") # Set the asset path macro to the absolute path on the dev machine
