FIND_PACKAGE(OpenSSL REQUIRED)

PYBIND11_ADD_MODULE(_snmp_stream
  module.cpp
  session.cpp
  types.cpp
  utils.cpp
)

TARGET_INCLUDE_DIRECTORIES(_snmp_stream
  PUBLIC ${PROJECT_SOURCE_DIR}/include
)

#ADD_DEPENDENCIES(_snmp_stream
#  soq netsnmp
#)

TARGET_LINK_LIBRARIES(_snmp_stream
  PRIVATE soq netsnmp OpenSSL::Crypto
)

IF(DEFINED ENV{SNMP_STREAM_COVERAGE})
  MESSAGE(STATUS "snmp-stream coverage reporting enabled")
  TARGET_COMPILE_OPTIONS(_snmp_stream
    PRIVATE -O0 -ftest-coverage -fprofile-arcs
  )
TARGET_LINK_LIBRARIES(_snmp_stream
  PRIVATE gcov
)
ENDIF()

# clang fix
IF(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  TARGET_COMPILE_OPTIONS(_snmp_stream
    PRIVATE -fsized-deallocation
  )
ENDIF()


INSTALL(
  TARGETS _snmp_stream
  LIBRARY DESTINATION "${PROJECT_SOURCE_DIR}/snmp_stream"
)
