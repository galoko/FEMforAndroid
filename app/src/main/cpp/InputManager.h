#ifndef FEMFORANDROID_INPUT_MANAGER_H
#define FEMFORANDROID_INPUT_MANAGER_H

#include <android/sensor.h>

#include "EigenTypes.h"

class InputManager {
public:
    static InputManager& getInstance() {
        static InputManager instance;

        return instance;
    }

    InputManager(InputManager const&)          = delete;
    void operator=(InputManager const&)  = delete;
private:
    int initialized;

    const int SENSOR_REFRESH_RATE_HZ = 100;
    const int32_t SENSOR_REFRESH_PERIOD_US = int32_t(1000000 / SENSOR_REFRESH_RATE_HZ);
    const float SENSOR_FILTER_ALPHA = 0.1f;

    EigenVector3 sensorDataFilter;

    ASensorEventQueue *accelerometerEventQueue;
public:
    InputManager();

    void initialize();
    void finalize();

    void applyUserInput();
};

#endif //FEMFORANDROIDINPUT_MANAGER_H
