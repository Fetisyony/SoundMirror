cmake_minimum_required(VERSION 3.18.1)

project("opus_decoder")

add_definitions(
        -DOPUS_BUILD
        -DUSE_ALLOCA
        -DHAVE_LRINT
        -DHAVE_LRINTF
)

add_compile_options(-O3)

set(OPUS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libopus)

include_directories(
        ${OPUS_DIR}/include
        ${OPUS_DIR}/celt
        ${OPUS_DIR}/silk
        ${OPUS_DIR}/silk/float
        ${OPUS_DIR}/src
)

file(GLOB OPUS_SOURCES
        "${OPUS_DIR}/src/*.c"
        "${OPUS_DIR}/silk/*.c"
        "${OPUS_DIR}/silk/float/*.c"
        "${OPUS_DIR}/celt/*.c"
)

list(REMOVE_ITEM OPUS_SOURCES
        "${OPUS_DIR}/src/opus_compare.c"
        "${OPUS_DIR}/src/opus_demo.c"
        "${OPUS_DIR}/celt/opus_custom_demo.c"
        "${OPUS_DIR}/src/repacketizer_demo.c"
)

add_library(native-lib SHARED
        native-lib.cpp
        ${OPUS_SOURCES}
)

target_link_options(native-lib PRIVATE "-Wl,-z,max-page-size=16384")
find_library(log-lib log)

target_link_libraries(native-lib ${log-lib})
