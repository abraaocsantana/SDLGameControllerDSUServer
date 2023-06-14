#pragma once

#include <cstdint>
#include <iostream>
#include <utility>
#include <array>

template<uint16_t DataLength>
struct CemuhookMessage
{
    static constexpr size_t FullLength() { return HeaderLenght + DataLength; }
    static const size_t HeaderLenght = 20;

    using DataBufer = std::array<uint8_t, FullLength()>;

    template<typename T, std::size_t Index, std::size_t IndexStart = 0>
    constexpr T& view();

    template<typename T, std::size_t Index, std::size_t IndexStart = 0>
    constexpr T& payloadView();

    inline void genCRC32();
    inline bool checkCRC32();

    constexpr CemuhookMessage(uint32_t id, uint32_t eventType);

    CemuhookMessage(CemuhookMessage&& otherMessage)
      : m_data{ std::move(otherMessage.m_data) } {};

    CemuhookMessage(DataBufer&& data)
      : m_data{ std::move(data) } {};

    DataBufer m_data;

    uint16_t& v_protocol = view<uint16_t, 4>();
    uint16_t& v_dataLength = view<uint16_t, 6>();
    uint32_t& v_crc32 = view<uint32_t, 8>();
    uint32_t& v_id = view<uint32_t, 12>();
    uint32_t& v_eventType = view<uint32_t, 16>();
};

struct CemuhookRequest : CemuhookMessage<8>
{
    template<uint32_t eventType>
    using RequestClass = CemuhookRequest;

    constexpr CemuhookRequest(uint32_t id, uint32_t eventType)
      : CemuhookMessage(id, eventType){};

    CemuhookRequest(CemuhookMessage<8>&& genericMessage)
      : CemuhookMessage<8>(std::move(genericMessage)){};
};

struct CemuhookControllerInfoRequest : CemuhookRequest
{
    const uint32_t InfoRequestEventTYpe = 0x100001;

    uint8_t& v_amount = payloadView<uint8_t, 0>();
    uint32_t& v_request = payloadView<uint32_t, 4>();

    constexpr CemuhookControllerInfoRequest(uint32_t id)
      : CemuhookRequest(id, InfoRequestEventTYpe){};

    CemuhookControllerInfoRequest(CemuhookRequest&& genericRequest)
      : CemuhookRequest(std::move(genericRequest))
    {
        if (v_eventType != InfoRequestEventTYpe) {
            std::cerr << "Something went wrong" << std::endl; // Test
        }
    };
};

struct CemuhookControllerDataRequest : CemuhookRequest
{
    const uint32_t ControllerDataRequest = 0x100002;

    uint8_t& v_actionBitMask = payloadView<uint8_t, 0>();
    uint8_t& v_slot = payloadView<uint8_t, 0>();
    uint32_t& v_macAddress = payloadView<uint32_t, 4>(); // 48bits!?

    constexpr CemuhookControllerDataRequest(uint32_t id)
      : CemuhookRequest(id, ControllerDataRequest){};

    CemuhookControllerDataRequest(CemuhookRequest&& genericRequest)
      : CemuhookRequest(std::move(genericRequest))
    {
        if (v_eventType != ControllerDataRequest) {
            std::cerr << "Something went wrong" << std::endl; // Test
        }
    };
};

template<uint16_t DataLength>
struct CemuhookOutgoingMessage : CemuhookMessage<11 + DataLength>
{
    uint8_t& v_slot = payloadView<uint8_t, 0>();
    uint8_t& v_state = payloadView<uint8_t, 1>();
    uint8_t& v_modelGyro =
      payloadView<uint8_t, 2>(); // 1 = no or partial gyro , 2 = ful gyro
    uint8_t& v_connectionType = payloadView<uint8_t, 3>(); // 1 = USB , 2 =
                                                         // Bluetooth
    uint32_t& v_macAddress = payloadView<uint32_t, 4>();   // 48 bits!?!?
    uint8_t& v_batteryStatus = payloadView<uint8_t, 10>();

    constexpr CemuhookOutgoingMessage(uint32_t id, uint32_t eventType);
};

struct CemuhookInfoMessage : CemuhookOutgoingMessage<12 - 11>
{
    // 0 - 11 Common Beginning
    uint8_t& v_zerotermination = payloadView<uint8_t, 11>();

    constexpr CemuhookInfoMessage(uint32_t id)
      : CemuhookOutgoingMessage(id, 0x100001)
    {
        v_zerotermination = 0u;
    };
};

struct CemuhookDataMessage : CemuhookOutgoingMessage<80 - 11>
{
    // 0 - 11 Common Beginning
    uint8_t& v_connected = payloadView<uint8_t, 11>();
    uint32_t& v_packetNumber = payloadView<uint32_t, 12>();
    uint8_t& v_bitmask1 = payloadView<uint8_t, 16>();
    uint8_t& v_bitmask2 = payloadView<uint8_t, 17>();
    uint8_t& v_buttonHome = payloadView<uint8_t, 18>();
    uint8_t& v_buttonTouch = payloadView<uint8_t, 19>();
    uint8_t& v_analogButtonStickLX = payloadView<uint8_t, 20>();
    uint8_t& v_analogButtonStickLY = payloadView<uint8_t, 21>();
    uint8_t& v_analogButtonStickRX = payloadView<uint8_t, 22>();
    uint8_t& v_analogButtonStickRY = payloadView<uint8_t, 23>();
    uint8_t& v_analogButtonDPadL = payloadView<uint8_t, 24>();
    uint8_t& v_analogButtonDPadD = payloadView<uint8_t, 25>();
    uint8_t& v_analogButtonDPadR = payloadView<uint8_t, 26>();
    uint8_t& v_analogButtonDPadU = payloadView<uint8_t, 27>();
    uint8_t& v_analogButtonY = payloadView<uint8_t, 28>();
    uint8_t& v_analogButtonB = payloadView<uint8_t, 29>();
    uint8_t& v_analogButtonA = payloadView<uint8_t, 30>();
    uint8_t& v_analogButtonX = payloadView<uint8_t, 31>();
    uint8_t& v_analogButtonR1 = payloadView<uint8_t, 32>();
    uint8_t& v_analogButtonL1 = payloadView<uint8_t, 33>();
    uint8_t& v_analogButtonR2 = payloadView<uint8_t, 34>();
    uint8_t& v_analogButtonL2 = payloadView<uint8_t, 35>();
    uint8_t& v_touch1Active = payloadView<uint8_t, 36>();
    uint8_t& v_touch1Id = payloadView<uint8_t, 36>();
    uint16_t& v_touch1X = payloadView<uint16_t, 36>();
    uint16_t& v_touch1Y = payloadView<uint16_t, 36>();
    uint8_t& v_touch2Active = payloadView<uint8_t, 36>();
    uint8_t& v_touch2Id = payloadView<uint8_t, 36>();
    uint16_t& v_touch2X = payloadView<uint16_t, 36>();
    uint16_t& v_touch2Y = payloadView<uint16_t, 36>();
    uint64_t& v_motionTimestamp = payloadView<uint64_t, 48>();
    float& v_accelX = payloadView<float, 56>();
    float& v_accelY = payloadView<float, 60>();
    float& v_accelZ = payloadView<float, 64>();
    float& v_gyroPitch = payloadView<float, 68>();
    float& v_gyroYaw = payloadView<float, 72>();
    float& v_gyroRoll = payloadView<float, 76>();

    constexpr CemuhookDataMessage(uint32_t id)
      : CemuhookOutgoingMessage(id, 0x100002){};
};

#include "cemuhook_protocol_defs.hpp"