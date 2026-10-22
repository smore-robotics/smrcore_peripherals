cmake_minimum_required(VERSION 3.16)

set(SMRCORE_PERIPHERALS_VERSION "0.0.0" CACHE STRING "smrcore_peripherals semantic version")
if(NOT SMRCORE_PERIPHERALS_VERSION MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+$")
    message(FATAL_ERROR "SMRCORE_PERIPHERALS_VERSION must be MAJOR.MINOR.PATCH, got: ${SMRCORE_PERIPHERALS_VERSION}")
endif()

project(smrcore_peripherals VERSION ${SMRCORE_PERIPHERALS_VERSION} LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(CTest)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/PeripheralHelpers.cmake)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
if(CMAKE_CONFIGURATION_TYPES)
    foreach(_cfg ${CMAKE_CONFIGURATION_TYPES})
        string(TOUPPER "${_cfg}" _cfg_upper)
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${_cfg_upper} "${CMAKE_BINARY_DIR}/bin")
        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${_cfg_upper} "${CMAKE_BINARY_DIR}/lib")
        set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${_cfg_upper} "${CMAKE_BINARY_DIR}/lib")
    endforeach()
endif()

option(BUILD_TESTS "Build unit tests" ON)
option(SMR_PERIPHERAL_BUILD_APPS "Build peripheral tools" ON)
option(SMR_PERIPHERAL_BUILD_TESTS "Build peripheral tests" ON)
if(NOT BUILD_TESTS)
    set(SMR_PERIPHERAL_BUILD_TESTS OFF CACHE BOOL "" FORCE)
endif()
option(SMR_PERIPHERAL_BUILD_PYTHON "Build Python bindings" OFF)
option(SMR_PERIPHERAL_WITH_SDK "Build robot SDK bridge and F/T calibration tools" OFF)
option(SMR_PERIPHERAL_WITH_PICO_TELEOP "Build optional SDK-backed dual-arm Pico teleoperation" OFF)
if(SMR_PERIPHERAL_WITH_PICO_TELEOP AND NOT SMR_PERIPHERAL_WITH_SDK)
    message(FATAL_ERROR "Pico teleoperation requires SMR_PERIPHERAL_WITH_SDK=ON")
endif()

set(SMR_PERIPHERAL_SDK_ROOT "" CACHE PATH "smrcore_sdk install prefix for standalone bridge builds")

set(SMR_PERIPHERAL_SDK_TARGET "")
if(SMR_PERIPHERAL_WITH_SDK)
    smr_peripheral_resolve_sdk_target(SMR_PERIPHERAL_SDK_TARGET)
endif()

file(GLOB_RECURSE SMR_PERIPHERAL_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

add_library(smrcore_peripherals ${SMR_PERIPHERAL_SOURCES})
add_library(smrcore::peripherals ALIAS smrcore_peripherals)
set_target_properties(smrcore_peripherals PROPERTIES EXPORT_NAME peripherals)

target_compile_features(smrcore_peripherals PUBLIC cxx_std_17)
target_include_directories(smrcore_peripherals
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
find_package(Threads REQUIRED)
find_package(nlohmann_json REQUIRED)
target_link_libraries(smrcore_peripherals
    PRIVATE
        Threads::Threads
        nlohmann_json::nlohmann_json
)

install(TARGETS smrcore_peripherals
    EXPORT smrcore_peripheralsTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(
    DIRECTORY src/peripherals/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/peripherals
    FILES_MATCHING PATTERN "*.hpp"
    PATTERN "sample_model.hpp" EXCLUDE
)

install(EXPORT smrcore_peripheralsTargets
    NAMESPACE smrcore::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/smrcore_peripherals
)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/smrcore_peripheralsConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMinorVersion
)
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/smrcore_peripheralsConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/smrcore_peripheralsConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/smrcore_peripherals
)
install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/smrcore_peripheralsConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/smrcore_peripheralsConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/smrcore_peripherals
)

if(SMR_PERIPHERAL_WITH_PICO_TELEOP)
    get_target_property(_pico_sdk_imported ${SMR_PERIPHERAL_SDK_TARGET} IMPORTED)
    if(_pico_sdk_imported)
        include(CheckCXXSourceCompiles)
        set(CMAKE_REQUIRED_LIBRARIES ${SMR_PERIPHERAL_SDK_TARGET})
        unset(SMR_PERIPHERAL_HAS_DUAL_ARM_SDK CACHE)
        check_cxx_source_compiles("#include <sdk/dual_arm_robot.hpp>
int main() { return 0; }"
            SMR_PERIPHERAL_HAS_DUAL_ARM_SDK)
        unset(CMAKE_REQUIRED_LIBRARIES)
        if(NOT SMR_PERIPHERAL_HAS_DUAL_ARM_SDK)
            message(FATAL_ERROR "Pico teleoperation needs a matching dual-arm SDK (sdk/dual_arm_robot.hpp). Set SMR_PERIPHERAL_SDK_ROOT to a compatible installation, or disable SMR_PERIPHERAL_WITH_PICO_TELEOP to build the original readers/bridges.")
        endif()
    endif()
    find_package(Eigen3 REQUIRED)
    add_library(smrcore_pico_teleop SHARED teleop/pico_teleop.cpp)
    target_compile_features(smrcore_pico_teleop PUBLIC cxx_std_17)
    target_link_libraries(smrcore_pico_teleop PRIVATE smrcore_peripherals Eigen3::Eigen nlohmann_json::nlohmann_json ${SMR_PERIPHERAL_SDK_TARGET})
    target_include_directories(smrcore_pico_teleop PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
    set_target_properties(smrcore_pico_teleop PROPERTIES INSTALL_RPATH "$ORIGIN" EXPORT_NAME pico_teleop)
    install(TARGETS smrcore_pico_teleop EXPORT smrcore_peripheralsTargets LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()

if(SMR_PERIPHERAL_BUILD_APPS)
    add_subdirectory(app)
endif()
if(BUILD_TESTING AND SMR_PERIPHERAL_BUILD_TESTS)
    add_subdirectory(unittests)
endif()
if(SMR_PERIPHERAL_BUILD_PYTHON)
    add_subdirectory(python)
endif()
