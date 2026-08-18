function(add_peripheral_app name)
    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE smrcore_peripherals)
    install(TARGETS ${name} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endfunction()

add_peripheral_app(app_peripherals_probe)

add_peripheral_app(app_peripherals_read_spacemouse)
add_peripheral_app(app_peripherals_read_spacemouse_raw)
add_peripheral_app(app_peripherals_read_ft_sensor)
add_peripheral_app(app_peripherals_read_pico)

if(SMR_PERIPHERAL_WITH_SDK)
    add_peripheral_app(app_peripherals_bridge)
    smr_target_enable_sdk_bridge(app_peripherals_bridge)

    add_peripheral_app(app_peripherals_ft_sensor_calib)
    smr_target_enable_sdk_bridge(app_peripherals_ft_sensor_calib)

    find_package(Eigen3 REQUIRED)
    add_peripheral_app(app_peripherals_pico_bridge)
    target_link_libraries(app_peripherals_pico_bridge PRIVATE Eigen3::Eigen)
    smr_target_enable_sdk_bridge(app_peripherals_pico_bridge)
endif()
