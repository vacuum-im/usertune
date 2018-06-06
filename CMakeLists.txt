cmake_minimum_required(VERSION 3.2)

project(usertune-plugin)
set(PLUGIN_NAME "usertune")

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(SOURCES usertune.cpp imetadatafetcher.cpp mprisfetcher1.cpp mprisfetcher2.cpp usertuneoptions.cpp usertunetypes.cpp)
set(HEADERS usertune.h imetadatafetcher.h mprisfetcher1.h mprisfetcher2.h usertuneoptions.h usertunetypes.h)
set(MOC_HEADERS usertune.h imetadatafetcher.h mprisfetcher1.h mprisfetcher2.h usertuneoptions.h)
set(UIS usertuneoptions.ui)

include_directories("${CMAKE_SOURCE_DIR}" "${CMAKE_BINARY_DIR}")

include("${CMAKE_SOURCE_DIR}/cmake/config.cmake")
include("${CMAKE_SOURCE_DIR}/translations/languages.cmake")
include("${CMAKE_SOURCE_DIR}/cmake/plugin.cmake")

find_package(Qt5Xml REQUIRED)
target_link_libraries(${PLUGIN_NAME} Qt5::Xml)
if (UNIX)
    find_package(Qt5DBus REQUIRED)
    find_package(Qt5Widgets REQUIRED)
    target_link_libraries(${PLUGIN_NAME} Qt5::DBus Qt5::Widgets)
endif (UNIX)

install(DIRECTORY "resources/menuicons" DESTINATION "${INSTALL_RESOURCES}")
