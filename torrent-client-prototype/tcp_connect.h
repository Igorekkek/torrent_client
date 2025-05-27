#pragma once

#include <string>
#include <chrono>

/*
 * Обертка над низкоуровневой структурой сокета.
 */
class TcpConnect {
public:
    TcpConnect(std::string ip, int port, std::chrono::milliseconds connectTimeout, std::chrono::milliseconds readTimeout);
    ~TcpConnect();

    void EstablishConnection();

    void SendData(const std::string& data);

    std::string ReceiveData(size_t bufferSize = 0);

    void CloseConnection();

    const std::string& GetIp() const;
    int GetPort() const;
private:
    const std::string ip_;
    const int port_;
    std::chrono::milliseconds connectTimeout_, readTimeout_;
    int sock_;
};
