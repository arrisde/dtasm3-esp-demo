cmake_minimum_required(VERSION 3.16.0)

FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)
idf_component_register(SRCS ${app_sources})
include_directories(
    ${CMAKE_SOURCE_DIR}/extern/dtasm.git/runtime/dtasm3/include
)

add_subdirectory(${CMAKE_SOURCE_DIR}/extern/dtasm.git/runtime/dtasm3 dtasm3)
# switch off some warnings originating from external code
target_compile_options(dtasm3 PUBLIC -Wno-error=reorder -Wno-maybe-uninitialized -Wno-attributes -Wno-unused-variable)
add_dependencies(${COMPONENT_LIB} dtasm3)
target_link_libraries(${COMPONENT_LIB} PUBLIC dtasm3)
