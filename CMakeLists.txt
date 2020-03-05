CMAKE_MINIMUM_REQUIRED(VERSION 3.12.4)
PROJECT(snmp_stream)

INCLUDE(ExternalProject)
INCLUDE(ProcessorCount)

SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY  ${CMAKE_BINARY_DIR}/lib)
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY  ${CMAKE_BINARY_DIR}/lib)
SET(CMAKE_CXX_STANDARD 17)
SET(CMAKE_EXPORT_COMPILE_COMMANDS ON)
SET(PYBIND11_CPP_STANDARD -std=c++17)

# add additional cmake commands
SET(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

# add vendor libraries
INCLUDE_DIRECTORIES(SYSTEM vendor/boost)
INCLUDE_DIRECTORIES(SYSTEM vendor/soq/src/libsoq)
INCLUDE_DIRECTORIES(SYSTEM ${CMAKE_BINARY_DIR}/include)

# allow debug symbols to be set via environment, poetry as of now cannot pass the flag
IF(DEFINED ENV{SNMP_STREAM_DEBUG})
  MESSAGE(STATUS "Debug mode enabled")
  ADD_DEFINITIONS(-DDEBUG)
  SET(CMAKE_BUILD_TYPE Debug)
ENDIF()

# enable parallel builds
ProcessorCount(N)
if(NOT N EQUAL 0)
  SET(GIT_CONFIG submodule.fetchJobs=${N})
  SET(MAKEFLAGS -j${N})
endif()

# net-snmp
ExternalProject_Add(netsnmp_external
  GIT_REPOSITORY https://git.code.sf.net/p/net-snmp/code
  GIT_TAG v5.8
  GIT_SHALLOW 1
  GIT_CONFIG ${GIT_CONFIG}
  CMAKE_ARGS
    -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
  CONFIGURE_COMMAND ./configure --prefix=${CMAKE_BINARY_DIR} --with-defaults --enable-ipv6 --disable-agent --disable-applications --disable-manuals --disable-scripts --disable-mibs --disable-mib-loading --disable-debugging --disable-embedded-perl --without-perl-modules --enable-static --disable-shared --with-pic --with-ldflags=-Bstatic
  BUILD_COMMAND ${CMAKE_MAKE_PROGRAM} snmplib
  INSTALL_COMMAND ${CMAKE_MAKE_PROGRAM} installlocalheaders && cd snmplib && ${CMAKE_MAKE_PROGRAM} install
  BUILD_IN_SOURCE 1
  UPDATE_COMMAND ""
)

ADD_LIBRARY(netsnmp STATIC IMPORTED)
SET_TARGET_PROPERTIES(netsnmp PROPERTIES
  POSITION_INDEPENDENT_CODE ON
  IMPORTED_LOCATION ${CMAKE_BINARY_DIR}/lib/libnetsnmp${CMAKE_STATIC_LIBRARY_SUFFIX}
)

# debug library
ADD_LIBRARY(soq STATIC
  vendor/soq/src/libsoq/debug.c
  vendor/soq/src/libsoq/kludge.c
  vendor/soq/src/libsoq/stderr.c
)
SET_TARGET_PROPERTIES(soq PROPERTIES
  POSITION_INDEPENDENT_CODE ON
)

# fix for installing with a package manager that does not clone recursive
FILE(GLOB PYBIND11_DIR_FILES vendor/pybind11)
LIST(LENGTH PYBIND11_DIR_FILES PYBIND11_EMPTY)
IF(PYBIND11_EMPTY EQUAL 0)
  FILE(REMOVE_RECURSE vendor/pybind11)
  EXECUTE_PROCESS(
    COMMAND git submodule update --init vendor/pybind11
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  )
ENDIF()

# add additional cmake files
ADD_SUBDIRECTORY(vendor/pybind11)
ADD_SUBDIRECTORY(snmp_stream/_snmp_stream)
ADD_SUBDIRECTORY(docs)
