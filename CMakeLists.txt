cmake_minimum_required(VERSION 3.10)
project(g300-autoled-switcher)
add_executable(g300-autoled-switcher main.c)

find_package (hidapi REQUIRED)
target_link_libraries (g300-autoled-switcher PRIVATE hidapi::hidapi)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBUSB REQUIRED libusb-1.0)
target_include_directories(g300-autoled-switcher PRIVATE ${LIBUSB_INCLUDE_DIRS})
target_link_libraries(g300-autoled-switcher PRIVATE ${LIBUSB_LIBRARIES})
