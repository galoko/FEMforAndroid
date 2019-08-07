#include "AssetManager.h"

#include <sys/stat.h>
#include <dirent.h>

#include "log.h"
#include "exceptionUtils.h"

#define ASSET_MANAGER_TAG "PT_ASSET_MANAGER"

AssetManager::AssetManager() {

}

void AssetManager::initialize(AAssetManager* nativeManager, string externalFilesDir) {

    if (this->initialized == 1)
        return;

    this->nativeManager = nativeManager;
    this->externalFilesDir = externalFilesDir;

    this->initialized = 1;
}

void AssetManager::finalize() {

    if (this->initialized == 0)
        return;

    this->initialized = 0;
}

string AssetManager::loadTextAsset(string assertName) {

    AAsset* asset = AAssetManager_open(nativeManager, assertName.c_str(), AASSET_MODE_BUFFER);
    if (!asset)
        return "";

    const void* data = AAsset_getBuffer(asset);
    off64_t size = AAsset_getLength64(asset);

    string result = string(static_cast<const char*>(data), (unsigned int) size);

    AAsset_close(asset);

    return result;
}

GLuint AssetManager::loadTextureAsset(string assertName) {

    AAsset* asset = AAssetManager_open(nativeManager, assertName.c_str(), AASSET_MODE_BUFFER);
    if (!asset)
        return 0;

    auto data = (const unsigned char*)AAsset_getBuffer(asset);
    off64_t size = AAsset_getLength64(asset);

    GLuint textureID = 0;

    if (size > 54) {
        unsigned int dataPos    = 54;

        unsigned int width      = *(unsigned int*)&(data[0x12]);
        unsigned int height     = *(unsigned int*)&(data[0x16]);

        if (size == 54 + width * height * 3) {

            auto pixels = data + dataPos;

            glGenTextures(1, &textureID);

            glBindTexture(GL_TEXTURE_2D, textureID);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE,
                         pixels);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            glGenerateMipmap(GL_TEXTURE_2D);
        }
    }

    AAsset_close(asset);

    return textureID;
}

MeshAsset* AssetManager::loadMeshBinAsset(string assertName, EigenVector3 translation, float scale,
        EigenQuaternion rotation) {

    AAsset* asset = AAssetManager_open(nativeManager, assertName.c_str(), AASSET_MODE_BUFFER);
    if (!asset)
        return nullptr;

    auto data = (const unsigned char*)AAsset_getBuffer(asset);
    off64_t size = AAsset_getLength64(asset);

    off64_t vertexCount = size / sizeof(AssetVertex);
    my_assert((size % sizeof(AssetVertex)) == 0);

    auto result = new MeshAsset();
    result->bufferVertexCount = vertexCount;
    result->vertexDataBuffer = new AssetVertex[result->bufferVertexCount];

    auto srcVertices = (AssetVertex*)data;
    auto dstVertices = result->vertexDataBuffer;

    for (int i = 0; i < vertexCount; i++) {

        EigenVector3 v = EigenVector3(srcVertices->X, srcVertices->Y, srcVertices->Z);

        v = (rotation * v) * scale + translation;

        dstVertices->X = v.x();
        dstVertices->Y = v.y();
        dstVertices->Z = v.z();

        EigenVector3 n = EigenVector3(srcVertices->NX, srcVertices->NY, srcVertices->NZ);

        n = rotation * n;

        dstVertices->NX = n.x();
        dstVertices->NY = n.y();
        dstVertices->NZ = n.z();

        dstVertices->U = srcVertices->U;
        dstVertices->V = srcVertices->V;

        srcVertices++;
        dstVertices++;
    }

    AAsset_close(asset);

    return result;
}

TetAsset* AssetManager::loadTetBinAsset(string assertName, EigenVector3 translation,
        float scale, EigenQuaternion rotation) {

    AAsset *asset = AAssetManager_open(nativeManager, assertName.c_str(), AASSET_MODE_BUFFER);
    if (!asset)
        return nullptr;

    TetAsset* result = nullptr;

    auto data = (const unsigned char *) AAsset_getBuffer(asset);
    off64_t size = AAsset_getLength64(asset);

#pragma pack(push, 1)
    struct BinaryVertex {
        float x, y, z;
    } __attribute__((packed));

    struct BinaryTet {
        unsigned short indices[4];
    } __attribute__((packed));

    struct BinaryTexCoord {
        float u, v;
    } __attribute__((packed));

    struct BinaryFace {
        unsigned short index[3], texIndex[3];
    } __attribute__((packed));
#pragma pack(pop)

    auto dataStart = data;

    unsigned short vertexCount, tetCount, texCoordsCount, faceCount;
    BinaryVertex* vertices;
    BinaryTet* tets;
    BinaryTexCoord* texCoords;
    BinaryFace* faces;

    vertexCount = *(unsigned short*)data;
    data += sizeof(vertexCount);
    vertices = (BinaryVertex*)data;
    data += vertexCount * sizeof(*vertices);

    tetCount = *(unsigned short*)data;
    data += sizeof(tetCount);
    tets = (BinaryTet*)data;
    data += tetCount * sizeof(*tets);

    texCoordsCount = *(unsigned short*)data;
    data += sizeof(texCoordsCount);
    texCoords = (BinaryTexCoord*)data;
    data += texCoordsCount * sizeof(*texCoords);

    faceCount = *(unsigned short*)data;
    data += sizeof(faceCount);
    faces = (BinaryFace*)data;
    data += faceCount * sizeof(*faces);

    unsigned int readed = (unsigned int)(data - dataStart);
    if (readed == size) {
        result = new TetAsset();

        EigenMatrix3 rotationMatrix = rotation.matrix();

        result->vertices.resize(vertexCount);
        result->verticesToRender.resize(vertexCount);

        for (int i = 0; i < vertexCount; i++) {
            float x = vertices[i].x;
            float y = vertices[i].y;
            float z = vertices[i].z;

            EigenVector3 v = EigenVector3(x, y, z);

            v = (rotation * v) * scale + translation;

            result->vertices[i] = v;
        }

        result->tets.resize(tetCount);
        for (int i = 0; i < tetCount; i++) {
            result->tets[i].resize(4);
            for (int j = 0; j < 4; j++)
                result->tets[i][j] = tets[i].indices[j];
        }

        result->faces.resize(faceCount);

        for (int i = 0; i < faceCount; i++)
            for (int j = 0; j < 3; j++) {

                EigenVector3 *vertex = &result->verticesToRender[faces[i].index[j]];

                TetAsset::EdgeVertex* edgeVertex = nullptr;

                for (int k = 0; k < result->edgeVertices.size(); k++) {
                    if (result->edgeVertices[k].vertex == vertex) {
                        edgeVertex = &result->edgeVertices[k];
                        break;
                    }
                }

                if (edgeVertex == nullptr) {
                    TetAsset::EdgeVertex newEdgeVertex;
                    newEdgeVertex.vertex = vertex;

                    result->edgeVertices.push_back(newEdgeVertex);

                    edgeVertex = &result->edgeVertices[result->edgeVertices.size() - 1];
                }
            }

        for (int i = 0; i < faceCount; i++)
            for (int j = 0; j < 3; j++) {

                EigenVector3* vertex = &result->verticesToRender[faces[i].index[j]];

                TetAsset::EdgeVertex* edgeVertex = nullptr;
                for (int k = 0; k < result->edgeVertices.size(); k++) {
                    if (result->edgeVertices[k].vertex == vertex) {
                        edgeVertex = &result->edgeVertices[k];
                        break;
                    }
                }
                my_assert(edgeVertex != nullptr);

                TetAsset::Face* face = &result->faces[i];

                face->vertex[j] = edgeVertex;
                face->texCoord[j] = vec2(texCoords[faces[i].texIndex[j]].u,
                                                    texCoords[faces[i].texIndex[j]].v);

                edgeVertex->connectedFaces.push_back(face);
            }

        result->bufferVertexCount = faceCount * 3;
        result->vertexDataBuffer = new AssetVertex[result->bufferVertexCount];
    }

    AAsset_close(asset);

    return result;
}

bool AssetManager::loadExternalBinaryFile(string fileName, void* dest, unsigned int size) {

    string fullFileName = this->externalFilesDir + "/" + fileName;

    bool result = false;

    FILE* fileHandle = fopen(fullFileName.c_str(), "rb");
    if (fileHandle != nullptr) {

        size_t readed = fread(dest, 1, size, fileHandle);
        if (readed == size)
            result = true;

        fclose(fileHandle);
        fileHandle = nullptr;
    }

    return result;
}

void AssetManager::saveExternalBinaryFile(string fileName, void* src, unsigned int size) {

    string fullFileName = this->externalFilesDir + "/" + fileName;

    bool result = false;

    FILE* fileHandle = fopen(fullFileName.c_str(), "w");
    if (fileHandle != nullptr) {

        size_t written = fwrite(src, 1, size, fileHandle);

        fclose(fileHandle);
        fileHandle = nullptr;
    }
}

// GPUAsset

GLuint GPUAsset::getBufferID() const {
    return this->bufferID;
}

int GPUAsset::getVertexCount() {
    return this->bufferVertexCount;
}

void GPUAsset::syncWithGPU() {
    if (!syncedWithGPU) {
        transferToGPU();
        syncedWithGPU = true;
    }
}

void GPUAsset::invalidate(bool fully) {

    syncedWithGPU = false;

    if (fully) {
        if (bufferID != 0) {
            glDeleteBuffers(1, &bufferID);
            bufferID = 0;
        }
    }
}

void GPUAsset::transferToGPU() {
    copyToGPU(GL_STATIC_DRAW);
}

void GPUAsset::copyToGPU(GLenum usage) {

    // print_log(ANDROID_LOG_INFO, ASSET_MANAGER_TAG, "GPU Asset copied to GPU");

    if (bufferID == 0)
        glGenBuffers(1, &bufferID);

    unsigned int size = getVertexCount() * sizeof(AssetVertex);

    glBindBuffer(GL_ARRAY_BUFFER, bufferID);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)size, vertexDataBuffer, usage);
}

// TetAsset

void TetAsset::calcNormals() {

    for (int faceIndex = 0; faceIndex < faces.size(); faceIndex++) {

        EigenVector3 A = *faces[faceIndex].vertex[0]->vertex - *faces[faceIndex].vertex[1]->vertex;
        EigenVector3 B = *faces[faceIndex].vertex[0]->vertex - *faces[faceIndex].vertex[2]->vertex;
        EigenVector3 C = *faces[faceIndex].vertex[1]->vertex - *faces[faceIndex].vertex[2]->vertex;

        float a = A.norm();
        float b = B.norm();
        float c = C.norm();

        float s = (a + b + c) * 0.5f;

        float area = std::sqrt(s * (s - a) * (s - b) * (s - c));

        EigenVector3 N = A.cross(B);
        N.normalize();

        faces[faceIndex].normal = N;
        faces[faceIndex].area = area;
    }

    for (int edgeVertexIndex = 0; edgeVertexIndex < edgeVertices.size(); edgeVertexIndex++) {
        EdgeVertex* edgeVertex = &edgeVertices[edgeVertexIndex];

        EigenVector3 normalSum = EigenVector3(0, 0, 0);
        float areaSum = 0.0;

        for (int faceIndex = 0; faceIndex < edgeVertex->connectedFaces.size(); faceIndex++) {
            Face* face = edgeVertex->connectedFaces[faceIndex];

            normalSum += face->normal * face->area;
            areaSum += face->area;
        }

        edgeVertex->normal = normalSum / areaSum;

        edgeVertex->normal.normalize();
    }
}

vector<EigenVector3>& TetAsset::getAllVertices() {
    return vertices;
}

unsigned int TetAsset::getAllVerticesCount() {
    return vertices.size();
}

vector<vector<int>>& TetAsset::getTets() {
    return tets;
}

unsigned int TetAsset::getTetCount() {
    return tets.size();
}

EigenVector3 TetAsset::getPosition() {

    EigenVector3 position = EigenVector3(0, 0, 0);

    for (int i = 0; i < edgeVertices.size(); i++) {
        position += *edgeVertices[i].vertex;
    }

    position /= edgeVertices.size();;

    return position;
}

void TetAsset::commitVertices() {

    for (int i = 0; i < vertices.size(); i++)
        verticesToRender[i] = vertices[i];
}

void TetAsset::transferToGPU() {

    commitVertices();

    calcNormals();

    for (int faceIndex = 0, vertexIndex = 0; faceIndex < faces.size(); faceIndex++) {
        vertexDataBuffer[vertexIndex].X = faces[faceIndex].vertex[0]->vertex->x();
        vertexDataBuffer[vertexIndex].Y = faces[faceIndex].vertex[0]->vertex->y();
        vertexDataBuffer[vertexIndex].Z = faces[faceIndex].vertex[0]->vertex->z();
        vertexIndex++;

        vertexDataBuffer[vertexIndex].X = faces[faceIndex].vertex[1]->vertex->x();
        vertexDataBuffer[vertexIndex].Y = faces[faceIndex].vertex[1]->vertex->y();
        vertexDataBuffer[vertexIndex].Z = faces[faceIndex].vertex[1]->vertex->z();
        vertexIndex++;

        vertexDataBuffer[vertexIndex].X = faces[faceIndex].vertex[2]->vertex->x();
        vertexDataBuffer[vertexIndex].Y = faces[faceIndex].vertex[2]->vertex->y();
        vertexDataBuffer[vertexIndex].Z = faces[faceIndex].vertex[2]->vertex->z();
        vertexIndex++;
    }

    for (int faceIndex = 0, vertexIndex = 0; faceIndex < faces.size(); faceIndex++) {

        vertexDataBuffer[vertexIndex].NX = faces[faceIndex].vertex[0]->normal.x();
        vertexDataBuffer[vertexIndex].NY = faces[faceIndex].vertex[0]->normal.y();
        vertexDataBuffer[vertexIndex].NZ = faces[faceIndex].vertex[0]->normal.z();

        vertexDataBuffer[vertexIndex].U = faces[faceIndex].texCoord[0].x;
        vertexDataBuffer[vertexIndex].V = faces[faceIndex].texCoord[0].y;

        vertexIndex++;

        vertexDataBuffer[vertexIndex].NX = faces[faceIndex].vertex[1]->normal.x();
        vertexDataBuffer[vertexIndex].NY = faces[faceIndex].vertex[1]->normal.y();
        vertexDataBuffer[vertexIndex].NZ = faces[faceIndex].vertex[1]->normal.z();

        vertexDataBuffer[vertexIndex].U = faces[faceIndex].texCoord[1].x;
        vertexDataBuffer[vertexIndex].V = faces[faceIndex].texCoord[1].y;

        vertexIndex++;

        vertexDataBuffer[vertexIndex].NX = faces[faceIndex].vertex[2]->normal.x();
        vertexDataBuffer[vertexIndex].NY = faces[faceIndex].vertex[2]->normal.y();
        vertexDataBuffer[vertexIndex].NZ = faces[faceIndex].vertex[2]->normal.z();

        vertexDataBuffer[vertexIndex].U = faces[faceIndex].texCoord[2].x;
        vertexDataBuffer[vertexIndex].V = faces[faceIndex].texCoord[2].y;

        vertexIndex++;
    }

    copyToGPU(GL_DYNAMIC_DRAW);
}