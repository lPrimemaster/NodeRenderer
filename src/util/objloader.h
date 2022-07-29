#pragma once
#include <vector>
#include <string>
#include "../math/vector.h"

namespace Utils
{
    std::vector<float> LoadFloatVertexDataFromFile(const std::string& filename);

    void DumpObjFileToDiskFromData(const std::string& filename, const float* data, size_t count, bool hasNormals = false);
    void DumpObjFileToDiskFromData(const std::string& filename, const std::vector<Vector3>& data);
    void DumpPointsToDiskFromData(const std::string& filename, const std::vector<Vector3>& data);
    void DumpPointsToDiskFromData(const std::string& filename, const float* data, size_t count, bool hasNormals = false);
}
