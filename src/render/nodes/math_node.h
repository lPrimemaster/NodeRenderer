#pragma once
#include "node.h"
#include "../../math/vector.h"

struct MathNode final : public PropertyNode
{
    enum class Mode
    {
        ADD,
        SUB,
        MUL,
        DIV
    } mode;

    inline MathNode(Mode m = Mode::ADD) : PropertyNode()
    {
        static int inc = 0;
        setInputsOrdered(
            {
                "A",
                "B"
            }
        );
        _output_count = 1;
        name = "Math Node #" + std::to_string(inc++);
        mode = m;
    }
    
    ~MathNode() {  }

    inline virtual void render() override
    {
        data.resetDataUpdate();

        static const char* const mode_names[] = {
            "A+B",
            "A-B",
            "AxB",
            "A/B"
        };

        ImGui::Combo("Mode", &currentmodeid, mode_names, sizeof(mode_names) / sizeof(mode_names[0]));
        mode = static_cast<Mode>(currentmodeid);

        disconnectInputIfNotOfType<int, unsigned int, float, Vector2, Vector3, Vector4>("A");
        disconnectInputIfNotOfType<int, unsigned int, float, Vector2, Vector3, Vector4>("B");

        if(!inputs.empty())
        {   
            PropertyNode* first_node = inputs.begin()->second;
            PropertyNode* second_node = nullptr;
            auto it = ++inputs.begin();
            if(it != inputs.end())
            {
                // There is a second node connected available
                second_node = it->second;
            }

            if(second_node)
            {
                if(second_node->data.type == first_node->data.type)
                {
                    assingAllTypes<int, unsigned int, float, Vector2, Vector3, Vector4>(first_node, second_node);
                }
                else
                {
                    L_ERROR("MathNode: Inputs must have the same type.");
                }
            }
            else
            {
                assingAllTypes<int, unsigned int, float, Vector2, Vector3, Vector4>(first_node, nullptr);
            }

            ImGui::BeginDisabled();
            if(data.isOfType<int>())
            {
                ImGui::InputInt("Result", &data.getValue<int>());
            }
            else if(data.isOfType<unsigned int>())
            {
                ImGui::InputScalar("Result", ImGuiDataType_U32, &data.getValue<unsigned int>());
            }
            else if(data.isOfType<float>())
            {
                ImGui::InputFloat("Result", &data.getValue<float>());
            }
            else if(data.isOfType<Vector2>())
            {
                ImGui::InputFloat2("Result", data.getValue<Vector2>().data);
            }
            else if(data.isOfType<Vector3>())
            {
                ImGui::InputFloat3("Result", data.getValue<Vector3>().data);
            }
            else if(data.isOfType<Vector4>())
            {
                ImGui::InputFloat4("Result", data.getValue<Vector4>().data);
            }
            ImGui::EndDisabled();
        }
    }

private:
    int currentmodeid = 0;

    template<typename T1, typename T2, typename... Args>
    inline bool assingAllTypes(PropertyNode* f, PropertyNode* s)
    {
        if(s == nullptr)
        {
            if(f->data.isOfType<T1>())
            {
                data.setValue(f->data.getValue<T1>());
                return true;
            }
        }
        else
        {
            if(f->data.isOfType<T1>() && s->data.isOfType<T1>())
            {
                data.setValue(math_op(f->data.getValue<T1>(), s->data.getValue<T1>()));
                return true;
            }
        }
        return assingAllTypes<T2, Args...>(f, s);
    }

    template<typename T>
    inline bool assingAllTypes(PropertyNode* f, PropertyNode* s)
    {
        if(s == nullptr)
        {
            if(f->data.isOfType<T>())
            {
                data.setValue(f->data.getValue<T>());
                return true;
            }
        }
        else
        {
            if(f->data.isOfType<T>() && s->data.isOfType<T>())
            {
                data.setValue(math_op(f->data.getValue<T>(), s->data.getValue<T>()));
                return true;
            }
        }
        return false;
    }

    template<typename T>
    T math_op(T a, T b)
    {
        switch (mode)
        {
            case Mode::ADD: return a + b;
            case Mode::SUB: return a - b;
            case Mode::MUL: return a * b;
            case Mode::DIV: return a / b;
        }
        return T(); // NOTE: Assuming there is a default ctor on the type
    }
};
