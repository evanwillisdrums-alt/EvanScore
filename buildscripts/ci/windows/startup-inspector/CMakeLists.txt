cmake_minimum_required(VERSION 3.28)
project(EvanScoreStartupInspector LANGUAGES CXX)
set(CMAKE_AUTOMOC ON)
find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick)
if(NOT TARGET Qt6::GuiPrivate)
    find_package(Qt6 REQUIRED COMPONENTS GuiPrivate)
endif()
add_library(evanscore-startup-inspector MODULE inspector.cpp)
target_link_libraries(evanscore-startup-inspector PRIVATE Qt6::Core Qt6::GuiPrivate Qt6::Quick)
set_target_properties(evanscore-startup-inspector PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins/platformthemes"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins/platformthemes")
