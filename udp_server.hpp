#pragma once

#include <asio.hpp>
#include <random>
#include <functional>
#include <thread>

#include "cemuhook_protocol.hpp"

using asio::ip::udp;

class CemuhookDataMessage;

class UdpServer
{
    uint32_t m_id;

    asio::io_context m_io_context;
    udp::socket m_socket;
    udp::endpoint m_remote_endpoint;
    bool m_connection_stablished = false;

    std::thread m_thread;

    std::vector<uint8_t> m_recv_buf;

    uint32_t generateId();

  public:
    uint32_t id();
    void sendData(CemuhookDataMessage& controllerDataState);
    void handleReceive(asio::error_code ec, std::size_t len);
    void asyncReceiveStart();
    void run();

    UdpServer();
    ~UdpServer();
};