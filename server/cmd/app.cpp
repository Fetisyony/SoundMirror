#include "remote/sockets/socketserver/tcpsocket/TCPAcceptor.hpp"
#include "remote/soundstreamingservice/consumerservice/wavefileconsumerservice/WaveConsumerService.hpp"
#include "config/ConfigManager.hpp"
#include "remote/timesyncservice/TimeSyncService.hpp"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>

#include "CommandCodes.hpp"
#include "remote/sockets/byteorder/ByteOrder.hpp"
#include "remote/sockets/socketserver/SocketConfig.hpp"
#include "remote/sockets/socketserver/tcpsocket/TcpConnection.hpp"
#include "remote/soundstreamingservice/manager/SoundStreamingManager.hpp"

using std::cout;
using std::endl;

void handleClient(int32_t code);

int main() {
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("logs/app.log", 1048576 * 5, 3);
    std::vector<spdlog::sink_ptr> sinks{console_sink, file_sink};
    auto logger = std::make_shared<spdlog::logger>("multi_sink", begin(sinks), end(sinks));
    spdlog::register_logger(logger);
    spdlog::set_default_logger(logger);
    spdlog::set_pattern("%H:%M:%S.%e [%^%l%$] [tid %t] %v");
    spdlog::set_level(spdlog::level::info);

    spdlog::info("Application started");

    ConfigManager::getInstance().initialize("../src/config/config.json");

    auto config = SocketConfig(ConfigManager::getInstance().getConfig().network.controlPort);
    auto masterServer = std::make_shared<TcpAcceptor>(ConfigManager::getInstance().getConfig().network.controlPort);

    std::vector<std::thread> threads;
    std::vector<BYTE> command(4);
    for (;;) {
        auto client = masterServer->accept();

        client->recvAll(command.data(), command.size());
        uint32_t value = byteorder::bufferLEToUint32(command.data());

        auto syncer = TimeSyncService(ConfigManager::getInstance().getConfig().network.timeSyncPort);
        syncer.syncConversation();

        threads.push_back(
            std::thread{handleClient, value}
        );
    }

    return 0;
}

void handleClient(int32_t code) {
    switch (code) {
        case SOUND_STREAM:
            streamSound();
            break;
        default:
            spdlog::warn("Unknown command code: {}", code);
            break;
    }
}
