PROJECT(LRView)

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

ENABLE_LANGUAGE(CXX)

INCLUDE(CheckCXXCompilerFlag)

IF(CMAKE_CXX_COMPILER_ID MATCHES GNU)
# check if compiler supports c++-0x
  CHECK_CXX_COMPILER_FLAG("-std=gnu++0x" HAVE_0x)
  IF(HAVE_0x)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++0x")
  ELSE(HAVE_0x)
    MESSAGE("A compiler with c++-0x support is needed")
    EXIT(1)
  ENDIF(HAVE_0x)
ENDIF(CMAKE_CXX_COMPILER_ID MATCHES GNU)

# Add local modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
                      ${CMAKE_SOURCE_DIR}/cmake/Modules)

# Required packages
MESSAGE("Lookin for packages")
FIND_PACKAGE(LRSpline REQUIRED)
FIND_PACKAGE(GoTools REQUIRED)
FIND_PACKAGE(GoTrivariate REQUIRED)
FIND_PACKAGE(OpenGL REQUIRED)
FIND_PACKAGE(GLUT REQUIRED)
FIND_PACKAGE(Boost REQUIRED)

# Required libraries
SET(DEPLIBS
  ${LRSpline_LIBRARIES}
  ${GoTools_LIBRARIES}
  ${GoTrivariate_LIBRARIES}
  ${GLUT_glut_LIBRARY}
  ${OPENGL_gl_LIBRARY} 
  ${OPENGL_glu_LIBRARY}
  ${BOOST_LIBRARIES}
)

# Required include directories
SET(INCLUDES
  ${PROJECT_SOURCE_DIR}/include
  ${GoTools_INCLUDE_DIRS} 
  ${GoTrivariate_INCLUDE_DIRS}
  ${LRSpline_INCLUDE_DIRS} 
  ${GLUT_INCLUDE_DIR}
  ${GLU_INCLUDE_PATH}
  ${BOOST_INCLUDES}
)

INCLUDE_DIRECTORIES(${INCLUDES})

SET(EXECUTABLE_OUTPUT_PATH bin)
SET(LIBRARY_OUTPUT_PATH lib)

IF(NOT WIN32)
  # Emit position-independent code, suitable for dynamic linking
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
  # Enable all warnings
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-parentheses")
ENDIF(NOT WIN32)

# Make some Apps
FILE(GLOB_RECURSE ALL_SRCS ${PROJECT_SOURCE_DIR}/src/*.cpp)
ADD_EXECUTABLE(ViewLR ${ALL_SRCS})
TARGET_LINK_LIBRARIES(ViewLR ${DEPLIBS})

# 'install' target
IF(WIN32)
  # TODO
ELSE(WIN32)
  # lib
ENDIF(WIN32)

