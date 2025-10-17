#pragma once

#include <glm/glm.hpp>

struct BasicMeshEntry {
    unsigned int NumIndices = 0;
    unsigned int NumVertices = 0;
    unsigned int BaseVertex = 0;
    unsigned int BaseIndex = 0;
    unsigned int ValidFaces = 0;
    int MaterialIndex = -1;
    glm::mat4 Transformation;
};