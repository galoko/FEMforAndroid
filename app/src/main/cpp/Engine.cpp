#include "Engine.h"

#include <unistd.h>

#include "log.h"
#include "exceptionUtils.h"

#include "AssetManager.h"
#include "Physics.h"
#include "Render.h"
#include "InputManager.h"

extern "C" {
#include "generalUtils.h"
}

#define ENGINE_TAG "PT_ENGINE"

Engine::Engine() : eventQueue(30) {

}

void Engine::initialize(AAssetManager* nativeAssetManager, string externalFilesDir) {

    if (this->initialized == 1)
        return;

    this->finalized = false;
    this->started = false;

    pthread_check_error(pthread_create(&thread, nullptr, thread_entrypoint, nullptr));

    auto initStruct = new InitStruct();
    initStruct->nativeAssetManager = nativeAssetManager;
    initStruct->externalFilesDir = externalFilesDir;

    pushEvent(Initialize, (void*)initStruct);

    this->initialized = 1;

    print_log(ANDROID_LOG_INFO, ENGINE_TAG, "Engine is initialized");
}

void Engine::finalize() {

    if (this->initialized == 0)
        return;

    pushEvent(Finalize);

    pthread_check_error(pthread_join(thread, nullptr));

    this->initialized = 0;

    print_log(ANDROID_LOG_INFO, ENGINE_TAG, "Engine is finalized");
}

void Engine::start() {
    pushEvent(Start);
}

void Engine::stop() {
    pushEvent(Stop);
}

void Engine::setOutputWindow(ANativeWindow* window) {
    pushEvent(SetOutputWindow, (void*)window);
}

// thread

void* Engine::thread_entrypoint(void* opaque) {

    Engine::getInstance().threadLoop();
    return nullptr;
}

void Engine::threadLoop() {

    while (true) {
        processEvents();
        if (finalized)
            break;

        my_assert(started);

        InputManager::getInstance().applyUserInput();
        Render::getInstance().draw();
    }
}

// messages

void Engine::pushEvent(EventMessage message, void *param) {

    EngineEvent event = { message, param };
    eventQueue.enqueue(event);
}

void Engine::processEvents() {

    EngineEvent event;

    while (!finalized) {
        // if loop is started
        if (started) {
            // then quickly process all events
            while (started && eventQueue.try_dequeue(event))
                processEvent(event);
            // still started
            if (started)
                return;
            // if not -> go to loop again
        }
        else {
            if (eventQueue.wait_dequeue_timed(event, -1))
                processEvent(event);
            else
                return; // ?
        }
    }
}

void Engine::processEvent(EngineEvent& event) {

    char* eventNames[5] = {
        "Initialize",
        "Finalize",
        "Start",
        "Stop",
        "SetOutputWindow"
    };
    print_log(ANDROID_LOG_INFO, ENGINE_TAG, "Event: %s", eventNames[event.message]);

    switch (event.message) {
        case Initialize: {
            nice(-20);

            InitStruct* initStruct = (InitStruct*)event.param;

            AssetManager::getInstance().initialize(initStruct->nativeAssetManager, initStruct->externalFilesDir);
            Render::getInstance().initialize();
            Physics::getInstance().initialize();
            InputManager::getInstance().initialize();

            delete initStruct;

            break;
        }
        case Finalize:
            InputManager::getInstance().finalize();
            Render::getInstance().finalize();
            Physics::getInstance().finalize();
            AssetManager::getInstance().finalize();
            finalized = true;
            break;
        case Start:
            started = true;
            Physics::getInstance().start();
            break;
        case Stop:
            Physics::getInstance().stop();
            started = false;
            break;
        case SetOutputWindow:
            Render::getInstance().setOutputWindow((ANativeWindow*)event.param);
            break;
        default:
            my_assert(false);
            break;
    }
}