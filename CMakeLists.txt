cmake_minimum_required(VERSION 3.5)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
add_compile_options("-Wno-format")

set(EXTRA_COMPONENT_DIRS managed_components)

project(home_monitoring)
