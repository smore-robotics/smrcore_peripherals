cmake_minimum_required(VERSION 3.18)

project(smrcore_peripherals_public_examples LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

option(SMRCORE_PERIPHERALS_BUILD_SDK_EXAMPLES
       "Build robot SDK bridge examples (requires smrcore_sdk)" ON)

find_package(smrcore_peripherals CONFIG REQUIRED)

if(TARGET smrcore::smrcore_peripherals)
    set(SMRC_PERIPHERALS_TARGET smrcore::smrcore_peripherals)
elseif(TARGET smrcore::peripherals)
    set(SMRC_PERIPHERALS_TARGET smrcore::peripherals)
else()
    message(FATAL_ERROR "smrcore_peripherals package did not export a known target")
endif()

function(add_peripheral_example name)
    add_executable(${name} examples/cpp/${name}.cpp)
    target_link_libraries(${name} PRIVATE ${SMRC_PERIPHERALS_TARGET})
endfunction()

add_peripheral_example(probe)
add_peripheral_example(read_spacemouse)
add_peripheral_example(read_spacemouse_raw)
add_peripheral_example(read_ft_sensor)

if(SMRCORE_PERIPHERALS_BUILD_SDK_EXAMPLES)
    find_package(smrcore_sdk CONFIG REQUIRED)

    if(TARGET smrcore::sdk)
        set(SMRC_SDK_TARGET smrcore::sdk)
    elseif(TARGET rcore::sdk)
        set(SMRC_SDK_TARGET rcore::sdk)
    else()
        message(FATAL_ERROR "smrcore_sdk package did not export a known target")
    endif()

    function(add_sdk_example name)
        add_executable(${name} examples/cpp/${name}.cpp)
        target_link_libraries(${name} PRIVATE ${SMRC_PERIPHERALS_TARGET} ${SMRC_SDK_TARGET})
    endfunction()

    add_sdk_example(peripherals_sdk)
endif()
