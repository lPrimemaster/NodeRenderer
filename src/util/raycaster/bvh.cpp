#include "bvh.h"

Utils::BvhTree::BvhTree(const float* vertices, size_t count, size_t stride, size_t offset)
{
    using Vector3f = bvh::Vector3<float>;
    std::vector<bvh::Triangle<float>> triangles;

    // Create triangles from parent vertices with certain offset / stride
    for(size_t i = offset; i < count; i += stride + 9)
    {
        triangles.emplace_back(
            Vector3f(vertices[i],
                     vertices[i+1],
                     vertices[i+2]),
            Vector3f(vertices[i+3],
                     vertices[i+4],
                     vertices[i+5]),
            Vector3f(vertices[i+6],
                     vertices[i+7],
                     vertices[i+8])
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
