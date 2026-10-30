# Please see README.md for build instructions

cmake_minimum_required(VERSION 3.16.4)

set(CMAKE_SYSTEM_NAME Generic)

set(CMAKE_SYSTEM_PROCESSOR arm)

# Use add_library() with the STATIC option to name the source file in the generated project.
# This avoids running the linker and is intended for use with cross-compiling toolchains that
# cannot link without custom flags or linker scripts
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_CONFIGURATION_TYPES Debug Release RelWithDebInfo MinSizeRel)

if(BUILD_KIT)
	message(NOTICE "Build Kit is defined as ${BUILD_KIT}")
endif()

message(NOTICE "CMAKE_CURRENT_BINARY_DIR is defined as ${CMAKE_CURRENT_BINARY_DIR}")

get_property(isMultiConfig GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)

if(isMultiConfig)
    message(NOTICE "Multi-config generator(${CMAKE_GENERATOR}) is active")
else()
    message(NOTICE "Single-config generator(${CMAKE_GENERATOR}) is active")
	if(DEFINED CMAKE_BUILD_TYPE)
	 	message(NOTICE "CMAKE_BUILD_TYPE is defined as ${CMAKE_BUILD_TYPE}")
	elseif(DEFINED BUILD_TYPE)
		set(CMAKE_BUILD_TYPE ${BUILD_TYPE})
		message(NOTICE "CMAKE_BUILD_TYPE is set to BUILD_TYPE ENV ${CMAKE_BUILD_TYPE}")
	else()
		message(FATAL_ERROR "Please specify the CMAKE_BUILD_TYPE with -DCMAKE_BUILD_TYPE=")
	endif()
    #set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE})
	message(NOTICE "CMAKE_RUNTIME_OUTPUT_DIRECTORY is defined as ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
endif()

cmake_minimum_required(VERSION 3.16.4)

# Project configuration
project(nfc_doorlock_fpb_ra2e3_ep
	VERSION 1.0.0
	LANGUAGES C CXX ASM)

set(CMAKE_CROSSCOMPILING "TRUE")
set(CMAKE_EXPORT_COMPILE_COMMANDS "TRUE")

include(${CMAKE_CURRENT_LIST_DIR}/Config.cmake)

include(${CMAKE_CURRENT_LIST_DIR}/cmake/GeneratedCfg.cmake)

# If required user can amend RASC generated flags here, e.g. list(REMOVE_ITEM RASC_CMAKE_C_FLAGS "-Wall")

include(${CMAKE_CURRENT_LIST_DIR}/cmake/GeneratedSrc.cmake)

# --------------------------------------------------------------------------
# User (application) additions - PES NFC card reader
# --------------------------------------------------------------------------
# All sources under src/ are already compiled via the GLOB_RECURSE in
# cmake/GeneratedSrc.cmake; only the header sub-folders need adding.
target_include_directories(${PROJECT_NAME}.elf
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rs-nfc-card-reader
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rs-nfc-card-reader/ptx105r
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rs-console-io
)

# picolibc/tinystdio links the float-capable vfprintf by default, which does
# not fit in 64 KB flash. The application only uses integer/string
# conversions, so redirect to the integer-only (long) variant.
target_link_options(${PROJECT_NAME}.elf
    PRIVATE
    -Wl,--defsym=vfprintf=__l_vfprintf
)

# Vendor code (PTX NFC SDK etc.) does not survive RASC's strict warning flags;
# silence it the same way GeneratedSrc.cmake treats COMPANION_FILES, so that
# warnings from first-party src/ stay visible.
file(GLOB_RECURSE Vendor_Source_Files
    ${CMAKE_CURRENT_SOURCE_DIR}/ra/renesas/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ra/renesas/*.cpp)
if(Vendor_Source_Files)
    set_source_files_properties(${Vendor_Source_Files} PROPERTIES COMPILE_FLAGS "-w")
endif()

