#ifndef FEMFORANDROID_ASSET_MANAGER_H
#define FEMFORANDROID_ASSET_MANAGER_H

#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>

#include <GLES2/gl2.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <string>

#include <vector>

#include "EigenTypes.h"

using namespace std;
using namespace glm;

struct AssetVertex {
    float X, Y, Z, NX, NY, NZ, U, V;
};

class GPUAsset {
    friend class AssetManager;
private:
    int bufferVertexCount;
    GLuint bufferID;

    bool syncedWithGPU;

    virtual void transferToGPU();
protected:
    AssetVertex* vertexDataBuffer;
    void copyToGPU(GLenum usage);
public:
    GLuint getBufferID() const;
    int getVertexCount();

    void syncWithGPU();
    void invalidate(bool fully = false);
};

class MeshAsset : public GPUAsset {
    friend class AssetManager;
public:
};

class TetAsset : public GPUAsset {
    friend class AssetManager;
private:
    vector<EigenVector3> vertices, verticesToRender;

    vector<vector<int>> tets;

    struct Face;

    struct EdgeVertex {
        EigenVector3* vertex;
        EigenVector3 normal;
        vector<Face*> connectedFaces;
    };

    vector<EdgeVertex> edgeVertices;

    struct Face {
        EdgeVertex* vertex[3];
        EigenVector3 normal;
        float area;
        vec2 texCoord[3];
    };

    vector<Face> faces;

    void commitVertices();

    void calcNormals();

    void transferToGPU() override;
public:
    vector<EigenVector3>& getAllVertices();
    unsigned int getAllVerticesCount();

    vector<vector<int>>& getTets();
    unsigned int getTetCount();

    EigenVector3 getPosition();
};

class AssetManager {
public:
    static AssetManager& getInstance() {
        static AssetManager instance;

        return instance;
    }

    AssetManager(AssetManager const&) = delete;
    void operator=(AssetManager const&)  = delete;
private:
    AssetManager();

    int initialized;

    AAssetManager* nativeManager;

    string externalFilesDir;
public:
    void initialize(AAssetManager* nativeManager, string externalFilesDir);
    void finalize();

    string loadTextAsset(string assertName);
    GLuint loadTextureAsset(string assertName);

    MeshAsset* loadMeshBinAsset(string assertName, EigenVector3 translation, float scale,
            EigenQuaternion rotation);
    TetAsset* loadTetBinAsset(string assertName, EigenVector3 translation, float scale,
            EigenQuaternion rotation);

    bool loadExternalBinaryFile(string fileName, void* dest, unsigned int size);
    void saveExternalBinaryFile(string fileName, void* src, unsigned int size);
};

#endif //FEMFORANDROID_ASSET_MANAGER_H
