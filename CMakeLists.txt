####################################################################################################

cmake_minimum_required(VERSION 3.16)
project(wayland-tracer LANGUAGES C)

####################################################################################################

find_library(RT_LIBRARY names librt)
find_package(EXPAT)

####################################################################################################

add_executable(${PROJECT_NAME}
  src/connection.c
  src/frontend-analyze.c
  src/frontend-bin.c
  src/tracer-analyzer.c
  src/tracer.c
  src/wayland-os.c
  src/wayland-util.c
)
add_compile_options(${PROJECT_NAME}
  "-Wall -Wextra -Wno-used-parameter -g -Wscrict-prototypes -Wmissing-prototypes -fvisibility=hidden"
)
target_link_libraries(${PROJECT_NAME} PUBLIC
  rt
  ${EXPAT_LIBRARIES}
)
