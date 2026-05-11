#pragma once
#include <sys/socket.h>

namespace util {
    void* get_in_addr(sockaddr* sa);
}
