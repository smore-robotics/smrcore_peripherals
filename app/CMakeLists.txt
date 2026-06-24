function(add_peripheral_app name)
    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE smrcore_peripherals)
    install(TARGETS ${name} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endfunction()

add_peripheral_app(app_peripherals_probe)

add_peripheral_app(app_peripherals_read_spacemouse)
add_peripheral_app(app_peripherals_read_spacemouse_raw)
add_peripheral_app(app_peripherals_read_ft_sensor)

add_peripheral_app(app_peripherals_bridge)
smr_target_enable_sdk_bridge(app_peripherals_bridge)
