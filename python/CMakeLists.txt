cmake_minimum_required(VERSION 3.20)
project(rcore_peripherals_py LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(PYBIND11_FINDPYTHON ON)
find_package(pybind11 CONFIG REQUIRED)

set(SMRCORE_PERIPHERALS_CPP_ROOT "$ENV{SMRCORE_PERIPHERALS_CPP_ROOT}" CACHE PATH
    "Installed smrcore_peripherals root")

if(TARGET smrcore_peripherals)
    set(SMRCORE_PERIPHERALS_USE_TREE TRUE)
    set(SMRCORE_PERIPHERALS_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../src")
    set(SMRCORE_PERIPHERALS_LIBRARY_DIR "${CMAKE_BINARY_DIR}/../../lib")
elseif(SMRCORE_PERIPHERALS_CPP_ROOT)
    find_path(SMRCORE_PERIPHERALS_INCLUDE_DIR
        NAMES peripherals/peripherals.hpp
        PATHS "${SMRCORE_PERIPHERALS_CPP_ROOT}/include"
        NO_DEFAULT_PATH
    )

    find_library(SMRCORE_PERIPHERALS_LIBRARY
        NAMES smrcore_peripherals
        PATHS "${SMRCORE_PERIPHERALS_CPP_ROOT}/lib" "${SMRCORE_PERIPHERALS_CPP_ROOT}/bin"
        NO_DEFAULT_PATH
    )

    if(NOT SMRCORE_PERIPHERALS_INCLUDE_DIR OR NOT SMRCORE_PERIPHERALS_LIBRARY)
        message(FATAL_ERROR
            "Unable to find smrcore_peripherals headers/library under "
            "SMRCORE_PERIPHERALS_CPP_ROOT=${SMRCORE_PERIPHERALS_CPP_ROOT}")
    endif()

    get_filename_component(SMRCORE_PERIPHERALS_LIBRARY_DIR
        "${SMRCORE_PERIPHERALS_LIBRARY}" DIRECTORY)
else()
    message(FATAL_ERROR
        "SMRCORE_PERIPHERALS_CPP_ROOT is required. Build and install C++ first "
        "(./scripts/build.sh), then set SMRCORE_PERIPHERALS_CPP_ROOT to build_<Type>/install.")
endif()

if(NOT DEFINED SMRCORE_PERIPHERALS_VERSION)
    if(DEFINED ENV{SMRCORE_PERIPHERALS_VERSION})
        set(SMRCORE_PERIPHERALS_VERSION "$ENV{SMRCORE_PERIPHERALS_VERSION}")
    else()
        set(SMRCORE_PERIPHERALS_VERSION "0.0.0")
    endif()
endif()

set(SMRCORE_PERIPHERALS_PY_RUNTIME_LIBS "")
if(WIN32)
    file(GLOB SMRCORE_PERIPHERALS_PY_RUNTIME_LIBS
        "${SMRCORE_PERIPHERALS_CPP_ROOT}/bin/*.dll"
        "${SMRCORE_PERIPHERALS_CPP_ROOT}/lib/*.dll"
    )
elseif(SMRCORE_PERIPHERALS_CPP_ROOT)
    file(GLOB SMRCORE_PERIPHERALS_PY_RUNTIME_LIBS
        "${SMRCORE_PERIPHERALS_CPP_ROOT}/lib/libsmrcore_peripherals.so"
        "${SMRCORE_PERIPHERALS_CPP_ROOT}/lib/libsmrcore_peripherals.so.*"
    )
endif()

pybind11_add_module(_native native/bindings.cpp)

target_compile_definitions(_native PRIVATE
    SMRCORE_PERIPHERALS_VERSION_STR="${SMRCORE_PERIPHERALS_VERSION}"
)

if(SMRCORE_PERIPHERALS_USE_TREE)
    target_link_libraries(_native PRIVATE smrcore_peripherals)
else()
    target_link_libraries(_native PRIVATE "${SMRCORE_PERIPHERALS_LIBRARY}")
endif()

target_include_directories(_native
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/native
        ${SMRCORE_PERIPHERALS_INCLUDE_DIR}
)

set_target_properties(_native PROPERTIES
    BUILD_RPATH "${SMRCORE_PERIPHERALS_LIBRARY_DIR}"
    INSTALL_RPATH "$ORIGIN/.libs;$ORIGIN"
)

install(TARGETS _native LIBRARY DESTINATION rcore_peripherals)
if(SMRCORE_PERIPHERALS_PY_RUNTIME_LIBS)
    install(FILES ${SMRCORE_PERIPHERALS_PY_RUNTIME_LIBS}
        DESTINATION rcore_peripherals/.libs
    )
endif()
