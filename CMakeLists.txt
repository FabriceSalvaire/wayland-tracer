
####################################################################################################

cmake_minimum_required(VERSION 3.16)
project(wayland-tracer LANGUAGES C)

####################################################################################################

find_library(RT_LIBRARY names librt)
find_package(EXPAT)

####################################################################################################

add_executable(${PROJECT_NAME}
  src/wayland/connection.c
  src/wayland/wayland-os.c
  src/wayland/wayland-util.c
  src/frontend-analyze.c
  src/frontend-bin.c
  src/tracer-analyzer.c
  src/tracer.c
)
target_include_directories(${PROJECT_NAME}
  PRIVATE
  ${CMAKE_SOURCE_DIR}/src
  ${CMAKE_SOURCE_DIR}/src/wayland
)
add_compile_options(${PROJECT_NAME}
  "-Wall -Wextra -Wno-used-parameter -g -Wscrict-prototypes -Wmissing-prototypes -fvisibility=hidden"
)
target_link_libraries(${PROJECT_NAME} PUBLIC
  rt
  ${EXPAT_LIBRARIES}
)
