#include "model.h"

namespace Models {
    void Model::drawWire(bool smooth) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        drawSolid(smooth);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
}