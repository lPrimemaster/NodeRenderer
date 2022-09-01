#pragma once
#include <vector>
#include "../math/vector.h"
#include "../../glm/glm/glm.hpp"

// This file declares custom node output data

// mesh_node.h output
struct MeshNodeData
{
    float* vertex_data = nullptr;
    size_t data_size = 0;
};

// mesh_interp_node.h output
struct MeshInterpListData
{
    std::vector<MeshNodeData> meshes;
    size_t totalMeshesDataSize = 0; // Unsused by the render node
    float t = 0;
    bool changeParamOnly = false;
};

// render_node.h output
struct RenderNodeData
{
    // Instance and mesh rendering data
    unsigned int   _instanceCount = 0;
    MeshNodeData** _meshPtr = nullptr;
    unsigned int   _meshCount = 0;
    float          _meshParam = 0.0f;
    Vector4**      _worldPositionPtr = nullptr;
    glm::mat4**    _worldRotationPtr = nullptr;
    Vector4**      _instanceColorsPtr = nullptr;
    glm::mat4**    _motifPositionPtr = nullptr;

    // Fog rendering data
    float   _fogMax = 50.0f;
    float   _fogMin = 10.0f;
    Vector3 _fogColor = { 0.7f, 0.7f, 0.7f };
    bool    _fogChanged = false;

    // Position related options
    bool         _repeatBlocks = false;
    Vector3      _motifSize = Vector3(2.0f, 2.0f, 2.0f);
    unsigned int _motif_span = 1;
    unsigned int _motifInstances[3] = {1, 1, 1};
    bool         _motifChanged = false;
};


