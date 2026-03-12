#ifndef OBJMODEL_H
#define OBJMODEL_H

#include "model.h"
#include <string>

namespace Models {

    class ObjModel : public Model {
    public:
        ObjModel();
        ObjModel(const std::string& filePath);
        virtual ~ObjModel();
        virtual void drawSolid(bool smooth = true);

    private:
        GLuint vao;
        void load(const std::string& filePath);
    };

}

#endif