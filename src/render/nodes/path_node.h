#pragma once
#include "node.h"
#include "../../math/vector.h"
#include "../../math/comb.inl"
#include <numeric>

struct PathNode final : public PropertyNode
{
    inline PathNode() : PropertyNode(Vector3(0, 0, 0))
    {
        static int inc = 0;
        setInputsOrdered(
            {
                "t"
            }
        );
        _output_count = 1;
        name = "Path Node #" + std::to_string(inc++);
    }
    
    ~PathNode() {  }

    enum class Type
    {
        ALONG_X,
        ALONG_Y,
        ALONG_Z,
        ALONG_V,
        FROM_LIST
    };

    inline virtual void render() override
    {
        static const char* const type_names[] = {
            "Along X",
            "Along Y",
            "Along Z",
            "Along Vector",
            "Points from list"
        };

        if(ImGui::Combo("Type", &currenttypeid, type_names, sizeof(type_names) / sizeof(type_names[0])))
        {
            type = static_cast<Type>(currenttypeid);

            switch (type)
            {
            case Type::ALONG_X:
                forward = Vector3(1, 0, 0);
                along_forward = false;
                static_forward = true;
                disconnectInputIfNotOfType<PropertyNode::EmptyType>("forward");
                disconnectInputIfNotOfType<PropertyNode::EmptyType>("points");
                setInputsOrdered(
                    {
                        "t"
                    }
                );
                break;
            case Type::ALONG_Y:
                forward = Vector3(0, 1, 0);
                along_forward = false;
                static_forward = true;
                disconnectInputIfNotOfType<PropertyNode::EmptyType>("forward");
                disconnectInputIfNotOfType<PropertyNode::EmptyType>("points");
                setInputsOrdered(
                    {
                        "t"
                    }
                );
                break;
            case Type::ALONG_Z:
                forward = Vector3(0, 0, 1);
                along_forward = false;
                static_forward = true;
                disconnectInputIfNotOfType<PropertyNode::EmptyType>("forward");
                disconnectInputIfNotOfType<PropertyNode::EmptyType>("points");
                setInputsOrdered(
                    {
                        "t"
                    }
                );
                break;
            case Type::ALONG_V:
                forward = Vector3(0, 0, 1);
                along_forward = true;
                static_forward = true;
                along_inited = false;
                disconnectInputIfNotOfType<PropertyNode::EmptyType>("points");
                setInputsOrdered(
                    {
                        "t",
                        "forward"
                    }
                );
                break;
            case Type::FROM_LIST:
                forward = Vector3(0, 0, 0);
                static_forward = false;
                along_forward = false;
                curve_inited = false;
                disconnectInputIfNotOfType<PropertyNode::EmptyType>("forward");
                setInputsOrdered(
                    {
                        "t",
                        "points"
                    }
                );
                break;
            
            default:
                break;
            }
        }

        if(static_forward)
        {
            ImGui::InputFloat3("Start Position", start_pos.data, "%.1f");
        }
    }

    inline virtual void update() override
    {
        data.resetDataUpdate();

        disconnectInputIfNotOfType<float>("t");

        float t = 0.0f;

        auto param = inputs_named.find("t");
        if(param != inputs_named.end())
        {
            t = param->second->data.getValue<float>();
        }
        
        // Straight line umclamped t
        if(static_forward)
        {
            if(along_forward)
            {
                disconnectInputIfNotOfType<Vector3>("forward");
                auto forward_in = inputs_named.find("forward");
                if(forward_in != inputs_named.end())
                {
                    if(!along_inited || forward_in->second->data.dataChanged())
                    {
                        forward = Vector3::Normalize(forward_in->second->data.getValue<Vector3>());
                        along_inited = true;
                    }
                }
            }

            calculated_pos = start_pos + forward * t;
        }
        else // Bezier clamped segments t -> [0, 1]
        {
            disconnectInputIfNotOfType<std::vector<Vector3>>("points");
            auto points_in = inputs_named.find("points");
            if(points_in != inputs_named.end())
            {
                if(!curve_inited || points_in->second->data.dataChanged())
                {
                    auto points = points_in->second->data.getValue<std::vector<Vector3>>();
                    points_copy = points;
                    L_TRACE("points:");
                    for(int i = 0; i < points_copy.size(); i++) L_TRACE("(%.1f, %.1f, %.1f)", points_copy[i].x, points_copy[i].y, points_copy[i].z);
                    curve_inited = true;
                }

                // What segment are we in ?
                const int segments = points_copy.size();
                int seg = (int)(segments * t); // TODO : Assert t is between 0 and 1
                

                calculated_pos = bezierAt(t);
            }
        }
        data.setValue(calculated_pos);
    }

private:
    int currenttypeid = 0;
    Type type = Type::ALONG_X;

    Vector3 calculated_pos = Vector3(0, 0, 0);
    Vector3 start_pos = Vector3(0, 0, 0);
    Vector3 forward = Vector3(1, 0, 0);
    bool static_forward = true;
    bool along_forward = false;
    bool along_inited = false;
    bool curve_inited = false;

    std::vector<Vector3> points_copy;

    inline Vector3 bezierAt(float t)
    {
        Vector3 position;
        const int n = points_copy.size();
        for(int i = 0; i < n; i++)
        {
            position += bernsteinBasis(n, i, t) * points_copy[i];
        }
        return position;
    }

    inline float bernsteinBasis(int n, int i, float t)
    {
        assert(i < n);

        if(n < 11)
        {
            return BinomialTable[n][i] * (float)std::pow(t, i) * (float)std::pow(1 - t, n - i);
        }
        else
        {
            CachedBinKey binKey = CachedBinKey(n, i);
            auto cb = cached_binomials.find(binKey);
            if(cb == cached_binomials.end())
            {
                // Calculate the binomial
                float b = Math::Binomial(n, i);
                cached_binomials.emplace(binKey, b);
                return b * (float)std::pow(t, i) * (float)std::pow(1 - t, n - i);
            }
            else
            {
                return cb->second * (float)std::pow(t, i) * (float)std::pow(1 - t, n - i);
            }
        }
    }

    struct CachedBinKey
    {
        CachedBinKey(int n, int i) : n(n), i(i) {  }
        int n;
        int i;

        inline bool operator<(const CachedBinKey &k) const
        {
            return n < k.n || (n == k.n && i < k.i);
        }
    };

    std::map<CachedBinKey, float> cached_binomials;

    static constexpr float BinomialTable[11][11] = {
        { Math::Binomial( 0,  0) },
        { Math::Binomial( 1,  0), Math::Binomial( 1,  1) },
        { Math::Binomial( 2,  0), Math::Binomial( 2,  1), Math::Binomial( 2,  2) },
        { Math::Binomial( 3,  0), Math::Binomial( 3,  1), Math::Binomial( 3,  2), Math::Binomial( 3,  3) },
        { Math::Binomial( 4,  0), Math::Binomial( 4,  1), Math::Binomial( 4,  2), Math::Binomial( 4,  3), Math::Binomial( 4,  4) },
        { Math::Binomial( 5,  0), Math::Binomial( 5,  1), Math::Binomial( 5,  2), Math::Binomial( 5,  3), Math::Binomial( 5,  4), Math::Binomial( 5,  5) },
        { Math::Binomial( 6,  0), Math::Binomial( 6,  1), Math::Binomial( 6,  2), Math::Binomial( 6,  3), Math::Binomial( 6,  4), Math::Binomial( 6,  5), Math::Binomial( 6,  6) },
        { Math::Binomial( 7,  0), Math::Binomial( 7,  1), Math::Binomial( 7,  2), Math::Binomial( 7,  3), Math::Binomial( 7,  4), Math::Binomial( 7,  5), Math::Binomial( 7,  6), Math::Binomial( 7,  7) },
        { Math::Binomial( 8,  0), Math::Binomial( 8,  1), Math::Binomial( 8,  2), Math::Binomial( 8,  3), Math::Binomial( 8,  4), Math::Binomial( 8,  5), Math::Binomial( 8,  6), Math::Binomial( 8,  7), Math::Binomial( 8,  8) },
        { Math::Binomial( 9,  0), Math::Binomial( 9,  1), Math::Binomial( 9,  2), Math::Binomial( 9,  3), Math::Binomial( 9,  4), Math::Binomial( 9,  5), Math::Binomial( 9,  6), Math::Binomial( 9,  7), Math::Binomial( 9,  8), Math::Binomial( 9,  9) },
        { Math::Binomial(10,  0), Math::Binomial(10,  1), Math::Binomial(10,  2), Math::Binomial(10,  3), Math::Binomial(10,  4), Math::Binomial(10,  5), Math::Binomial(10,  6), Math::Binomial(10,  7), Math::Binomial(10,  8), Math::Binomial(10,  9), Math::Binomial(10,  10) },
    };
};
