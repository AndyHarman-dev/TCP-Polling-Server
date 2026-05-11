#include "util/sockets.h"
#include <netinet/in.h>

namespace util {
    void* get_in_addr(sockaddr* sa) {
        if (sa->sa_family == AF_INET) {
            return &(reinterpret_cast<sockaddr_in*>(sa)->sin_addr);
        }
        return &(reinterpret_cast<sockaddr_in6*>(sa)->sin6_addr);
    }
}
