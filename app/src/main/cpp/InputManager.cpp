#include "InputManager.h"

#include "log.h"
#include "exceptionUtils.h"

#include "Physics.h"
#include "Render.h"

#define INPUT_MANAGER_TAG "PT_INPUT_MANAGER"

InputManager::InputManager() {

}

void InputManager::initialize() {

    if (this->initialized == 1)
        return;

    // init accelerometer sensor

    ASensorManager* sensorManager = ASensorManager_getInstance();
    my_assert(sensorManager != nullptr);

    const ASensor* accelerometer = ASensorManager_getDefaultSensor(sensorManager, ASENSOR_TYPE_ACCELEROMETER);
    my_assert(accelerometer != nullptr);

    ALooper* looper = ALooper_prepare(ALOOPER_PREPARE_ALLOW_NON_CALLBACKS);
    my_assert(looper != nullptr);

    accelerometerEventQueue = ASensorManager_createEventQueue(sensorManager, looper, 3, nullptr, nullptr);
    my_assert(accelerometerEventQueue != nullptr);

    auto status = ASensorEventQueue_enableSensor(accelerometerEventQueue, accelerometer);
    my_assert(status >= 0);

    status = ASensorEventQueue_setEventRate(accelerometerEventQueue, accelerometer, SENSOR_REFRESH_PERIOD_US);
    my_assert(status >= 0);

    this->initialized = 1;

    print_log(ANDROID_LOG_INFO, INPUT_MANAGER_TAG, "Input manager is initialized");
}

void InputManager::finalize() {

    if (this->initialized == 0)
        return;

    this->initialized = 0;

    print_log(ANDROID_LOG_INFO, INPUT_MANAGER_TAG, "Input manager is finalized");
}

void InputManager::applyUserInput() {

    ALooper_pollAll(0, nullptr, nullptr, nullptr);
    ASensorEvent event;
    float a = SENSOR_FILTER_ALPHA;
    while (ASensorEventQueue_getEvents(accelerometerEventQueue, &event, 1) > 0) {
        EigenVector3 eventAcceleration = EigenVector3(event.acceleration.x, event.acceleration.y, event.acceleration.z);

        sensorDataFilter = eventAcceleration;
    }

    EigenVector3 rotatedVec = EigenVector3(-sensorDataFilter.x(), sensorDataFilter.z(), -sensorDataFilter.y());

    // print_log(ANDROID_LOG_INFO, INPUT_MANAGER_TAG, "%f %f %f", rotatedVec.x, rotatedVec.y, rotatedVec.z);

    /*
    rotatedVec.y = (std::min(std::max(rotatedVec.y, 0.0f), 9.8f) - 4.9f) * 2.5f;
    if (fabs(rotatedVec.y) < 3.5f)
        rotatedVec.y = 0.0f;
    rotatedVec.x = std::min(std::max(rotatedVec.x * 2.0f, -9.8f), 9.8f);
     */

    EigenVector3 zAxis = EigenVector3(0, 0, -1);
    float angle = Render::getInstance().getCameraZAngle();
    EigenQuaternion cameraZrotate = EigenQuaternion(EigenAngleAxis(angle, zAxis));

    rotatedVec = cameraZrotate * rotatedVec;

    // print_log(ANDROID_LOG_INFO, INPUT_MANAGER_TAG, "%f %f %f", rotatedVec.x, rotatedVec.y, rotatedVec.z);

    Physics::getInstance().setGravity(rotatedVec);
}