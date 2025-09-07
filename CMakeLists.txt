cmake_minimum_required(VERSION 3.10)
project(ColmapEditor)

set(CMAKE_CXX_STANDARD 17)

find_package(wxWidgets REQUIRED COMPONENTS core base gl)
find_package(OpenSceneGraph REQUIRED osgViewer osgGA osgUtil osgDB osg)
find_package(OpenGL REQUIRED)

include(${wxWidgets_USE_FILE})
include_directories(${OPENSCENEGRAPH_INCLUDE_DIRS})

file(GLOB SRC_FILES src/*.cpp src/*.h)

add_executable(ColmapEditor WIN32 ${SRC_FILES})

target_link_libraries(ColmapEditor ${wxWidgets_LIBRARIES} ${OPENSCENEGRAPH_LIBRARIES} ${OPENGL_LIBRARIES})
