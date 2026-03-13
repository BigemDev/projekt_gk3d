#include "objmodel.h"
#include <stdio.h>
#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <cstring>

namespace Models {

    ObjModel::ObjModel() {
    vertices      = nullptr;
    normals       = nullptr;
    vertexNormals = nullptr;
    texCoords     = nullptr;
    colors        = nullptr;
    vertexCount   = 0;
    vao           = 0;
}

    ObjModel::ObjModel(const std::string& filePath) : ObjModel() {
        load(filePath);
    }

    ObjModel::~ObjModel() {
        delete[] vertices;
        delete[] normals;
        delete[] texCoords;
    }

    static void parseFaceVertex(const char* token, int& vi, int& ti, int& ni) {
        vi = ti = ni = 0;
        if (sscanf(token, "%d/%d/%d", &vi, &ti, &ni) == 3) return;
        if (sscanf(token, "%d//%d",   &vi, &ni)      == 2) return;
        if (sscanf(token, "%d/%d",    &vi, &ti)       == 2) return;
        sscanf(token, "%d", &vi);
    }

    void ObjModel::load(const std::string& filePath) {
        FILE* f = fopen(filePath.c_str(), "r");
        if (!f) {
            fprintf(stderr, "ObjModel: nie można otworzyć pliku '%s'\n", filePath.c_str());
            return;
        }

        std::vector<glm::vec4> rawV;   // pozycje
        std::vector<glm::vec2> rawVT;  // UV
        std::vector<glm::vec4> rawVN;  // normalne

        std::vector<glm::vec4> outV;
        std::vector<glm::vec4> outVN;
        std::vector<glm::vec2> outVT;

        char line[512];
        while (fgets(line, sizeof(line), f)) {
            if (line[0] == '#' || line[0] == '\n') continue;

            if (strncmp(line, "vn ", 3) == 0) {
                float x, y, z;
                sscanf(line + 3, "%f %f %f", &x, &y, &z);
                rawVN.push_back(glm::vec4(x, y, z, 0.0f));

            } else if (strncmp(line, "vt ", 3) == 0) {
                float u, v;
                sscanf(line + 3, "%f %f", &u, &v);
                rawVT.push_back(glm::vec2(u, v));

            } else if (line[0] == 'v' && line[1] == ' ') {
                float x, y, z;
                sscanf(line + 2, "%f %f %f", &x, &y, &z);
                rawV.push_back(glm::vec4(x, y, z, 1.0f));

            } else if (line[0] == 'f' && line[1] == ' ') {
                std::vector<glm::ivec3> faceVerts;
                char* ctx = nullptr;
                char tmp[512];
                strncpy(tmp, line + 2, sizeof(tmp));
                char* tok = strtok_r(tmp, " \t\r\n", &ctx);
                while (tok) {
                    int vi, ti, ni;
                    parseFaceVertex(tok, vi, ti, ni);
                    faceVerts.push_back(glm::ivec3(vi, ti, ni));
                    tok = strtok_r(nullptr, " \t\r\n", &ctx);
                }

                for (int i = 1; i + 1 < (int)faceVerts.size(); i++) {
                    int idx[3] = {0, i, i + 1};
                    for (int j = 0; j < 3; j++) {
                        auto& fv = faceVerts[idx[j]];
                        int vi = fv.x - 1;
                        int ti = fv.y - 1;
                        int ni = fv.z - 1;

                        outV.push_back(vi >= 0 && vi < (int)rawV.size()
                            ? rawV[vi] : glm::vec4(0, 0, 0, 1));

                        outVN.push_back(ni >= 0 && ni < (int)rawVN.size()
                            ? rawVN[ni] : glm::vec4(0, 1, 0, 0));

                        outVT.push_back(ti >= 0 && ti < (int)rawVT.size()
                            ? rawVT[ti] : glm::vec2(0, 0));
                    }
                }
            }
        }
        fclose(f);

        vertexCount = (int)outV.size();
        printf("ObjModel: wczytano '%s' (%d wierzchołków)\n",
               filePath.c_str(), vertexCount);

        vertices  = new float[vertexCount * 4];
        normals   = new float[vertexCount * 4];
        texCoords = new float[vertexCount * 2];

        for (int i = 0; i < vertexCount; i++) {
            vertices[i*4+0] = outV[i].x;
            vertices[i*4+1] = outV[i].y;
            vertices[i*4+2] = outV[i].z;
            vertices[i*4+3] = outV[i].w;

            normals[i*4+0] = outVN[i].x;
            normals[i*4+1] = outVN[i].y;
            normals[i*4+2] = outVN[i].z;
            normals[i*4+3] = outVN[i].w;

            texCoords[i*2+0] = outVT[i].x;
            texCoords[i*2+1] = outVT[i].y;
        }

        vertexNormals = normals;

        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        GLuint vbos[3];
        glGenBuffers(3, vbos);

        glBindBuffer(GL_ARRAY_BUFFER, vbos[0]);
        glBufferData(GL_ARRAY_BUFFER, vertexCount * 4 * sizeof(float), vertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 4, GL_FLOAT, false, 0, 0);

        glBindBuffer(GL_ARRAY_BUFFER, vbos[1]);
        glBufferData(GL_ARRAY_BUFFER, vertexCount * 4 * sizeof(float), normals, GL_STATIC_DRAW);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 4, GL_FLOAT, false, 0, 0);

        glBindBuffer(GL_ARRAY_BUFFER, vbos[2]);
        glBufferData(GL_ARRAY_BUFFER, vertexCount * 2 * sizeof(float), texCoords, GL_STATIC_DRAW);
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, false, 0, 0);

        glBindVertexArray(0);
    }

    void ObjModel::drawSolid(bool smooth) {
        if (vao == 0) return;
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, vertexCount);
        glBindVertexArray(0);
    }

}
