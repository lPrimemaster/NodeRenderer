#include "bvh.h"
#include "../../../glm/glm/glm.hpp"
#include "../../../glm/glm/gtx/transform.hpp"

Utils::BvhTree::BvhTree(const float* vertices, size_t count, size_t stride, size_t offset, float sx, float sy, float sz)
{
    using Vector3f = bvh::Vector3<float>;
    std::vector<bvh::Triangle<float>> triangles;

    // Create triangles from parent vertices with certain offset / stride
    for(size_t i = offset; i < count; i += stride + 18)
    {
        glm::mat4 scaleMat = glm::scale(glm::vec3(sx, sy, sz));

        glm::vec4 v1 = scaleMat * glm::vec4(vertices[i   ], vertices[i+ 1], vertices[i+ 2], 1.0f);
        glm::vec4 v2 = scaleMat * glm::vec4(vertices[i+ 6], vertices[i+ 7], vertices[i+ 8], 1.0f);
        glm::vec4 v3 = scaleMat * glm::vec4(vertices[i+12], vertices[i+13], vertices[i+14], 1.0f);
        // NOTE: Scale matrix should not affect w
        triangles.emplace_back(
            Vector3f(v1.x,
                     v1.y,
                     v1.z),
            Vector3f(v2.x,
                     v2.y,
                     v2.z),
            Vector3f(v3.x,
                     v3.y,
                     v3.z)
        );
    }
    
    // Build the bvh and its requirements
    auto [bboxes, centers] = bvh::compute_bounding_boxes_and_centers(triangles.data(), triangles.size());
    auto global_bbox = bvh::compute_bounding_boxes_union(bboxes.get(), triangles.size());

    bvh::SweepSahBuilder<BvhHandler> builder(bvhHandler);
    builder.build(global_bbox, bboxes.get(), centers.get(), triangles.size());

    primitive_intersector = new bvh::ClosestPrimitiveIntersector<BvhHandler, bvh::Triangle<float>>(bvhHandler, triangles.data());
    traverser = new bvh::SingleRayTraverser<BvhHandler>(bvhHandler);
}

Utils::BvhTree::~BvhTree()
{
    if(primitive_intersector)
        delete primitive_intersector;
    if(traverser)
        delete traverser;
}
