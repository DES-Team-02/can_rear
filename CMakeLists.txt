### SETUP ###
cmake_minimum_required(VERSION 3.15)
project(Can_Receiver_IC VERSION 0.1 LANGUAGES CXX)

### PACKAGES ###
find_package(CommonAPI REQUIRED)
find_package(CommonAPI-SomeIP REQUIRED)

#### VARIABLES ####
set(Can_Receiver_SOURCES
    src/main.cpp
    src/SpeedSensorStubImpl.cpp
    src/CanDataRegister.cpp
    src/MovingAverageFilter.cpp
    src/CanReceiver.cpp
)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -std=c++11")
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(INSTALL_LIB_DIR lib CACHE PATH "Installation directory for libraries")
set(INSTALL_BIN_DIR bin CACHE PATH "Installation directory for binaries")
set(INSTALL_INCLUDE_DIR include/CanReceiver CACHE PATH "Installation directory for header files")
set(DEF_INSTALL_CMAKE_DIR lib/cmake/CanReceiver-someip)
set(INSTALL_CMAKE_DIR ${DEF_INSTALL_CMAKE_DIR} CACHE PATH "Installation directory for CMake files")

foreach(p LIB INCLUDE CMAKE)
  set(var INSTALL_${p}_DIR)
  if(NOT IS_ABSOLUTE "${${var}}")
    set(ABSOLUTE_${var} "${CMAKE_INSTALL_PREFIX}/${${var}}")
  endif()
endforeach()

FILE(GLOB PRJ_SOMEIP_LIB_SRCS ./src-gen/someip/v0/commonapi/*.cpp)

### DIRECTORIES ###
include_directories(
	./include
    ./src-gen/core
    ./src-gen/someip
    ${COMMONAPI_INCLUDE_DIRS}
    ${COMMONAPI_SOMEIP_INCLUDE_DIRS}
)

link_directories(
    ${COMMONAPI_LIBDIR}
    ${COMMONAPI_SOMEIP_CMAKE_DIR}/build
)
#### BUILD ####
# Service
add_executable(CanReceiver 
    ${Can_Receiver_SOURCES}
)
target_link_libraries(CanReceiver CommonAPI)

add_library(CanReceiver-someip
    SHARED ${PRJ_SOMEIP_LIB_SRCS}
)
target_link_libraries(CanReceiver-someip
    CommonAPI
    CommonAPI-SomeIP
)
set_target_properties(CanReceiver-someip PROPERTIES VERSION 0.1 SOVERSION 0.1)
target_include_directories(CanReceiver-someip PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src-gen/core>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src-gen/someip>
    $<INSTALL_INTERFACE:${INSTALL_INCLUDE_DIR}>
)
set_target_properties (CanReceiver-someip PROPERTIES INTERFACE_LINK_LIBRARY "")

install(TARGETS CanReceiver CanReceiver-someip
        EXPORT CanReceiver-someipTargets
        LIBRARY DESTINATION ${INSTALL_LIB_DIR}
        RUNTIME DESTINATION ${INSTALL_BIN_DIR}
        ARCHIVE DESTINATION ${INSTALL_LIB_DIR}
        PUBLIC_HEADER DESTINATION "${INSTALL_INCLUDE_DIR}/CanReceiver"
)
