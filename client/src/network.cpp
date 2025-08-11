#include "network.hpp"

#include "include/net_config.hpp"

#include <spdlog/spdlog.h>

#include <netinet/in.h>
#include <sys/socket.h>
#include <vector>

int network::connect_clients(size_t n_clients, std::vector<int> &fds) {
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(net_config::GATEWAY_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    for (size_t i = 0; i < n_clients; i++) {
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd == -1) {
            spdlog::critical("[network::connect_clients] socket: {}", errno);
            return -1;
        }

        if (connect(fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) == -1) {
            spdlog::critical("[network::connect_clients] connect: {}", errno);
            return -1;
        }

        fds.push_back(fd);
    }

    return 0;
}
