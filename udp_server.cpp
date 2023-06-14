#include <functional>

#include "udp_server.hpp"
#include "cemuhook_controller_data.hpp"

udp::socket* m_socket;
uint32_t m_id;
udp::endpoint m_remote_endpoint;
bool m_connection_stablished = false;

std::vector<uint8_t> m_recv_buf;
asio::io_context m_io_context;

uint32_t
UdpServer::id()
{
    return m_id;
}

void
UdpServer::sendData(CemuhookDataMessage& controllerDataState)
{
    if (m_connection_stablished) {
        static uint32_t packetNumber = 0l;
        controllerDataState.v_packetNumber = ++packetNumber;
        controllerDataState.genCRC32();

        m_socket.send_to(asio::buffer(controllerDataState.m_data,
                                       controllerDataState.m_data.size()),
                          m_remote_endpoint);
    }
}

void
UdpServer::handleReceive(asio::error_code ec, std::size_t len)
{
    if (!ec && len > 0) {

        m_connection_stablished = true;

        if (len <= 28) {
            CemuhookMessage<8>::DataBufer received_data{};
            for (int i = 0; i < len; ++i) {
                received_data[i] = m_recv_buf[i];
            }
            CemuhookRequest r{ CemuhookMessage<8>(std::move(received_data)) };
            m_connection_stablished = true;
            if (r.v_eventType == 0x100001) {
                static CemuhookMessage<2> m(0001, 0x100000);
                m.payloadView<uint16_t, 0>() = (uint16_t)1001u;
                m.genCRC32();
                m_socket.send_to(asio::buffer(m.m_data), m_remote_endpoint);
            }
            if (r.v_eventType == 0x100002) {
                static CemuhookInfoMessage m(m_id);
                m.genCRC32();
                m_socket.send_to(asio::buffer(m.m_data, m.m_data.size()),
                                  m_remote_endpoint);
            }
        }
    } else {

        std::cerr << "Error code: " << ec << "-" << ec.message() << std::endl;
        std::cerr << "Connection Failed" << std::endl;
        m_connection_stablished = false;
    }
    asyncReceiveStart();
}

void
UdpServer::asyncReceiveStart()
{
    m_socket.async_receive_from(asio::buffer(m_recv_buf, 1024),
                                m_remote_endpoint,
                                std::bind(&UdpServer::handleReceive,
                                          this,
                                          std::placeholders::_1,
                                          std::placeholders::_2));
}

void
UdpServer::run()
{
    m_thread = std::thread([&](){m_io_context.run();});
    m_thread.detach();
}

UdpServer::~UdpServer()
{
    if(m_thread.joinable())
        m_thread.join();
}

uint32_t
UdpServer::generateId(){
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> dist(0,
                                                                  UINT32_MAX);
    return dist(rng);
}

UdpServer::UdpServer()
  : m_id{ generateId() }
  , m_recv_buf(1024)
  , m_socket(m_io_context, udp::endpoint(udp::v4(), 26760))
{
    printf("This service Id %i", m_id);
    asyncReceiveStart();
}