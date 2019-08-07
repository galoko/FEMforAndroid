#ifndef FEMFORANDROID_RENDER_H
#define FEMFORANDROID_RENDER_H

#include <android/native_window.h>

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <string>

#include <glm/glm.hpp>

#include "Physics.h"

#include "AssetManager.h"

using namespace std;
using namespace glm;

class Render {
public:
    static Render& getInstance() {
        static Render instance;

        return instance;
    }

    Render(Render const&) = delete;
    void operator=(Render const&)  = delete;
private:
    Render();

    int initialized;

    ANativeWindow* window;

    void initializeWindow();
    void finalizeWindow();

    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;
    EGLint width, height;

    void initializeEGL();
    void finalizeEGL();

    GLuint program, positionIndex, normalIndex, texCoordIndex;

    GLuint modelTexture, wallTexture;

    GLint projectionID, viewID, texID;

    vec3 cameraPosition;
    float cameraAngleX, cameraAngleZ;

    const vec3 up = { 0, 0, 1 };

    void initializeGL();
    void finalizeGL();

    void updateProjectionMatrix();
    void updateViewMatrix();

    void lookAtPoint(const vec3 point);

    void initVertexBuffer(GLuint buffer);
    void drawAsset(GPUAsset* asset, GLuint tex);
public:
    void initialize();
    void finalize();

    void setOutputWindow(ANativeWindow* window);

    float getCameraXAngle();
    float getCameraZAngle();

    void draw();
};

#endif //FEMFORANDROID_RENDER_H
