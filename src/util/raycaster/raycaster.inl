#include "bvh.h"
#include "../../render/nodes/mesh_node.h" // For MeshNodeData
#include <numbers>

namespace Utils
{
    struct ClosestTriangleResult
    {
        ClosestTriangleResult() {  }
        ClosestTriangleResult(BvhTree::ISecResult isect_result, Vector3 new_vertex) : isect_result(isect_result), new_vertex(new_vertex) {  }
        BvhTree::ISecResult isect_result;
        Vector3 new_vertex;
    };

    inline std::optional<ClosestTriangleResult> TraceClosestTriangle(const int sampleCount, const Vector3& origin, const BvhTree& geometry)
    {
        std::optional<ClosestTriangleResult> closestIsect = std::nullopt;
        // Place sampleCount samples on a unit sphere around origin and create all the rays
        const float phi = std::numbers::pi_v<float> * (3.0f - sqrtf(5.0f));
        for(int s = 0; s < sampleCount; s++)
        {
            float theta = phi * s;

            float x = cosf(theta);
            float y = 1.0f - (s / ((float)sampleCount - 1.0f)) * 2.0f;
            float z = sinf(theta);
            Vector3 direction(x, y, z);

            if(auto hit = geometry.isect(origin, direction))
            {
                if(closestIsect)
                {
                    if(hit->intersection.t < closestIsect->isect_result.intersection.t)
                    {
                        closestIsect = ClosestTriangleResult(hit.value(), origin + direction * hit->intersection.t);
                    }
                }
                else
                {
                    closestIsect = ClosestTriangleResult(hit.value(), origin + direction * hit->intersection.t);
                }
            }
        }
        return closestIsect;
    }

    inline std::vector<BvhTree*> ConstructBvhTreesFromMeshes(std::vector<MeshNodeData> meshes, int* outParentIdx, bool adjustBounds = true)
    {
        // NOTE: This only works with meshes that are origin centered
        // If they are not, some results may come out weird

        struct InternalAABB
        {
            InternalAABB(float x, float y, float z)
            {
                x_sz = x;
                y_sz = y;
                z_sz = z;
            }

            float x_sz;
            float y_sz;
            float z_sz;
        };

        // Compute the largest bounding box and then scale them all accordingly
        std::vector<Vector3> scales(meshes.size(), Vector3(1.0f, 1.0f, 1.0f));
        int largestMeshCountIdx = -1;
        size_t maxCount = 0;
        int j = 0;
        if(adjustBounds)
        {
            std::vector<InternalAABB> iv_aabb;
            for(auto m : meshes)
            {
                float maxX = std::numeric_limits<float>::lowest();
                float minX = std::numeric_limits<float>::max();
                float maxY = std::numeric_limits<float>::lowest();
                float minY = std::numeric_limits<float>::max();
                float maxZ = std::numeric_limits<float>::lowest();
                float minZ = std::numeric_limits<float>::max();

                for(int i = 0; i < m.data_size; i += 3)
                {
                    float x = m.vertex_data[i];
                    float y = m.vertex_data[i+1];
                    float z = m.vertex_data[i+2];

                    if(x > maxX) maxX = x;
                    if(x < minX) minX = x;

                    if(y > maxY) maxY = y;
                    if(y < minY) minY = y;

                    if(z > maxZ) maxZ = z;
                    if(z < minZ) minZ = z;
                }

                if(m.data_size > maxCount)
                {
                    maxCount = m.data_size;
                    largestMeshCountIdx = j;
                }

                iv_aabb.push_back(InternalAABB(maxX - minX, maxY - minY, maxZ - minZ));
                j++;
            }

            if(outParentIdx != nullptr)
            {
                *outParentIdx = largestMeshCountIdx;
            }

            float maxX = 0.0f;
            float maxY = 0.0f;
            float maxZ = 0.0f;
            for(int i = 0; i < (int)iv_aabb.size(); i++)
            {
                if(iv_aabb[i].x_sz > maxX) maxX = iv_aabb[i].x_sz;
                if(iv_aabb[i].y_sz > maxY) maxY = iv_aabb[i].y_sz;
                if(iv_aabb[i].z_sz > maxZ) maxZ = iv_aabb[i].z_sz;
            }

            for(int i = 0; i < (int)iv_aabb.size(); i++)
            {
                scales[i] = Vector3(iv_aabb[i].x_sz / maxX, iv_aabb[i].y_sz / maxY, iv_aabb[i].z_sz / maxZ);
            }
        }

        std::vector<BvhTree*> ret;
        ret.reserve(meshes.size());
        for(int i = 0; i < (int)meshes.size(); i++)
        {
            if(i == largestMeshCountIdx) continue; // Skip the mesh with the most vertices, which will be the reference
            ret.emplace_back(new BvhTree(meshes[i].vertex_data, meshes[i].data_size, 0, 0, scales[i].x, scales[i].y, scales[i].z));
        }

        return ret;
    }

    inline void FreeBvhTrees(std::vector<BvhTree*> bvhList)
    {
        for(auto bvh : bvhList)
        {
            delete bvh;
        }
    }

    // TODO: Multi-thread ray intersector
    inline std::vector<std::vector<Vector3>> CalculateNewRaycastVerticesIntersection(const std::vector<BvhTree*>& bvhList, const MeshNodeData& parentMesh, const int sampleCount)
    {
        std::vector<std::vector<Vector3>> new_points_by_mesh;
        for(auto bvh : bvhList)
        {
            static_assert(sizeof(float) * 3 == sizeof(Vector3));
            std::vector<Vector3> new_points;
            new_points.reserve(parentMesh.data_size / 6);
            for(int i = 0; i < parentMesh.data_size; i += 6)
            {
                Vector3 vertex(parentMesh.vertex_data[i], parentMesh.vertex_data[i + 1], parentMesh.vertex_data[i + 2]);

                if(auto closest_triangle = TraceClosestTriangle(sampleCount, vertex, *bvh))
                {
                    Vector3 new_vertex = closest_triangle->new_vertex;
                    new_points.push_back(new_vertex);
                }
            }
            new_points.shrink_to_fit();
            new_points_by_mesh.push_back(new_points);
        }
        return new_points_by_mesh;
    }

    // inline 

    inline void RefitNewVerticesToOriginalMeshPoints(std::vector<std::vector<float>>& calculatedVertices, const std::vector<MeshNodeData>& originalMeshes)
    {
        
    }
}
