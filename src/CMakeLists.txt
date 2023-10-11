add_subdirectory(adc)
add_subdirectory(device_info)
add_subdirectory(bluetooth)

target_sources(app PRIVATE main.c)