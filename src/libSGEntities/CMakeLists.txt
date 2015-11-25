cmake_minimum_required(VERSION 2.8.11)

find_package(OpenSceneGraph REQUIRED COMPONENTS osgDB osgGA osgUtil osgViewer)
include_directories(${OPENSCENEGRAPH_INCLUDE_DIRS})

include_directories(
    ${CMAKE_SOURCE_DIR}/libGUI
    ${CMAKE_SOURCE_DIR}/dureu3d
    ${CMAKE_SOURCE_DIR}/libSGUtils
)

set (SOURCES_SGEntities
    canvas.h
    canvas.cpp
    axes.h
    axes.cpp
    rootscene.h
    rootscene.cpp
    stroke.h
    stroke.cpp
)

add_library(libSGEntities
    ${SOURCES_SGEntities}
)

target_link_libraries( libSGEntities
  ${OPENSCENEGRAPH_LIBRARIES}
)