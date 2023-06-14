#include "cemuhook_protocol.hpp"
#include "crc32.h"

template<uint16_t DataLength>
template<typename T, std::size_t Index, std::size_t IndexStart>
constexpr T&
CemuhookMessage<DataLength>::view()
{
    return reinterpret_cast<T&>(m_data[IndexStart + Index]);
}

template<uint16_t DataLength>
template<typename T, std::size_t Index, std::size_t IndexStart>
constexpr T&
CemuhookMessage<DataLength>::payloadView()
{
    return view<T, Index + IndexStart, HeaderLenght>();
}

template<uint16_t DataLength>
inline void
CemuhookMessage<DataLength>::genCRC32()
{
    v_crc32 = (uint32_t)0u;
    v_crc32 = calcCrc32(m_data.data(), FullLength());
};

template<uint16_t DataLength>
inline bool
CemuhookMessage<DataLength>::checkCRC32()
{
    uint32_t crc32 = v_crc32;
    genCRC32();
    uint32_t computedCrc32 = v_crc32;
    v_crc32 = crc32;
    return crc32 == computedCrc32;
};

template<uint16_t DataLength>
constexpr CemuhookMessage<DataLength>::CemuhookMessage(uint32_t id,
                                                       uint32_t eventType)
{
    m_data[0] = 'D';
    m_data[1] = 'S';
    m_data[2] = 'U';
    m_data[3] = 'S';
    v_protocol = 1001ul;
    v_dataLength = (uint16_t)(DataLength + sizeof(eventType));
    v_crc32 = 0ull; // CRC32;
    v_id = id;
    v_eventType = eventType;
};

template<uint16_t DataLength>
constexpr CemuhookOutgoingMessage<DataLength>::CemuhookOutgoingMessage(
  uint32_t id,
  uint32_t eventType)
  : CemuhookMessage(id, eventType)
{
    v_slot = 0;
    v_state = 2;
    v_modelGyro = 2;
    v_connectionType = 1;
    v_macAddress = 0x545200c8aac1;
    v_batteryStatus = 5322;
    // TEST VALS
};
