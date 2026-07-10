cmake_minimum_required(VERSION 3.29)
project(server)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-static")

find_package(Boost CONFIG REQUIRED COMPONENTS lockfree)

include_directories(src)

set(
        SERVER_SOURCES
        cmd/app.cpp
        src/remote/soundstreamingservice/recorder/WASAPIAudioRecorder.cpp
        src/remote/soundstreamingservice/recorder/WASAPIAudioRecorder.hpp

        src/remote/soundstreamingservice/consumerservice/wavefileconsumerservice/WaveConsumerService.cpp
        src/remote/soundstreamingservice/consumerservice/wavefileconsumerservice/WaveConsumerService.hpp

        src/ErrorsConfig.cpp
        src/ErrorCodes.hpp

        src/remote/soundstreamingservice/recorder/creators/builder/WASAPIAudioRecorderBuilder.cpp
        src/remote/soundstreamingservice/recorder/creators/builder/WASAPIAudioRecorderBuilder.hpp
        src/remote/soundstreamingservice/recorder/creators/directors/sharedmode/WASAPIAudioSharedModeRecorderDirector.cpp
        src/remote/soundstreamingservice/recorder/creators/directors/sharedmode/WASAPIAudioSharedModeRecorderDirector.hpp
        src/remote/soundstreamingservice/recorder/creators/directors/exclusivemode/WASAPIAudioExclusiveModeRecorderDirector.cpp
        src/remote/soundstreamingservice/recorder/creators/directors/exclusivemode/WASAPIAudioExclusiveModeRecorderDirector.hpp

        src/remote/soundstreamingservice/recorder/ErrorHandling.hpp

        src/remote/soundstreamingservice/recorder/utils.cpp
        src/remote/soundstreamingservice/recorder/utils.hpp
        src/remote/soundstreamingservice/recorder/exceptions/BaseRecorderException.hpp
        src/remote/soundstreamingservice/recorder/exceptions/EventCreationFailureException.hpp
        src/remote/soundstreamingservice/recorder/exceptions/StartRecordingFailure.hpp
        src/remote/soundstreamingservice/recorder/exceptions/ChunkRecordingFailure.hpp
        src/remote/soundstreamingservice/consumer/Consumer.cpp
        src/remote/soundstreamingservice/consumer/Consumer.hpp
        src/remote/soundstreamingservice/producer/Producer.cpp
        src/remote/soundstreamingservice/producer/Producer.hpp
        src/remote/soundstreamingservice/audiobufferpool/AudioBufferPool.cpp
        src/remote/soundstreamingservice/audiobufferpool/AudioBufferPool.hpp
        src/remote/soundstreamingservice/recorder/exceptions/RecorderCreationFailed.hpp
        src/constants/QueueSizes.hpp
        src/remote/soundstreamingservice/consumerservice/networkstreamingservice/StreamingService.cpp
        src/remote/sockets/socketserver/tcpsocket/TcpConnection.cpp
        src/remote/sockets/socketserver/tcpsocket/TcpConnection.hpp
        src/remote/soundstreamingservice/consumerservice/networkstreamingservice/StreamingService.hpp
        src/remote/soundstreamingservice/consumerservice/IConsumerService.hpp
        src/remote/soundstreamingservice/consumerservice/wavefileconsumerservice/wave_constants.hpp
        src/remote/timesyncservice/TimeSyncService.cpp
        src/remote/timesyncservice/TimeSyncService.hpp
        src/config/ConfigManager.cpp
        src/config/ConfigManager.hpp
        src/remote/timesyncservice/SyncException.hpp
        src/exceptions/BaseException.hpp
        src/remote/sockets/networkanalyzer/NetworkAnalyzer.cpp
        src/remote/sockets/networkanalyzer/NetworkAnalyzer.hpp
        src/remote/sockets/socketserver/SocketConfig.hpp
        src/remote/sockets/socketclient/UdpSocketClient.cpp
        src/remote/sockets/socketclient/UdpSocketClient.hpp
        src/remote/timesyncservice/GetTimestamp.hpp
        src/remote/sockets/byteorder/ByteOrder.cpp
        src/remote/sockets/byteorder/ByteOrder.hpp
        src/remote/soundstreamingservice/Common.hpp
        src/remote/sockets/exceptions/NetworkExceptions.hpp
        src/remote/sockets/socketserver/IConnection.hpp
        src/remote/sockets/socketserver/IAcceptor.hpp
        src/remote/sockets/socketserver/tcpsocket/TCPAcceptor.cpp
        src/remote/sockets/socketserver/tcpsocket/TCPAcceptor.hpp
        src/remote/soundstreamingservice/ControlCodes.hpp
        src/remote/soundstreamingservice/manager/SoundStreamingManager.cpp
        src/remote/soundstreamingservice/manager/SoundStreamingManager.hpp
        src/CommandCodes.hpp
        src/remote/soundstreamingservice/recorder/IResampler.hpp
        src/remote/soundstreamingservice/recorder/IAudioRecorder.hpp
        src/remote/soundstreamingservice/recorder/SoxrResampler.hpp
        src/remote/soundstreamingservice/recorder/ResamplingAudioRecorderDecorator.hpp
        src/remote/soundstreamingservice/recorder/MonoDownmixAudioRecorderDecorator.hpp
)

add_executable(
        server
        ${SERVER_SOURCES}
)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

include(FetchContent)
FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.12.0
)
FetchContent_MakeAvailable(nlohmann_json)

FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG        v1.16.0
)
FetchContent_MakeAvailable(spdlog)

set(OPUS_BUILD_SHARED_LIBRARY OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
        Opus
        GIT_REPOSITORY https://github.com/xiph/opus.git
        GIT_TAG        v1.5.2
)
set(OPUS_BUILD_PROGRAMS OFF CACHE BOOL "" FORCE)
set(OPUS_BUILD_TESTING OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(Opus)

list(PREPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules")

FetchContent_Declare(
        soxr
        GIT_REPOSITORY https://github.com/chirlu/soxr.git
        GIT_TAG        0.1.3
)

set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(WITH_OPENMP OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(soxr)

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
        Iphlpapi
        nlohmann_json::nlohmann_json
        Boost::lockfree
        spdlog::spdlog
        opus
        soxr
)
target_compile_options(server PRIVATE -Wall -Wextra -pedantic)
