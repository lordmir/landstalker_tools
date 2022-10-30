CMAKE_MINIMUM_REQUIRED(VERSION 3.13.4)

SET(LIBRARY_NAME landstalker)

ADD_LIBRARY(${LIBRARY_NAME})

SET_TARGET_PROPERTIES(${LIBRARY_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_EXTENSIONS OFF
)

TARGET_INCLUDE_DIRECTORIES(${LIBRARY_NAME}
    PUBLIC include
    PUBLIC ${PROJECT_BINARY_DIR})

TARGET_SOURCES(
    ${LIBRARY_NAME}
    PRIVATE src/BitBarrel.cpp
    PRIVATE src/BitBarrelWriter.cpp
    PRIVATE src/Block.cpp
    PRIVATE src/BlocksetCmp.cpp
    PRIVATE src/EndCreditString.cpp
    PRIVATE src/HuffmanString.cpp
    PRIVATE src/HuffmanTree.cpp
    PRIVATE src/HuffmanTrees.cpp
    PRIVATE src/IntroString.cpp
    PRIVATE src/LSString.cpp
    PRIVATE src/LZ77.cpp
    PRIVATE src/Tile.cpp
    PRIVATE src/TileAttributes.cpp
    PRIVATE src/Tilemap2DRLE.cpp
    PRIVATE src/Tilemap3DCmp.cpp
    PRIVATE src/Utils.cpp
)

INSTALL(TARGETS ${LIBRARY_NAME} DESTINATION lib)
INSTALL(FILES
    include/landstalker/BitBarrel.h
    include/landstalker/BitBarrelWriter.h
    include/landstalker/Block.h
    include/landstalker/BlocksetCmp.h
    include/landstalker/Charset.h
    include/landstalker/EndCreditString.h
    include/landstalker/HuffmanString.h
    include/landstalker/HuffmanTree.h
    include/landstalker/HuffmanTrees.h
    include/landstalker/IntroString.h
    include/landstalker/LSString.h
    include/landstalker/LZ77.h
    include/landstalker/RomOffsets.h
    include/landstalker/Tile.h
    include/landstalker/TileAttributes.h
    include/landstalker/Tilemap2DRLE.h
    include/landstalker/Tilemap3DCmp.h
    include/landstalker/Utils.h
    DESTINATION include/landstalker
)
