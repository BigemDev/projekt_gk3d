#ifndef MODEL_H
#define MODEL_H

#include <glad/gl.h>
#include <vector>
#include <glm/glm.hpp>

namespace Models {
    
    class Model {
    public:
        int vertexCount;
        float* vertices;
        float* normals;
        float* vertexNormals;
        float* texCoords;
        float* colors;

        virtual void drawSolid(bool smooth = true) = 0;
        virtual void drawWire(bool smooth = false);
    };
}

#endif