cmake_minimum_required(VERSION 3.29)
project(server)

set(CMAKE_CXX_STANDARD 20)

set(BOOST_ROOT "C:/local/boost_1_88_0")
set(BOOST_INCLUDEDIR "${BOOST_ROOT}")

find_package(Boost REQUIRED)
if (Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
endif()

include_directories(src)

set(
        SERVER_SOURCES
        cmd/app.cpp
        src/recorder/WASAPIAudioRecorder.cpp
        src/recorder/WASAPIAudioRecorder.hpp

        src/mathutils/converters.cpp
        src/mathutils/converters.hpp

        src/wavefile/WaveCreator.cpp
        src/wavefile/WaveCreator.hpp

        src/ErrorsConfig.cpp
        src/ErrorCodes.hpp

        src/recorder/creators/builder/WASAPIAudioRecorderBuilder.cpp
        src/recorder/creators/builder/WASAPIAudioRecorderBuilder.hpp
        src/recorder/creators/directors/sharedmode/WASAPIAudioSharedModeRecorderDirector.cpp
        src/recorder/creators/directors/sharedmode/WASAPIAudioSharedModeRecorderDirector.hpp
        src/recorder/creators/directors/exclusivemode/WASAPIAudioExclusiveModeRecorderDirector.cpp
        src/recorder/creators/directors/exclusivemode/WASAPIAudioExclusiveModeRecorderDirector.hpp

        src/recorder/ErrorHandling.hpp

        src/recorder/utils.cpp
        src/recorder/utils.hpp
        src/recorder/exceptions/BaseRecorderException.hpp
        src/recorder/exceptions/EventCreationFailureException.hpp
        src/recorder/exceptions/StartRecordingFailure.hpp
        src/recorder/exceptions/ChunkRecordingFailure.hpp
        src/consumer/Consumer.cpp
        src/consumer/Consumer.hpp
        src/producer/Producer.cpp
        src/producer/Producer.hpp
        src/AudioBufferPool.cpp
        src/AudioBufferPool.hpp
        src/recorder/exceptions/RecorderCreationFailed.hpp
        src/constants/QueueSizes.hpp
        src/network/StreamingService.cpp
        src/network/socketserver/TCPSocketServer.cpp
        src/network/socketserver/TCPSocketServer.hpp
        src/network/socketserver/ISocketServer.hpp
        src/network/StreamingService.hpp
        src/consumer/IConsumerService.hpp
        src/wavefile/WaveConstants.hpp
)

add_executable(
        server
        ${SERVER_SOURCES}
)

target_link_libraries(
        server
        PRIVATE
        ole32
        uuid
        winmm
        ws2_32
        propsys
        ksuser
        Avrt
)
target_compile_options(server PRIVATE -Wall -Wextra -pedantic)
