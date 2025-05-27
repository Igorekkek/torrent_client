#include "tcp_connect.h"
#include "byte_tools.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdexcept>
#include <cstring>
#include <iostream>
#include <chrono>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>  
#include <sys/poll.h>
#include <limits>
#include <utility>
#include <cerrno>

TcpConnect::TcpConnect(std::string ip, int port, std::chrono::milliseconds connectTimeout, std::chrono::milliseconds readTimeout)
    : ip_(std::move(ip)),
      port_(port),
      connectTimeout_(connectTimeout),
      readTimeout_(readTimeout),
      sock_(-1) {}

TcpConnect::~TcpConnect() {
    CloseConnection();
}

void TcpConnect::EstablishConnection() {
    sock_ = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_ == -1) {
        throw std::runtime_error("Failed to create socket");
    }

    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_);
    if (inet_pton(AF_INET, ip_.c_str(), &addr.sin_addr) <= 0) {
        throw std::runtime_error("Invalid address");
    }

    int flags = fcntl(sock_, F_GETFL, 0);
    fcntl(sock_, F_SETFL, flags | O_NONBLOCK);

    int res = connect(sock_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    pollfd pfd{sock_, POLLOUT, 0};
    int pollres = poll(&pfd, 1, connectTimeout_.count());
    if (pollres <= 0) {
        CloseConnection();
        throw std::runtime_error("can't connect to peer");
    }
}

void TcpConnect::SendData(const std::string& data) {
    using Clock = std::chrono::steady_clock;
    auto deadline = Clock::now() + std::chrono::milliseconds(1000);
    size_t totalSent = 0;

    while (totalSent < data.size()) {
        int64_t sent = send(sock_, data.data() + totalSent, data.size() - totalSent, 0);

        if (sent > 0) {
            totalSent += sent;
        } else if (sent == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            auto now = Clock::now();
            if (now >= deadline) {
                CloseConnection();
                throw std::runtime_error("<send> timeout");
            }
            int64_t remain_working_time = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now).count();

            pollfd pfd{sock_, POLLOUT, 0};
            int pollRes = poll(&pfd, 1, remain_working_time);
            if (pollRes <= 0) {
                CloseConnection();
                throw std::runtime_error("<send> timeout or error");
            }
        } else {
            throw std::runtime_error("<send> failed");
        }
    }
}

std::string TcpConnect::ReceiveData(size_t bufferSize) {
    // Если размер буфера не задан, читаем длину сообщения (4 байта)
    if (bufferSize == 0) {
        std::string length = ReceiveData(4);
        if (length.size() != 4) throw std::runtime_error("<ReceiveData> can't read buffersize");
        bufferSize = BytesToInt(length);
    }

    std::string result;
    result.resize(bufferSize);
    size_t totalReceived = 0;

    using Clock = std::chrono::steady_clock;
    auto deadline = Clock::now() + readTimeout_;

    while (totalReceived < bufferSize) {
        ssize_t received = recv(sock_, result.data() + totalReceived, bufferSize - totalReceived, 0);

        if (received > 0) {
            totalReceived += received;
            continue;
        }

        if (received == 0) {
            CloseConnection();
            throw std::runtime_error("<RecieveData> connection closed by peer during receive");
        }

        if (received == -1 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)) {
            auto now = Clock::now();
            if (now >= deadline) {
                CloseConnection();
                throw std::runtime_error("<RecieveData> receive timeout exceeded");
            }

            int64_t remain_working_time = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now).count();
            pollfd pfd{sock_, POLLIN, 0};
            int pollRes = poll(&pfd, 1, remain_working_time);
            if (pollRes <= 0) {
                CloseConnection();
                throw std::runtime_error("Poll error or receive timeout");
            }
            continue;
        }

        CloseConnection();
        throw std::runtime_error(std::string("<ReceiveData> recv() failed: ") + std::strerror(errno));
    }
    return result;
}

void TcpConnect::CloseConnection() {
    if (sock_ >= 0) {
        close(sock_);
        sock_ = -1;
    }
}

const std::string& TcpConnect::GetIp() const {
    return ip_;
}

int TcpConnect::GetPort() const {
    return port_;
}