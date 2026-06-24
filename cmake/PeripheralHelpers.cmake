# SDK bridge helpers for smrcore_peripherals.

function(smr_peripheral_resolve_sdk_target out_var)
    if(TARGET rcoresdk)
        set(_sdk_target rcoresdk)
    elseif(TARGET smrcore::sdk)
        set(_sdk_target smrcore::sdk)
    elseif(TARGET rcore::sdk)
        set(_sdk_target rcore::sdk)
    else()
        if(SMR_PERIPHERAL_SDK_ROOT)
            list(PREPEND CMAKE_PREFIX_PATH "${SMR_PERIPHERAL_SDK_ROOT}")
        endif()
        find_package(smrcore_sdk CONFIG REQUIRED)

        if(TARGET smrcore::sdk)
            set(_sdk_target smrcore::sdk)
        elseif(TARGET rcoresdk)
            set(_sdk_target rcoresdk)
        elseif(TARGET rcore::sdk)
            set(_sdk_target rcore::sdk)
        else()
            message(FATAL_ERROR
                "find_package(smrcore_sdk) succeeded but no supported SDK target "
                "was exported (expected smrcore::sdk, rcoresdk, or rcore::sdk).")
        endif()
    endif()

    if(NOT _sdk_target)
        message(FATAL_ERROR
            "SMR_PERIPHERAL_WITH_SDK=ON requires an embedded rcore SDK target or "
            "a standalone smrcore_sdk package. Run scripts/download.sh or pass "
            "-DSMR_PERIPHERAL_SDK_ROOT=<smrcore_sdk install prefix>.")
    endif()

    set(${out_var} ${_sdk_target} PARENT_SCOPE)
endfunction()

function(smr_target_enable_sdk_bridge target_name)
    if(SMR_PERIPHERAL_WITH_SDK)
        if(NOT SMR_PERIPHERAL_SDK_TARGET)
            message(FATAL_ERROR
                "smr_target_enable_sdk_bridge: SMR_PERIPHERAL_SDK_TARGET is not set")
        endif()
        target_compile_definitions(${target_name} PRIVATE SMR_PERIPHERAL_WITH_SDK=1)
        target_link_libraries(${target_name} PRIVATE ${SMR_PERIPHERAL_SDK_TARGET})
    endif()
endfunction()
