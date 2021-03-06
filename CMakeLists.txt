cmake_minimum_required(VERSION 3.0)
project(SpaceAdventure)

# Bump up warning levels appropriately for clang and gcc
# Also set debug/optimization flags depending on the build type. IDE users choose this when
# selecting the build mode in their IDE
if (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU" OR ${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -pedantic -std=c++11")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")
endif()

option(PROFILING "Enable instrumentation code" ON)
if (PROFILING)
  add_definitions(-DPROFILING)
endif()

set(TECHNIQUE "DOD" CACHE STRING "Data Oriented Design (DOD) or Object Oriented Progrmamming (OOP)") 

# lookup GLFW
find_package(PkgConfig REQUIRED)
pkg_search_module(GLFW REQUIRED glfw3)
include_directories(${GLFW_INCLUDE_DIR})

# lookup OpenGL and add the include directory to our include path
find_package(OpenGL REQUIRED)
include_directories(${OPENGL_INCLUDE_DIR})

# lookup OpenGL Extension Wrangler
find_package(GLEW REQUIRED)

# lookup Performance API
if (PROFILING)
  pkg_search_module(PAPI REQUIRED papi)
  include_directories(${PAPI_INCLUDE_DIRS})
endif()

# build the game
if (${TECHNIQUE} STREQUAL "DOD")
  add_definitions(-DDOD)
  add_executable(GAME Debug.cpp Asset.cpp EntityManager.cpp CompManagers.cpp Main.cpp)
elseif (${TECHNIQUE} STREQUAL "OOP")
  add_definitions(-DOOP)
  add_executable(GAME Debug.cpp Asset.cpp MathOOP.cpp EntityOOP.cpp CompManagersOOP.cpp MainOOP.cpp)
endif()

# add the static library for Simple Opengl Image Library 2, and include its header
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(GAME ${GLEW_LIBRARIES} ${GLFW_STATIC_LIBRARIES} ${PAPI_LIBRARIES} ${CMAKE_CURRENT_SOURCE_DIR}/lib/libsoil2-debug.a)

# set an output directory for our binaries
set(BIN_DIR ${SpaceAdventure_SOURCE_DIR})
if (${CMAKE_BUILD_TYPE} STREQUAL "Release")
  set(BIN_DIR ${BIN_DIR}/release)
elseif (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
  set(BIN_DIR ${BIN_DIR}/debug)
endif()
if (${TECHNIQUE} STREQUAL "DOD")
  set(BIN_DIR ${BIN_DIR}/dod)
elseif (${TECHNIQUE} STREQUAL "OOP")
  set(BIN_DIR ${BIN_DIR}/oop)
endif()

install(TARGETS GAME DESTINATION ${BIN_DIR})
  
add_custom_command(TARGET GAME PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets ${BIN_DIR})
