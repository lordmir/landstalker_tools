CMAKE_MINIMUM_REQUIRED(VERSION 3.13.4)

SET(EXECUTABLE_NAME lz77)

ADD_EXECUTABLE(${EXECUTABLE_NAME} main.cpp)

SET_TARGET_PROPERTIES(${EXECUTABLE_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_EXTENSIONS OFF
)

TARGET_INCLUDE_DIRECTORIES(${EXECUTABLE_NAME}
    PUBLIC ${PROJECT_BINARY_DIR}
    PUBLIC ../common/include
    PUBLIC ../third_party/tclap-1.2.2/include
)
TARGET_LINK_LIBRARIES(${EXECUTABLE_NAME} landstalker)

INSTALL(TARGETS ${EXECUTABLE_NAME} DESTINATION bin)
