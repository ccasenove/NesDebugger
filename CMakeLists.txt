cmake_minimum_required(VERSION 3.10)

project(NesDebugger C)

find_program(GLIB_COMPILE_RESOURCES NAMES glib-compile-resources REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

include_directories(${GTK3_INCLUDE_DIRS})
link_directories(${GTK3_LIBRARY_DIRS})

add_definitions(${GTK3_CFLAGS_OTHER})

add_custom_command(
    OUTPUT ${PROJECT_BINARY_DIR}/resources.c
    COMMAND ${GLIB_COMPILE_RESOURCES} 
    ARGS --target=${PROJECT_BINARY_DIR}/resources.c --generate-source debuggerapp.gresource.xml
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    DEPENDS ${PROJECT_SOURCE_DIR}/window.xml ${PROJECT_SOURCE_DIR}/ppu_registers_window.xml 
        ${PROJECT_SOURCE_DIR}/memory_window.xml ${PROJECT_SOURCE_DIR}/ppu_tables_window.xml 
        ${PROJECT_SOURCE_DIR}/breakpoint_window.xml ${PROJECT_SOURCE_DIR}/system_palette_window.xml
        ${PROJECT_SOURCE_DIR}/disassembler_window.xml ${PROJECT_SOURCE_DIR}/oam_window.xml
    COMMENT "Building GTK resources file..."
)

file (GLOB SOURCES "*.c")
add_executable(${PROJECT_NAME} ${SOURCES} ${PROJECT_BINARY_DIR}/resources.c)

target_link_libraries(NesDebugger ${GTK3_LIBRARIES})