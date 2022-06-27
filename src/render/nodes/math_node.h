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

    inline MathNode(Mode m = Mode::ADD) : PropertyNode(Type::MATH, 2, { "A", "B" }, 1, { "result" })
    {
        static int inc = 0;
        name = "Math Node #" + std::to_string(inc++);
        mode = m;
    }
    
    ~MathNode() {  }

    inline virtual void render() override
    {
        auto data = outputs[0];

        static const char* const mode_names[] = {
            "A+B",
            "A-B",
            "AxB",
            "A/B"
        };

        ImGui::Combo("Mode", &currentmodeid, mode_names, sizeof(mode_names) / sizeof(mode_names[0]));
        mode = static_cast<Mode>(currentmodeid);

        if(!inputs.empty())
        {   
            ImGui::BeginDisabled();
            if(data->isOfType<int>())
            {
                ImGui::InputInt("Result", &data->getValue<int>());
            }
            else if(data->isOfType<unsigned int>())
            {
                ImGui::InputScalar("Result", ImGuiDataType_U32, &data->getValue<unsigned int>());
            }
            else if(data->isOfType<float>())
            {
                ImGui::InputFloat("Result", &data->getValue<float>());
            }
            else if(data->isOfType<Vector2>())
            {
                ImGui::InputFloat2("Result", data->getValue<Vector2>().data);
            }
            else if(data->isOfType<Vector3>())
            {
                ImGui::InputFloat3("Result", data->getValue<Vector3>().data);
            }
            else if(data->isOfType<Vector4>())
            {
                ImGui::InputFloat4("Result", data->getValue<Vector4>().data);
            }
            ImGui::EndDisabled();
        }
    }

    inline virtual void update() override 
    {
        outputs[0]->resetDataUpdate();

        disconnectInputIfNotOfType<int, unsigned int, float, Vector2, Vector3, Vector4>("A");
        disconnectInputIfNotOfType<int, unsigned int, float, Vector2, Vector3, Vector4>("B");

        if(!inputs.empty())
        {
            PropertyGenericData* first_data = inputs.begin()->second;
            PropertyGenericData* second_data = nullptr;
            auto it = ++inputs.begin();
            if(it != inputs.end())
            {
                // There is a second node connected available
                second_data = it->second;
            }

            if(second_data)
            {
                if(second_data->type == first_data->type)
                {
                    assingAllTypes<int, unsigned int, float, Vector2, Vector3, Vector4>(first_data, second_data);
                }
                else
                {
                    L_ERROR("MathNode: Inputs must have the same type.");
                }
            }
            else
            {
                assingAllTypes<int, unsigned int, float, Vector2, Vector3, Vector4>(first_data, nullptr);
            }
        }
    }

private:
    int currentmodeid = 0;

    template<typename T1, typename T2, typename... Args>
    inline bool assingAllTypes(PropertyGenericData* f, PropertyGenericData* s)
    {
        if(s == nullptr)
        {
            if(f->isOfType<T1>())
            {
                outputs[0]->setValue(f->getValue<T1>());
                return true;
            }
        }
        else
        {
            if(f->isOfType<T1>() && s->isOfType<T1>())
            {
                outputs[0]->setValue(math_op(f->getValue<T1>(), s->getValue<T1>()));
                return true;
            }
        }
        return assingAllTypes<T2, Args...>(f, s);
    }

    template<typename T>
    inline bool assingAllTypes(PropertyGenericData* f, PropertyGenericData* s)
    {
        if(s == nullptr)
        {
            if(f->isOfType<T>())
            {
                outputs[0]->setValue(f->getValue<T>());
                return true;
            }
        }
        else
        {
            if(f->isOfType<T>() && s->isOfType<T>())
            {
                outputs[0]->setValue(math_op(f->getValue<T>(), s->getValue<T>()));
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
