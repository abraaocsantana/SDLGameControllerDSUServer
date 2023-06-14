#pragma once

#include <SDL.h>

#include "cemuhook_protocol.hpp"

class CemuhookControllerData
{
    CemuhookDataMessage m_cemuhookControllerDataState;
    unsigned m_stuckCount = 0;

  public:
    CemuhookControllerData(uint32_t service_id)
      : m_cemuhookControllerDataState{ service_id } {};

    CemuhookDataMessage& data();

    void setConnected(bool connected);

    void sdlSensorToData(uint16_t controllerIndex,
                         SDL_SensorType sensortype,
                         float data1,
                         float data2,
                         float data3,
                         uint64_t timestamp_us,
                         bool& sensorStuck);
};