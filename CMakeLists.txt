cmake_minimum_required(VERSION 3.10)
project(kcomp VERSION 1.0.2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

add_executable(kcomp
  src/main.cpp
  src/io/file_io.cpp
  src/core/range_coder.cpp
  src/core/benchmark.cpp
  src/models/model257.cpp
  src/models/ppm.cpp
  src/models/rle.cpp
  src/models/lz77.cpp
  src/models/lzopt.cpp
  src/models/lzx.cpp
  src/models/bwt.cpp
  src/models/mixer.cpp
  src/models/cm.cpp
  src/models/dict.cpp
  src/models/lzma.cpp
)

target_include_directories(kcomp PRIVATE src)
target_compile_definitions(kcomp PRIVATE KCOMP_VERSION="${PROJECT_VERSION}")

if(MSVC)
  target_compile_options(kcomp PRIVATE /W4)
else()
  target_compile_options(kcomp PRIVATE -Wall -Wextra -pedantic)
endif()

# Install target
include(GNUInstallDirs)
install(TARGETS kcomp
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Uninstall target
if(NOT TARGET uninstall)
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

  add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
endif()