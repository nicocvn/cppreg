# -------------------------------------------------------------------------- #
# cppreg library CMake script
#
# Nicolas Clauvelin (nclauvelin@sendyne.com)
# Sendyne Corp., 2017
#
# -------------------------------------------------------------------------- #


# --- cppreg library ---

# Header directories.
set(cppreg_headers_dirs
    .
    policies/
    register/)

# List of API headers.
set(cppreg_headers
    cppreg.h
    cppreg_Defines.h
    cppreg_Includes.h
    policies/AccessPolicy.h
    register/Field.h
    register/Internals.h
    register/Mask.h
    register/Memory.h
    register/MergeWrite.h
    register/Register.h
    register/RegisterPack.h
    register/ShadowValue.h
    register/Traits.h)

# Refactor headers directories.
set(build_cppreg_headers_dirs "")
foreach(dir IN ITEMS ${cppreg_headers_dirs})
    list(APPEND build_cppreg_headers_dirs "${CMAKE_CURRENT_SOURCE_DIR}/${dir}")
endforeach()

# Header-only library.
add_library(cppreg INTERFACE)
target_include_directories(cppreg INTERFACE ${cppreg_headers_dirs})

# Include directories interfaces.
# This properly setup the API headers for the build and install phase.
set_property(TARGET cppreg
             PROPERTY INTERFACE_INCLUDE_DIRECTORIES
             $<BUILD_INTERFACE:${build_cppreg_headers_dirs}>
             $<INSTALL_INTERFACE:include/cppreg>)


# --- Install directives ---

install(FILES ${cppreg_headers} DESTINATION include/cppreg)
install(TARGETS cppreg DESTINATION lib EXPORT cppreg-target)
install(EXPORT cppreg-target DESTINATION include/cppreg)
