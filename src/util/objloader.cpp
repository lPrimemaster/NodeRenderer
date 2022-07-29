#define TINYOBJLOADER_IMPLEMENTATION
#include "objloader.h"

#include "../../tinyobjloader/tiny_obj_loader.h"

#include "../log/logger.h"

std::vector<float> Utils::LoadFloatVertexDataFromFile(const std::string& filename)
{
    tinyobj::ObjReaderConfig reader_config;
    tinyobj::ObjReader reader;

    if (!reader.ParseFromFile(filename, reader_config))
    {
        if (!reader.Error().empty()) 
        {
            L_ERROR("TinyObjReader: %s", reader.Error().c_str());
        }
    }

    if (!reader.Warning().empty()) 
    {
        L_ERROR("TinyObjReader: %s", reader.Warning().c_str());
    }

    auto& attrib = reader.GetAttrib();
    auto& shapes = reader.GetShapes();

    std::vector<float> data;

    for (size_t s = 0; s < shapes.size(); s++)
    {
        size_t index_offset = 0;
        for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++)
        {
            size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);
            data.reserve(data.capacity() + fv * 6); // NOTE: capacity() should be the same as size() here
            for (size_t v = 0; v < fv; v++)
            {
                // Assuming triangles
                tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
                tinyobj::real_t vx = attrib.vertices[3*size_t(idx.vertex_index)+0];
                tinyobj::real_t vy = attrib.vertices[3*size_t(idx.vertex_index)+1];
                tinyobj::real_t vz = attrib.vertices[3*size_t(idx.vertex_index)+2];

                if (idx.normal_index >= 0)
                {
                    tinyobj::real_t nx = attrib.normals[3*size_t(idx.normal_index)+0];
                    tinyobj::real_t ny = attrib.normals[3*size_t(idx.normal_index)+1];
                    tinyobj::real_t nz = attrib.normals[3*size_t(idx.normal_index)+2];

                    data.push_back(vx);
                    data.push_back(vy);
                    data.push_back(vz);

                    data.push_back(nx);
                    data.push_back(ny);
                    data.push_back(nz);
                }
                else
                {
                    // TODO: Compute some normals based on the triangles, for now, throw an error
                    L_ERROR("Obj Reader requires an input with normal data.");
                    return std::vector<float>();
                }
            }
            index_offset += fv;
        }
    }
    return data;
}

void Utils::DumpObjFileToDiskFromData(const std::string& filename, const float* data, size_t count, bool hasNormals)
{
    FILE* f = fopen(("objdumps/" + filename).c_str(), "w");

    if(f != nullptr)
    {
        fprintf(f, "o dump\n");
        for(size_t i = 0; i < count; i += (hasNormals ? 6 : 3))
        {
            fprintf(f, "v %f %f %f\n", data[i], data[i+1], data[i+2]);
        }

        if(hasNormals)
        {
            for(size_t i = 3; i < count; i += 6)
            {
                fprintf(f, "vn %f %f %f\n", data[i], data[i+1], data[i+2]);
            }

            for(size_t i = 0; i < count / 3; i++)
            {
                fprintf(f, "f %zu//%zu %zu//%zu %zu//%zu\n", 3*i+1, 3*i+1, 3*i+2, 3*i+2, 3*i+3, 3*i+3);
            }
        }
        else
        {
            for(size_t i = 0; i < count / 3; i++)
            {
                fprintf(f, "f %zu %zu %zu\n", 3*i+1, 3*i+2, 3*i+3);
            }  
        }

        fclose(f);
    }
}

void Utils::DumpObjFileToDiskFromData(const std::string& filename, const std::vector<Vector3>& data)
{
    const size_t stride = 3;
    float* tmp_data = new float[data.size() * stride];

    for(size_t i = 0; i < data.size(); i++)
    {
        tmp_data[stride * i    ] = data[i].x;
        tmp_data[stride * i + 1] = data[i].y;
        tmp_data[stride * i + 2] = data[i].z;
    }

    DumpObjFileToDiskFromData(filename, tmp_data, data.size() * 3, false);

    delete[] tmp_data;
}

void Utils::DumpPointsToDiskFromData(const std::string& filename, const std::vector<Vector3>& data)
{
    FILE* f = fopen(("objdumps/" + filename).c_str(), "w");

    if(f != nullptr)
    {
        for(auto& v : data)
        {
            fprintf(f, "%f, %f, %f,\n", v.x, v.y, v.z);
        }
        fclose(f);
    }
}

void Utils::DumpPointsToDiskFromData(const std::string& filename, const float* data, size_t count, bool hasNormals)
{
    FILE* f = fopen(("objdumps/" + filename).c_str(), "w");

    if(f != nullptr)
    {
        for(size_t i = 0; i < count; i += (hasNormals ? 6 : 3))
        {
            float x = data[i];
            float y = data[i+1];
            float z = data[i+2];
            fprintf(f, "%f, %f, %f,\n", x, y, z);
        }
        fclose(f);
    }
}
