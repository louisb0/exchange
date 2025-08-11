#pragma once

#include <vector>

namespace network {

int connect_clients(size_t n_clients, std::vector<int> &fds);

} // namespace network
