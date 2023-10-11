target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
FILE(GLOB app_sources *.c)
target_sources(app PRIVATE ${app_sources})