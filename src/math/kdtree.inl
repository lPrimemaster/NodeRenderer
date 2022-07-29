#include "vector.h"
#include <algorithm>
#include <vector>
#include <queue>

namespace Math
{
    struct KdNode
    {
        KdNode(const Vector3& point) : point(point) {  }
        KdNode() {  }

        ~KdNode()
        {
            if(lo) delete lo;
            if(hi) delete hi;
        }

        size_t idx = 0;
        size_t cut_dim = 0;

        Vector3 point;

        KdNode* lo = nullptr;
        KdNode* hi = nullptr;

        Vector3 lbound;
        Vector3 hbound;
    };

    class KdTree
    {
    public:
        KdTree(const std::vector<KdNode>& nodesvec) : nodes(nodesvec)
        {
            lbound = nodesvec.begin()->point;
            hbound = nodesvec.begin()->point;

            for(size_t i = 1; i < nodesvec.size(); i++)
            {
                for(size_t j = 0; j < 3; j++)
                {
                    float val = nodes[i].point.data[j];
                    if(lbound.data[j] > val) lbound.data[j] = val;
                    if(hbound.data[j] < val) hbound.data[j] = val;
                }
            }
            
            // Build the tree
            root = BuildTree(0, 0, nodes.size());
        }

        ~KdTree()
        {
            if(root) delete root;
        }

        std::vector<KdNode> kNNSearch(const Vector3& point, size_t k)
        {
            std::vector<KdNode> result;

            if(k < 1) return std::vector<KdNode>();

            neighbourheap = new std::priority_queue<nnheap, std::vector<nnheap>, nnheap::compare>();

            if(k > nodes.size())
            {
                k = nodes.size();
                for(size_t i = 0; i < k; i++)
                {
                    neighbourheap->push(nnheap(i, Vector3::DistanceSqr(nodes[i].point, point)));
                }
            }
            else
            {
                internal_nnsearch(point, root, k);
            }

            while(!neighbourheap->empty())
            {
                // L_TRACE("%d %f", neighbourheap->top().idx, neighbourheap->top().distance);
                size_t i = neighbourheap->top().idx;
                neighbourheap->pop();
                result.push_back(nodes[i]);
            }

            k = result.size();
            for(size_t i = 0; i < k / 2; i++)
            {
                KdNode temp = result[i];
                result[i] = result[k - i - 1];
                result[k - i - 1] = temp;
            }
            delete neighbourheap;
            return result;
        }

    private:
        KdNode* BuildTree(size_t depth, size_t a, size_t b)
        {
            KdNode* node = new KdNode();
            node->lbound = lbound;
            node->hbound = hbound;
            node->cut_dim = depth % 3;
            
            if(b - a <= 1)
            {
                node->idx = a;
                node->point = nodes[a].point;
            }
            else
            {
                size_t m = (a + b) / 2;
                std::nth_element(
                    nodes.begin() + a, 
                    nodes.begin() + m, 
                    nodes.begin() + b,
                    [&](const KdNode& p, const KdNode& q) -> bool { return (p.point.data[node->cut_dim] < q.point.data[node->cut_dim]); }
                );
                node->point = nodes[m].point;
                float cut_val = nodes[m].point.data[node->cut_dim];
                node->idx = m;

                if(m - a > 0)
                {
                    float temp = hbound.data[node->cut_dim];
                    hbound.data[node->cut_dim] = cut_val;
                    node->lo = BuildTree(depth + 1, a, m);
                    hbound.data[node->cut_dim] = temp;
                }

                if(b - m > 1)
                {
                    float temp = lbound.data[node->cut_dim];
                    lbound.data[node->cut_dim] = cut_val;
                    node->hi = BuildTree(depth + 1, m + 1, b);
                    lbound.data[node->cut_dim] = temp;
                }
            }
            return node;
        }

        struct nnheap
        {
            nnheap(size_t idx, float distance) : idx(idx), distance(distance) {  }
            size_t idx;
            float distance;

            struct compare
            {
                bool operator()(const nnheap& a, const nnheap& b)
                {
                    return a.distance < b.distance;
                }
            };
        };

        bool internal_nnsearch(const Vector3& point, KdNode* node, size_t k)
        {
            float curr_dist = Vector3::DistanceSqr(point, node->point);

            if(neighbourheap->size() < k)
            {
                neighbourheap->push(nnheap(node->idx, curr_dist));
            }
            else if(curr_dist < neighbourheap->top().distance)
            {
                neighbourheap->pop();
                neighbourheap->push(nnheap(node->idx, curr_dist));
            }

            if(point.data[node->cut_dim] < node->point.data[node->cut_dim])
            {
                if(node->lo && internal_nnsearch(point, node->lo, k)) return true;
            }
            else
            {
                if(node->hi && internal_nnsearch(point, node->hi, k)) return true;
            }

            float dist;
            if(neighbourheap->size() < k)
            {
                dist = std::numeric_limits<float>::max();
            }
            else
            {
                dist = neighbourheap->top().distance;
            }

            if(point.data[node->cut_dim] < node->point.data[node->cut_dim])
            {
                if(node->hi && internal_bound_overlap(point, dist, node->hi) && internal_nnsearch(point, node->hi, k)) return true;
            }
            else
            {
                if(node->lo && internal_bound_overlap(point, dist, node->lo) && internal_nnsearch(point, node->lo, k)) return true;
            }

            if(neighbourheap->size() == k) dist = neighbourheap->top().distance;
            return internal_bound_within(point, dist, node);
        }

        bool internal_bound_overlap(const Vector3& point, float distance, KdNode* node)
        {
            float sum = 0.0f;
            for(size_t i = 0; i < 3; i++)
            {
                if(point.data[i] < node->lbound.data[i])
                {
                    sum += std::powf(point.data[i] - node->lbound.data[i], 2);
                    if(sum > distance) return false;
                }
                if(point.data[i] > node->hbound.data[i])
                {
                    sum += std::powf(point.data[i] - node->hbound.data[i], 2);
                    if(sum > distance) return false;
                }
            }
            return true;
        }

        bool internal_bound_within(const Vector3& point, float distance, KdNode* node)
        {
            for(size_t i = 0; i < 3; i++)
            {
                if(std::powf(point.data[i] - node->lbound.data[i], 2) <= distance ||
                   std::powf(point.data[i] - node->hbound.data[i], 2) <= distance) return false;
            }
            return true;
        }

        std::priority_queue<nnheap, std::vector<nnheap>, nnheap::compare>* neighbourheap;
        std::vector<KdNode> nodes;
        KdNode* root = nullptr;
        Vector3 lbound;
        Vector3 hbound;
    };
}
