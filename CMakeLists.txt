cmake_minimum_required(VERSION 2.8)

project(usertune-plugin)
set(PLUGIN_NAME usertune)

set(SOURCES usertune.cpp imetadatafetcher.cpp mprisfetcher1.cpp mprisfetcher2.cpp usertuneoptions.cpp usertunetypes.cpp)
set(HEADERS usertune.h imetadatafetcher.h mprisfetcher1.h mprisfetcher2.h usertuneoptions.h usertunetypes.h)
set(MOC_HEADERS usertune.h imetadatafetcher.h mprisfetcher1.h mprisfetcher2.h usertuneoptions.h)
set(UIS usertuneoptions.ui)

set(QT_USE_QTDBUS ON)
set(QT_USE_QTWEBKIT OFF)

include_directories("${CMAKE_SOURCE_DIR}" "${CMAKE_BINARY_DIR}")

include("${CMAKE_SOURCE_DIR}/cmake/config.cmake")
include("${CMAKE_SOURCE_DIR}/translations/languages.cmake")
include("${CMAKE_SOURCE_DIR}/cmake/plugin.cmake")

install(DIRECTORY "resources/menuicons" DESTINATION "${INSTALL_RESOURCES}")
