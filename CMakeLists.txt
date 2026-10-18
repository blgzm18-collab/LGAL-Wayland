cmake_minimum_required(VERSION 3.20)
project(lgal VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Per-pixel software rendering is very slow in an unoptimised build
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(LGAL_BUILD_EXAMPLES "Build the sandbox example" ON)

include(GNUInstallDirs)

# -----------------------------------------------------------------------------
# 1. Helper: generate client header + glue code from a protocol XML
# -----------------------------------------------------------------------------
find_program(WAYLAND_SCANNER_EXECUTABLE wayland-scanner REQUIRED)

function(generate_wayland_protocol xml_file output_var)
    if(NOT EXISTS "${xml_file}")
        message(FATAL_ERROR "Protocol file not found: ${xml_file} (install wayland-protocols)")
    endif()

    get_filename_component(basename ${xml_file} NAME_WE)

    set(header_file "${CMAKE_CURRENT_BINARY_DIR}/generated/${basename}-client-protocol.h")
    set(source_file "${CMAKE_CURRENT_BINARY_DIR}/generated/${basename}-protocol.c")

    add_custom_command(
        OUTPUT "${header_file}" "${source_file}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/generated"
        COMMAND ${WAYLAND_SCANNER_EXECUTABLE} client-header "${xml_file}" "${header_file}"
        COMMAND ${WAYLAND_SCANNER_EXECUTABLE} private-code "${xml_file}" "${source_file}"
        DEPENDS "${xml_file}"
        COMMENT "Generating Wayland code for ${basename}"
        VERBATIM
    )

    set(${output_var} "${header_file}" "${source_file}" PARENT_SCOPE)
    set(${output_var}_HEADER "${header_file}" PARENT_SCOPE)
endfunction()

# -----------------------------------------------------------------------------
# 2. Dependencies & protocol generation
# -----------------------------------------------------------------------------
find_package(PkgConfig REQUIRED)
pkg_check_modules(WAYLAND_CLIENT REQUIRED IMPORTED_TARGET wayland-client)
pkg_check_modules(WAYLAND_PROTOCOLS REQUIRED wayland-protocols)

pkg_get_variable(WAYLAND_PROTOCOLS_BASE wayland-protocols pkgdatadir)

set(XDG_SHELL_XML "${WAYLAND_PROTOCOLS_BASE}/stable/xdg-shell/xdg-shell.xml")
generate_wayland_protocol("${XDG_SHELL_XML}" XDG_SHELL_SOURCES)

set(XDG_DECOR_XML "${WAYLAND_PROTOCOLS_BASE}/unstable/xdg-decoration/xdg-decoration-unstable-v1.xml")
generate_wayland_protocol("${XDG_DECOR_XML}" XDG_DECOR_SOURCES)

# -----------------------------------------------------------------------------
# 3. Library
# -----------------------------------------------------------------------------
# CONFIGURE_DEPENDS re-runs the glob when you add or remove a source file
file(GLOB_RECURSE LGAL_SOURCES CONFIGURE_DEPENDS "src/*.cpp" "src/*.c")

add_library(lgal SHARED ${LGAL_SOURCES} ${XDG_SHELL_SOURCES} ${XDG_DECOR_SOURCES})

set_target_properties(lgal PROPERTIES
    VERSION   ${PROJECT_VERSION}
    SOVERSION 0
)

target_include_directories(lgal
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/lgal>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# PUBLIC: our headers include wayland-client.h
target_link_libraries(lgal PUBLIC PkgConfig::WAYLAND_CLIENT)

# Wayland listener tables intentionally leave newer callbacks unset
target_compile_options(lgal PRIVATE
    "$<$<COMPILE_LANGUAGE:CXX>:-Wall>"
    "$<$<COMPILE_LANGUAGE:CXX>:-Wextra>"
    "$<$<COMPILE_LANGUAGE:CXX>:-Wno-missing-field-initializers>")

# -----------------------------------------------------------------------------
# 4. Sandbox executable
# -----------------------------------------------------------------------------
if(LGAL_BUILD_EXAMPLES AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples/main.cpp")
    add_executable(lgal_sandbox examples/main.cpp)
    target_link_libraries(lgal_sandbox PRIVATE lgal)
    target_compile_options(lgal_sandbox PRIVATE -Wall -Wextra)

    add_custom_target(run
        COMMAND lgal_sandbox
        DEPENDS lgal_sandbox
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
endif()

# -----------------------------------------------------------------------------
# 5. Install
# -----------------------------------------------------------------------------
install(TARGETS lgal LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/lgal
        FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp")
# lgal_window.hpp includes these generated headers
install(FILES ${XDG_SHELL_SOURCES_HEADER} ${XDG_DECOR_SOURCES_HEADER}
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/lgal)

# Input demo (optional)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples/input_demo.cpp")
    add_executable(lgal_input_demo examples/input_demo.cpp)
    target_link_libraries(lgal_input_demo PRIVATE lgal)
    target_compile_options(lgal_input_demo PRIVATE -Wall -Wextra)
endif()