# -------------------------------------------------------------------------- #
# cppreg library CMake script
#
# Nicolas Clauvelin (nclauvelin@sendyne.com)
# Sendyne Corp., 2017
#
# -------------------------------------------------------------------------- #


# --- cppreg library files ---

# Header directories.
set(CPPREG_HEADERS_DIRS
    .
    policies/
    register/)

# List of API headers.
set(CPPREG_API_HEADERS
    cppreg.h
    cppreg_Defines.h
    cppreg_Includes.h
    policies/AccessPolicy.h
    register/Field.h
    register/Mask.h
    register/Overflow.h
    register/Register.h
    register/RegisterPack.h
    register/ShadowValue.h
    register/Traits.h)

# Refactor headers directories.
set(BUILD_HEADERS_DIRS "")
foreach(dir IN ITEMS ${CPPREG_HEADERS_DIRS})
    list(APPEND BUILD_HEADERS_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/${dir}")
endforeach()

# Header-only library.
add_library(cppreg INTERFACE)
target_include_directories(cppreg INTERFACE . register/ policies/)

# Include directories interfaces.
# This properly setup the API headers for the build and install phase.
set_property(TARGET cppreg
             PROPERTY INTERFACE_INCLUDE_DIRECTORIES
             $<BUILD_INTERFACE:${BUILD_HEADERS_DIRS}>
             $<INSTALL_INTERFACE:include/cppreg>)


# --- Install directives ---

install(FILES ${CPPREG_API_HEADERS} DESTINATION include/cppreg)
install(TARGETS cppreg DESTINATION lib EXPORT cppreg-libs)
install(EXPORT cppreg-libs DESTINATION include/cppreg)
