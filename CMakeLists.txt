cmake_minimum_required(VERSION 3.5)
project(elera C)
set(CMAKE_C_STANDARD 99)

# Optional: enforce using Homebrew paths on macOS
if(APPLE)
  set(CMAKE_PREFIX_PATH "/opt/homebrew" CACHE STRING "" FORCE)  # M1 Macs
  include_directories(/opt/homebrew/include)
  link_directories(/opt/homebrew/lib)
endif()

# Add our source files
file(GLOB_RECURSE PROJECT_SOURCES_CLIENT CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/moon/sources/*.c")
file(GLOB_RECURSE PROJECT_SOURCES_SERVER CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/sun/sources/*.c")
set(PROJECT_INCLUDE_CLIENT "${CMAKE_CURRENT_LIST_DIR}/moon/inc/")
set(PROJECT_INCLUDE_SERVER "${CMAKE_CURRENT_LIST_DIR}/sun/inc/")

# Server executable
add_executable(${PROJECT_NAME}_server)
target_sources(${PROJECT_NAME}_server PRIVATE ${PROJECT_SOURCES_SERVER})
target_include_directories(${PROJECT_NAME}_server PRIVATE ${PROJECT_INCLUDE_SERVER})
target_compile_definitions(${PROJECT_NAME}_server PUBLIC ASSETS_PATH="${CMAKE_CURRENT_SOURCE_DIR}/assets/")

# Client executable
add_executable(${PROJECT_NAME}_client)
target_sources(${PROJECT_NAME}_client PRIVATE ${PROJECT_SOURCES_CLIENT})
target_include_directories(${PROJECT_NAME}_client PRIVATE ${PROJECT_INCLUDE_CLIENT})
target_compile_definitions(${PROJECT_NAME}_client PUBLIC ASSETS_PATH="${CMAKE_CURRENT_SOURCE_DIR}/assets/")

# Platform-specific linking
if(APPLE)
  target_link_libraries(${PROJECT_NAME}_server PRIVATE czmq zmq raylib "-framework Cocoa" "-framework IOKit" "-framework GLUT" "-framework OpenGL")
  target_link_libraries(${PROJECT_NAME}_client PRIVATE pthread czmq zmq raylib "-framework Cocoa" "-framework IOKit" "-framework GLUT" "-framework OpenGL")
elseif(UNIX AND NOT APPLE)
  target_link_libraries(${PROJECT_NAME}_server PRIVATE czmq zmq raylib GL m pthread dl X11)
  target_link_libraries(${PROJECT_NAME}_client PRIVATE czmq zmq raylib GL m pthread dl X11)
endif()
