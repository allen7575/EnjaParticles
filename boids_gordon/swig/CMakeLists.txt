cmake_minimum_required(VERSION 2.8)
SET (MODULE_NAME flock)

FIND_PACKAGE(SWIG REQUIRED)
INCLUDE(${SWIG_USE_FILE})

FIND_PACKAGE(PythonLibs)
INCLUDE_DIRECTORIES(${PYTHON_INCLUDE_PATH})

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

#SET(CMAKE_SWIG_FLAGS "")

SET_SOURCE_FILES_PROPERTIES(${MODULE_NAME}.i PROPERTIES CPLUSPLUS ON)
#SET_SOURCE_FILES_PROPERTIES(${MODULE_NAME}.i PROPERTIES SWIG_FLAGS "-includeall")
SWIG_ADD_MODULE(${MODULE_NAME} python flock.i ../boids.cpp ../structs.cpp ../domain/IV.cpp)
SWIG_LINK_LIBRARIES(${MODULE_NAME} ${PYTHON_LIBRARIES})

# Keep our demo script with it
FILE(COPY example_boids.py DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
FILE(COPY glboids.py DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
