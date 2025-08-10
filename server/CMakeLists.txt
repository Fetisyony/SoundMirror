cmake_minimum_required(VERSION 3.29)
project(server)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-static")

find_package(Boost CONFIG REQUIRED COMPONENTS lockfree)

include_directories(src)

set(
        SERVER_SOURCES
        cmd/app.cpp
        src/audiostreamingservice/recorder/WASAPIAudioRecorder.cpp
        src/audiostreamingservice/recorder/WASAPIAudioRecorder.hpp

        src/mathutils/converters.cpp
        src/mathutils/converters.hpp

        src/audiostreamingservice/wavefileconsumerservice/WaveConsumerService.cpp
        src/audiostreamingservice/wavefileconsumerservice/WaveConsumerService.hpp

        src/ErrorsConfig.cpp
        src/ErrorCodes.hpp

        src/audiostreamingservice/recorder/creators/builder/WASAPIAudioRecorderBuilder.cpp
        src/audiostreamingservice/recorder/creators/builder/WASAPIAudioRecorderBuilder.hpp
        src/audiostreamingservice/recorder/creators/directors/sharedmode/WASAPIAudioSharedModeRecorderDirector.cpp
        src/audiostreamingservice/recorder/creators/directors/sharedmode/WASAPIAudioSharedModeRecorderDirector.hpp
        src/audiostreamingservice/recorder/creators/directors/exclusivemode/WASAPIAudioExclusiveModeRecorderDirector.cpp
        src/audiostreamingservice/recorder/creators/directors/exclusivemode/WASAPIAudioExclusiveModeRecorderDirector.hpp

        src/audiostreamingservice/recorder/ErrorHandling.hpp

        src/audiostreamingservice/recorder/utils.cpp
        src/audiostreamingservice/recorder/utils.hpp
        src/audiostreamingservice/recorder/exceptions/BaseRecorderException.hpp
        src/audiostreamingservice/recorder/exceptions/EventCreationFailureException.hpp
        src/audiostreamingservice/recorder/exceptions/StartRecordingFailure.hpp
        src/audiostreamingservice/recorder/exceptions/ChunkRecordingFailure.hpp
        src/audiostreamingservice/consumer/Consumer.cpp
        src/audiostreamingservice/consumer/Consumer.hpp
        src/audiostreamingservice/producer/Producer.cpp
        src/audiostreamingservice/producer/Producer.hpp
        src/audiostreamingservice/audiobufferpool/AudioBufferPool.cpp
        src/audiostreamingservice/audiobufferpool/AudioBufferPool.hpp
        src/audiostreamingservice/recorder/exceptions/RecorderCreationFailed.hpp
        src/constants/QueueSizes.hpp
        src/audiostreamingservice/networkstreamingservice/StreamingService.cpp
        src/network/socketserver/tcpsocketserver/TCPSocketServer.cpp
        src/network/socketserver/tcpsocketserver/TCPSocketServer.hpp
        src/network/socketserver/ISocketServer.hpp
        src/audiostreamingservice/networkstreamingservice/StreamingService.hpp
        src/audiostreamingservice/consumer/IConsumerService.hpp
        src/audiostreamingservice/wavefileconsumerservice/WaveConstants.hpp
        src/timesyncservice/TimeSyncService.cpp
        src/timesyncservice/TimeSyncService.hpp
        src/config/ConfigManager.cpp
        src/config/ConfigManager.hpp
        src/timesyncservice/SyncException.hpp
        src/exceptions/BaseException.hpp
        src/network/networkanalyzer/NetworkAnalyzer.cpp
        src/network/networkanalyzer/NetworkAnalyzer.hpp
        src/network/socketserver/udpsocketserver/UDPSocketServer.cpp
        src/network/socketserver/udpsocketserver/UDPSocketServer.hpp
        src/network/socketserver/SocketConfig.hpp
)

add_executable(
        server
        ${SERVER_SOURCES}
)

include(FetchContent)
FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.12.0
)
FetchContent_MakeAvailable(nlohmann_json)
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
)
target_compile_options(server PRIVATE -Wall -Wextra -pedantic)
