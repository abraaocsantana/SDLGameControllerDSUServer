#include "cemuhook_controller_data.hpp"

CemuhookDataMessage&
CemuhookControllerData::data()
{
    return m_cemuhookControllerDataState;
}

void
CemuhookControllerData::setConnected(bool connected)
{
    m_cemuhookControllerDataState.v_connected = connected;
}

void
CemuhookControllerData::sdlSensorToData(uint16_t controllerIndex,
                                        SDL_SensorType sensortype,
                                        float data1,
                                        float data2,
                                        float data3,
                                        uint64_t timestamp_us,
                                        bool& sensorStuck)
{
    sensorStuck = false;

    float nAccelX;
    float nAccelY;
    float nAccelZ;

    switch (sensortype) {
        case SDL_SensorType::SDL_SENSOR_ACCEL:

            nAccelX = data1 / SDL_STANDARD_GRAVITY;
            nAccelY = data2 / SDL_STANDARD_GRAVITY;
            nAccelZ = data3 / SDL_STANDARD_GRAVITY;

            if ((nAccelX == m_cemuhookControllerDataState.v_accelX) &&
                (nAccelY == m_cemuhookControllerDataState.v_accelY) &&
                (nAccelZ == m_cemuhookControllerDataState.v_accelZ)) {
                ++m_stuckCount;
                if (m_stuckCount > 3) {
                    printf("Sensor is stuck!\n");
                    sensorStuck = true;
                    m_stuckCount = 0;
                }
            } else {
                m_stuckCount = 0;
            }

            m_cemuhookControllerDataState.v_accelX = nAccelX;
            m_cemuhookControllerDataState.v_accelY = nAccelY;
            m_cemuhookControllerDataState.v_accelZ = nAccelZ;
            m_cemuhookControllerDataState.v_motionTimestamp = timestamp_us;

            break;
        case SDL_SensorType::SDL_SENSOR_GYRO:
            m_cemuhookControllerDataState.v_gyroPitch =
              data1 / ((float)M_PI / 180.0f);
            m_cemuhookControllerDataState.v_gyroRoll =
              data3 / ((float)M_PI / 180.0f);
            m_cemuhookControllerDataState.v_gyroYaw =
              data2 / ((float)M_PI / 180.0f);

            break;
        default:
            break;
    };
}