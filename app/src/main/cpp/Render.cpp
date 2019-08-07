#include "Render.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "log.h"
#include "exceptionUtils.h"

extern "C" {
#include "generalUtils.h"
}

#define RENDER_TAG "PT_RENDER"

Render::Render() {

}

void Render::initialize() {

    if (this->initialized == 1)
        return;

    this->initialized = 1;
}

void Render::finalize() {

    if (this->initialized == 0)
        return;

    setOutputWindow(nullptr);

    this->initialized = 0;
}

void Render::setOutputWindow(ANativeWindow* window) {

    if (this->window == window) {
        if (window != nullptr)
            ANativeWindow_release(window);
        return;
    }

    if (this->window) {
        finalizeWindow();

        ANativeWindow_release(this->window);
        this->window = nullptr;
    }

    this->window = window;

    if (this->window) {
        initializeWindow();
    }
}

void Render::initializeWindow() {

    initializeEGL();
    initializeGL();

    print_log(ANDROID_LOG_INFO, RENDER_TAG, "Render is initialized");
}

void Render::finalizeWindow() {

    finalizeGL();
    finalizeEGL();

    print_log(ANDROID_LOG_INFO, RENDER_TAG, "Render is finalized");
}

void Render::initializeEGL() {
    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglCheckError(display != EGL_NO_DISPLAY, "eglGetDisplay");

    eglCheckError(eglInitialize(display, nullptr, nullptr) == EGL_TRUE, "eglInitialize");

    const EGLint configAttributes[] = {
            EGL_SURFACE_TYPE,    /* = */ EGL_WINDOW_BIT,
            EGL_RENDERABLE_TYPE, /* = */ 0,
            EGL_DEPTH_SIZE,      /* = */ 24,
            EGL_BLUE_SIZE,       /* = */ 8,
            EGL_GREEN_SIZE,      /* = */ 8,
            EGL_RED_SIZE,        /* = */ 8,
            EGL_CONFORMANT,      /* = */ EGL_OPENGL_ES2_BIT,
            EGL_NONE
    };

    EGLConfig config;
    EGLint numConfigs;
    eglCheckError(eglChooseConfig(display, configAttributes, &config, 1, &numConfigs) == EGL_TRUE, "eglChooseConfig");

    eglCheckError(numConfigs == 1, "eglChooseConfig.numConfigs");

    EGLint format;
    eglCheckError(eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format) == EGL_TRUE, "eglGetConfigAttrib");

    my_assert(ANativeWindow_setBuffersGeometry(window, 0, 0, format) == 0);

    EGLSurface surface = eglCreateWindowSurface(display, config, window, nullptr);
    eglCheckError(surface != EGL_NO_SURFACE, "eglCreateWindowSurface");

    const EGLint contextAttributes[] = {
            EGL_CONTEXT_CLIENT_VERSION, 2,
            EGL_NONE
    };
    EGLContext context = eglCreateContext(display, config, nullptr, contextAttributes);
    eglCheckError(context != EGL_NO_CONTEXT, "eglCreateContext");

    eglCheckError(eglMakeCurrent(display, surface, surface, context) == EGL_TRUE, "eglMakeCurrent");

    EGLint width;
    eglCheckError(eglQuerySurface(display, surface, EGL_WIDTH, &width) == EGL_TRUE, "eglQuerySurface.EGL_WIDTH");

    EGLint height;
    eglCheckError(eglQuerySurface(display, surface, EGL_HEIGHT, &height) == EGL_TRUE, "eglQuerySurface.EGL_HEIGHT");

    this->display = display;
    this->surface = surface;
    this->context = context;

    this->width = width;
    this->height = height;
}

void Render::finalizeEGL() {

    eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(display, context);
    eglDestroySurface(display, surface);
    eglTerminate(display);

    display = EGL_NO_DISPLAY;
    surface = EGL_NO_SURFACE;
    context = EGL_NO_CONTEXT;

    width = 0;
    height = 0;
}

void Render::initializeGL() {

    glViewport(0, 0, width, height);

    // TODO
    glDisable(GL_CULL_FACE);
    // glFrontFace(GL_CCW);
    // glCullFace(GL_BACK);

    glEnable(GL_DEPTH_TEST);

    // shaders

    string vertexShaderCode = AssetManager::getInstance().loadTextAsset("scene.vertexshader");
    string fragmentShaderCode = AssetManager::getInstance().loadTextAsset("scene.fragmentshader");

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

    char const * codePointer;

    codePointer = vertexShaderCode.c_str();
    glShaderSource(vertexShader, 1, &codePointer, nullptr);

    codePointer = fragmentShaderCode.c_str();
    glShaderSource(fragmentShader, 1, &codePointer, nullptr);

    GLint res = GL_FALSE;

    glCompileShader(vertexShader);
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &res);
    my_assert(res == GL_TRUE);

    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &res);
    my_assert(res == GL_TRUE);

    this->program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    glGetProgramiv(program, GL_LINK_STATUS, &res);
    my_assert(res == GL_TRUE);

    glDetachShader(program, vertexShader);
    glDetachShader(program, fragmentShader);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    glUseProgram(program);

    positionIndex = glGetAttribLocation(program, "vertexPosition");
    normalIndex = glGetAttribLocation(program, "vertexNormal");
    texCoordIndex = glGetAttribLocation(program, "vertexTexCoord");

    // color

    glClearColor(1, 0, 0, 1);

    // uniforms

    projectionID = glGetUniformLocation(program, "projection");
    viewID = glGetUniformLocation(program, "view");

    texID = glGetUniformLocation(program, "tex");

    // setup texture

    glActiveTexture(GL_TEXTURE0);
    glUniform1i(texID, 0);

    // textures

    modelTexture = AssetManager::getInstance().loadTextureAsset("model.bmp");
    wallTexture = AssetManager::getInstance().loadTextureAsset("wall.bmp");

    // setup matrices

    updateProjectionMatrix();

    cameraPosition = { 2, 2, 2 };

    lookAtPoint(vec3(0, 0, 0));

    updateViewMatrix();
}

void Render::finalizeGL() {

    glDeleteProgram(program);
    program = 0;

    glDeleteTextures(1, &modelTexture);
    modelTexture = 0;

    glDeleteTextures(1, &wallTexture);
    wallTexture = 0;

    MeshAsset* walls = Physics::getInstance().getWalls();
    if (walls)
        walls->invalidate(true);

    TetAsset* model = Physics::getInstance().getModel();
    if (model)
        model->invalidate(true);
}

void Render::updateProjectionMatrix() {
    float aspectRatio = (float)width / (float)height;
    mat4 projection = perspective(radians(45.0f), aspectRatio, 0.1f, 100.0f);
    glUniformMatrix4fv(projectionID, 1, GL_FALSE, value_ptr(projection));
}

void Render::updateViewMatrix() {

    mat4 view;

    // limit angles
    cameraAngleZ /= (float)M_PI * 2.0f;
    cameraAngleZ -= (float)(long)cameraAngleZ;
    cameraAngleZ *= (float)M_PI * 2.0f;

    cameraAngleX = std::min(std::max(0.1f, cameraAngleX), 3.13f);

    vec3 direction;

    float cameraAngleXsin, cameraAngleXcos, cameraAngleZsin, cameraAngleZcos;
    sincosf(cameraAngleX, &cameraAngleXsin, &cameraAngleXcos);
    sincosf(cameraAngleZ, &cameraAngleZsin, &cameraAngleZcos);

    direction.x = cameraAngleXsin * cameraAngleZsin;
    direction.y = cameraAngleXsin * cameraAngleZcos;
    direction.z = cameraAngleXcos;

    view = lookAt(cameraPosition, cameraPosition + direction, up);

    glUniformMatrix4fv(viewID, 1, GL_FALSE, value_ptr(view));
}

void Render::lookAtPoint(const vec3 point) {

    vec3 direction = normalize(point - cameraPosition);

    float sinX = sqrt(1 - direction.z * direction.z);

    cameraAngleZ = atan2(direction.x / sinX, direction.y / sinX);
    cameraAngleX = atan2(sinX, direction.z);

    updateViewMatrix();
}

void Render::initVertexBuffer(GLuint buffer) {

    const unsigned int SIZE_OF_VERTEX = (3 + 3 + 2) * sizeof(float);

    glEnableVertexAttribArray(positionIndex);
    glVertexAttribPointer(positionIndex, 3, GL_FLOAT, GL_FALSE, SIZE_OF_VERTEX, (void *)(0 * sizeof(float)));
    glEnableVertexAttribArray(normalIndex);
    glVertexAttribPointer(normalIndex, 3, GL_FLOAT, GL_FALSE, SIZE_OF_VERTEX, (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(texCoordIndex);
    glVertexAttribPointer(texCoordIndex, 2, GL_FLOAT, GL_FALSE, SIZE_OF_VERTEX, (void *)((3 + 3) * sizeof(float)));
}

void Render::drawAsset(GPUAsset* asset, GLuint tex) {

    glBindTexture(GL_TEXTURE_2D, tex);

    GLuint buffer = asset->getBufferID();
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    initVertexBuffer(buffer);

    glDrawArrays(GL_TRIANGLES, 0, asset->getVertexCount());
}

void Render::draw() {

    if (this->window == nullptr)
        return;

    MeshAsset* walls = Physics::getInstance().getWalls();
    TetAsset* model = Physics::getInstance().getModel();

    walls->syncWithGPU();
    model->syncWithGPU();

    EigenVector3 position = model->getPosition();

    lookAtPoint(vec3(position.x(), position.y(), position.z()));

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    drawAsset((GPUAsset*) walls, wallTexture);
    drawAsset((GPUAsset*) model, modelTexture);

    // glFlush(); // do we need this or what?

    eglSwapBuffers(display, surface);
}

float Render::getCameraXAngle() {
    return this->cameraAngleX;
}

float Render::getCameraZAngle() {
    return this->cameraAngleZ;
}