#include <SDL.h>
#include <SDL_stdinc.h>

#include "cemuhook_protocol.hpp"
#include "cemuhook_controller_data.hpp"
#include "udp_server.hpp"

#define BUTTON_SIZE 50
#define AXIS_SIZE 50

#define BUTTON_SIZE 50
#define AXIS_SIZE 50

/* This is indexed by SDL_JoystickPowerLevel + 1. */
static const char* power_level_strings[] = {
    "unknown", /* SDL_JOYSTICK_POWER_UNKNOWN */
    "empty",   /* SDL_JOYSTICK_POWER_EMPTY */
    "low",     /* SDL_JOYSTICK_POWER_LOW */
    "medium",  /* SDL_JOYSTICK_POWER_MEDIUM */
    "full",    /* SDL_JOYSTICK_POWER_FULL */
    "wired",   /* SDL_JOYSTICK_POWER_WIRED */
};
SDL_COMPILE_TIME_ASSERT(power_level_strings,
                        SDL_arraysize(power_level_strings) ==
                          SDL_JOYSTICK_POWER_MAX + 1);

static SDL_bool retval = SDL_FALSE;
static SDL_bool done = SDL_FALSE;
static SDL_bool set_LED = SDL_FALSE;
static int trigger_effect = 0;
static SDL_Texture *background_front, *background_back, *button_texture,
  *axis_texture;
static SDL_GameController* gamecontroller;
static SDL_GameController** gamecontrollers;
static int num_controllers = 0;
static SDL_Joystick* virtual_joystick = NULL;
static SDL_GameControllerAxis virtual_axis_active = SDL_CONTROLLER_AXIS_INVALID;
static int virtual_axis_start_x;
static int virtual_axis_start_y;
static SDL_GameControllerButton virtual_button_active =
  SDL_CONTROLLER_BUTTON_INVALID;

static const char*
GetSensorName(SDL_SensorType sensor)
{
    switch (sensor) {
        case SDL_SENSOR_ACCEL:
            return "accelerometer";
        case SDL_SENSOR_GYRO:
            return "gyro";
        case SDL_SENSOR_ACCEL_L:
            return "accelerometer (L)";
        case SDL_SENSOR_GYRO_L:
            return "gyro (L)";
        case SDL_SENSOR_ACCEL_R:
            return "accelerometer (R)";
        case SDL_SENSOR_GYRO_R:
            return "gyro (R)";
        default:
            return "UNKNOWN";
    }
}

static int
FindController(SDL_JoystickID controller_id)
{
    int i;

    for (i = 0; i < num_controllers; ++i) {
        if (controller_id ==
            SDL_JoystickInstanceID(
              SDL_GameControllerGetJoystick(gamecontrollers[i]))) {
            return i;
        }
    }
    return -1;
}

static void
AddController(int device_index, SDL_bool verbose)
{
    SDL_JoystickID controller_id =
      SDL_JoystickGetDeviceInstanceID(device_index);
    SDL_GameController* controller;
    SDL_GameController** controllers;
    Uint16 firmware_version;
    SDL_SensorType sensors[] = { SDL_SENSOR_ACCEL,   SDL_SENSOR_GYRO,
                                 SDL_SENSOR_ACCEL_L, SDL_SENSOR_GYRO_L,
                                 SDL_SENSOR_ACCEL_R, SDL_SENSOR_GYRO_R };
    unsigned int i;

    controller_id = SDL_JoystickGetDeviceInstanceID(device_index);
    SDL_Log("ID: %i\n", controller_id);
    if (controller_id < 0) {
        SDL_Log("Couldn't get controller ID: %s\n", SDL_GetError());
        return;
    }

    if (FindController(controller_id) >= 0) {
        /* We already have this controller */
        return;
    }

    controller = SDL_GameControllerOpen(device_index);
    if (controller == NULL) {
        SDL_Log("Couldn't open controller: %s\n", SDL_GetError());
        return;
    }

    controllers = (SDL_GameController**)SDL_realloc(
      gamecontrollers, (num_controllers + 1) * sizeof(*controllers));
    if (controllers == NULL) {
        SDL_GameControllerClose(controller);
        return;
    }

    controllers[num_controllers++] = controller;
    gamecontrollers = controllers;
    gamecontroller = controller;
    trigger_effect = 0;

    if (verbose) {
        const char* name = SDL_GameControllerName(gamecontroller);
        const char* path = SDL_GameControllerPath(gamecontroller);
        SDL_Log("Opened game controller %s%s%s\n",
                name,
                path ? ", " : "",
                path ? path : "");
    }

    firmware_version = SDL_GameControllerGetFirmwareVersion(gamecontroller);
    if (firmware_version) {
        if (verbose) {
            SDL_Log("Firmware version: 0x%x (%d)\n",
                    firmware_version,
                    firmware_version);
        }
    }

    for (i = 0; i < SDL_arraysize(sensors); ++i) {
        SDL_SensorType sensor = sensors[i];

        if (SDL_GameControllerHasSensor(gamecontroller, sensor)) {
            if (verbose) {
                SDL_Log(
                  "Enabling %s at %.2f Hz\n",
                  GetSensorName(sensor),
                  SDL_GameControllerGetSensorDataRate(gamecontroller, sensor));
            }
            SDL_GameControllerSetSensorEnabled(
              gamecontroller, sensor, SDL_TRUE);
        }
    }

    if (SDL_GameControllerHasRumble(gamecontroller)) {
        SDL_Log("Rumble supported");
    }

    if (SDL_GameControllerHasRumbleTriggers(gamecontroller)) {
        SDL_Log("Trigger rumble supported");
    }
}

static void
SetController(SDL_JoystickID controller)
{
    int i = FindController(controller);

    if (i < 0) {
        return;
    }

    if (gamecontroller != gamecontrollers[i]) {
        gamecontroller = gamecontrollers[i];
    }
}

static void
DelController(SDL_JoystickID controller)
{
    int i = FindController(controller);

    if (i < 0) {
        return;
    }

    SDL_GameControllerClose(gamecontrollers[i]);

    --num_controllers;
    if (i < num_controllers) {
        SDL_memcpy(&gamecontrollers[i],
                   &gamecontrollers[i + 1],
                   (num_controllers - i) * sizeof(*gamecontrollers));
    }

    if (num_controllers > 0) {
        gamecontroller = gamecontrollers[0];
    } else {
        gamecontroller = NULL;
    }
}

static void
ResetController(int controllerIndex)
{
    if (SDL_GameControllerGetAttached(
          SDL_GameControllerFromPlayerIndex(controllerIndex))) {
        DelController(controllerIndex);
        AddController(controllerIndex, SDL_TRUE);
    }
}

void
loop(std::shared_ptr<UdpServer> p_updServer)
{
    SDL_Event event;

    static CemuhookControllerData cemuhookControllerDataState{p_updServer->id()};

    auto tickStart = SDL_GetTicks();

    /* Update to get the current event state */
    SDL_PumpEvents();

    cemuhookControllerDataState.setConnected( SDL_NumJoysticks() > 0 ? 1 : 0 );

    /* Process all currently pending events */
    while (SDL_PeepEvents(
             &event, 1, SDL_GETEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT) == 1) {

        switch (event.type) {

            case SDL_CONTROLLERDEVICEADDED:
                SDL_Log(
                  "Game controller device %d added.\n",
                  (int)SDL_JoystickGetDeviceInstanceID(event.cdevice.which));
                AddController(event.cdevice.which, SDL_TRUE);
                break;

            case SDL_CONTROLLERDEVICEREMOVED:
                SDL_Log("Game controller device %d removed.\n",
                        (int)event.cdevice.which);
                DelController(event.cdevice.which);
                break;

            case SDL_CONTROLLERTOUCHPADDOWN:
            case SDL_CONTROLLERTOUCHPADMOTION:
            case SDL_CONTROLLERTOUCHPADUP:
                SDL_Log(
                  "Controller %" SDL_PRIs32 " touchpad %" SDL_PRIs32
                  " finger %" SDL_PRIs32 " %s %.2f, %.2f, %.2f\n",
                  event.ctouchpad.which,
                  event.ctouchpad.touchpad,
                  event.ctouchpad.finger,
                  (event.type == SDL_CONTROLLERTOUCHPADDOWN
                     ? "pressed at"
                     : (event.type == SDL_CONTROLLERTOUCHPADUP ? "released at"
                                                               : "moved to")),
                  event.ctouchpad.x,
                  event.ctouchpad.y,
                  event.ctouchpad.pressure);
                break;

            case SDL_CONTROLLERSENSORUPDATE:
#ifdef VERBOSE_SENSORS
                SDL_Log("Controller %" SDL_PRIs32
                        " sensor %s: %.2f, %.2f, %.2f (%" SDL_PRIu64 ")\n",
                        event.csensor.which,
                        GetSensorName((SDL_SensorType)event.csensor.sensor),
                        event.csensor.data[0],
                        event.csensor.data[1],
                        event.csensor.data[2],
                        event.csensor.timestamp_us);
#endif /* VERBOSE_SENSORS */

                bool sensorStuck;

                cemuhookControllerDataState.sdlSensorToData(
                  event.csensor.which,
                  (SDL_SensorType)event.csensor.sensor,
                  event.csensor.data[0],
                  event.csensor.data[1],
                  event.csensor.data[2],
                  event.csensor.timestamp_us,
                  sensorStuck);

                if (sensorStuck) {
                    ResetController(event.csensor.which);
                }

                break;

            case SDL_CONTROLLERAXISMOTION:
                if (event.caxis.value <= (-SDL_JOYSTICK_AXIS_MAX / 2) ||
                    event.caxis.value >= (SDL_JOYSTICK_AXIS_MAX / 2)) {
                    SetController(event.caxis.which);
                }
#ifdef VERBOSE_AXES
                SDL_Log("Controller %" SDL_PRIs32 " axis %s changed to %d\n",
                        event.caxis.which,
                        SDL_GameControllerGetStringForAxis(
                          (SDL_GameControllerAxis)event.caxis.axis),
                        event.caxis.value);
#endif /* VERBOSE_AXES */
                break;

            case SDL_CONTROLLERBUTTONDOWN:
            case SDL_CONTROLLERBUTTONUP:
                if (event.type == SDL_CONTROLLERBUTTONDOWN) {
                    SetController(event.cbutton.which);
                }
                SDL_Log("Controller %" SDL_PRIs32 " button %s %s\n",
                        event.cbutton.which,
                        SDL_GameControllerGetStringForButton(
                          (SDL_GameControllerButton)event.cbutton.button),
                        event.cbutton.state ? "pressed" : "released");
                break;

            case SDL_JOYBATTERYUPDATED:
                SDL_Log("Controller %" SDL_PRIs32
                        " battery state changed to %s\n",
                        event.jbattery.which,
                        power_level_strings[event.jbattery.level + 1]);
                break;

            case SDL_KEYDOWN:
                if (event.key.keysym.sym >= SDLK_0 &&
                    event.key.keysym.sym <= SDLK_9) {
                    if (gamecontroller) {
                        int player_index = (event.key.keysym.sym - SDLK_0);

                        SDL_GameControllerSetPlayerIndex(gamecontroller,
                                                         player_index);
                    }
                    break;
                }
                if (event.key.keysym.sym == SDLK_a) {
                    break;
                }
                if (event.key.keysym.sym == SDLK_d) {
                    break;
                }
                if (event.key.keysym.sym != SDLK_ESCAPE) {
                    break;
                }
                SDL_FALLTHROUGH;
            case SDL_QUIT:
                done = SDL_TRUE;
                break;
            default:
                break;
        }
    }

    p_updServer->sendData(cemuhookControllerDataState.data());

    auto elapsedTicks = (SDL_GetTicks() - tickStart);
    uint32_t delay = 32ul;
    SDL_Delay(elapsedTicks > delay ? 0 : delay - elapsedTicks);
}

void
inputStart(std::shared_ptr<UdpServer> p_updServer)
{
    int controller_count = 0;
    int controller_index = 0;
    char guid[64];

    SDL_SetHint(SDL_HINT_ACCELEROMETER_AS_JOYSTICK, "0");
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_PS4_RUMBLE, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_PS5_RUMBLE, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_STEAM, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_ROG_CHAKRAM, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
    SDL_SetHint(SDL_HINT_LINUX_JOYSTICK_DEADZONES, "1");

    /* Enable standard application logging */
    // SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO);

    /* Initialize SDL (Note: video is required to start event loop) */
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER) <
        0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "Couldn't initialize SDL: %s\n",
                     SDL_GetError());
        exit(1);
    }

    // SDL_GameControllerAddMappingsFromFile("gamecontrollerdb.txt");

    ///* Print information about the mappings */
    // if (argv[1] && SDL_strcmp(argv[1], "--mappings") == 0) {
    //   SDL_Log("Supported mappings:\n");
    //   for (int i = 0; i < SDL_GameControllerNumMappings(); ++i) {
    //     char* mapping = SDL_GameControllerMappingForIndex(i);
    //     if (mapping) {
    //       SDL_Log("\t%s\n", mapping);
    //       SDL_free(mapping);
    //     }
    //   }
    //   SDL_Log("\n");
    // }

    /* Print information about the controller */
    for (int i = 0; i < SDL_NumJoysticks(); ++i) {
        const char* name;
        const char* path;
        const char* description;

        SDL_JoystickGetGUIDString(
          SDL_JoystickGetDeviceGUID(i), guid, sizeof(guid));

        if (SDL_IsGameController(i)) {
            controller_count++;
            name = SDL_GameControllerNameForIndex(i);
            path = SDL_GameControllerPathForIndex(i);
            switch (SDL_GameControllerTypeForIndex(i)) {
                case SDL_CONTROLLER_TYPE_AMAZON_LUNA:
                    description = "Amazon Luna Controller";
                    break;
                case SDL_CONTROLLER_TYPE_GOOGLE_STADIA:
                    description = "Google Stadia Controller";
                    break;
                case SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_JOYCON_LEFT:
                case SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_JOYCON_RIGHT:
                case SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_JOYCON_PAIR:
                    description = "Nintendo Switch Joy-Con";
                    break;
                case SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_PRO:
                    description = "Nintendo Switch Pro Controller";
                    break;
                case SDL_CONTROLLER_TYPE_PS3:
                    description = "PS3 Controller";
                    break;
                case SDL_CONTROLLER_TYPE_PS4:
                    description = "PS4 Controller";
                    break;
                case SDL_CONTROLLER_TYPE_PS5:
                    description = "PS5 Controller";
                    break;
                case SDL_CONTROLLER_TYPE_XBOX360:
                    description = "XBox 360 Controller";
                    break;
                case SDL_CONTROLLER_TYPE_XBOXONE:
                    description = "XBox One Controller";
                    break;
                case SDL_CONTROLLER_TYPE_VIRTUAL:
                    description = "Virtual Game Controller";
                    break;
                default:
                    description = "Game Controller";
                    break;
            }
            AddController(i, SDL_FALSE);
        } else {
            name = SDL_JoystickNameForIndex(i);
            path = SDL_JoystickPathForIndex(i);
            description = "Joystick";
        }
        SDL_Log("%s %d: %s%s%s (guid %s, VID 0x%.4x, PID 0x%.4x, player index "
                "= %d)\n",
                description,
                i,
                name ? name : "Unknown",
                path ? ", " : "",
                path ? path : "",
                guid,
                SDL_JoystickGetDeviceVendor(i),
                SDL_JoystickGetDeviceProduct(i),
                SDL_JoystickGetDevicePlayerIndex(i));
    }
    SDL_Log("There are %d game controller(s) attached (%d joystick(s))\n",
            controller_count,
            SDL_NumJoysticks());

    if (controller_index < num_controllers) {
        gamecontroller = gamecontrollers[controller_index];
    } else {
        gamecontroller = NULL;
    }

    /* Loop, getting controller events! */
    while (!done) {
        loop(p_updServer);
    }

    SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK |
                      SDL_INIT_GAMECONTROLLER);
}