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

        // TODO: Add the following operations: pow / abs / mod / exp / log / sin / cos / sinh / cosh
        static const char* const mode_names[] = {
            "A+B",
            "A-B",
            "AxB",
            "A/B"
        };

        ImGui::Combo("Mode", &currentmodeid, mode_names, sizeof(mode_names) / sizeof(mode_names[0]));
        mode = static_cast<Mode>(currentmodeid);

        disconnectInputIfNotOfType<float, Vector4>("A");
        disconnectInputIfNotOfType<float, Vector4>("B");

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

            if(first_node->data.isOfType<float>()) // Value
            {
                if(second_node)
                {
                    if(second_node->data.isOfType<float>()) // Value
                    {
                        data.setValue(
                            math_op(
                                first_node->data.getValue<float>(),
                                second_node->data.getValue<float>()
                            )
                        );
                    }
                    else if(second_node->data.isOfType<Vector4>()) // Color or vector
                    {
                        float* op_color = second_node->data.getValue<Vector4>().data;
                        float scalar = first_node->data.getValue<float>();
                        float r = math_op(scalar, op_color[0]);
                        float g = math_op(scalar, op_color[1]);
                        float b = math_op(scalar, op_color[2]);
                        float a = math_op(scalar, op_color[3]);
                        data.setValue(Vector4(r, g, b, a));
                    }
                    else // This node does not support this input type
                    {

                    }
                }
                else // Only one node (scalar)
                {
                    data.setValue(first_node->data.getValue<float>());
                }
            }
            else if(first_node->data.isOfType<Vector4>()) // Color or vector
            {
                if(second_node)
                {
                    if(second_node->data.isOfType<float>()) // Value
                    {
                        float* op_color = first_node->data.getValue<Vector4>().data;
                        float scalar = second_node->data.getValue<float>();
                        float r = math_op(scalar, op_color[0]);
                        float g = math_op(scalar, op_color[1]);
                        float b = math_op(scalar, op_color[2]);
                        float a = math_op(scalar, op_color[3]);
                        data.setValue(Vector4(r, g, b, a));
                    }
                    else if(second_node->data.isOfType<Vector4>()) // Color or vector
                    {
                        float* color1 = first_node->data.getValue<Vector4>().data;
                        float* color2 = second_node->data.getValue<Vector4>().data;
                        float r = math_op(color1[0], color2[0]);
                        float g = math_op(color1[1], color2[1]);
                        float b = math_op(color1[2], color2[2]);
                        float a = math_op(color1[3], color2[3]);
                        data.setValue(Vector4(r, g, b, a));
                    }
                    else // This node does not support this input type
                    {

                    }
                }
                else // Only one node (color or vector)
                {
                    data.setValue(first_node->data.getValue<Vector4>());
                }
            }
            else // This node does not support this input type
            {

            }

            ImGui::BeginDisabled();
            if(data.isOfType<float>())
            {
                ImGui::InputFloat("Result", &data.getValue<float>());
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

    template<typename T>
    T math_op(T a, T b)
    {
        switch (mode)
        {
            case Mode::ADD: return a + b;
            case Mode::SUB: return a - b;
            case Mode::MUL: return a * b;
            case Mode::DIV: return a / b;
            default: return 0;
        }
    }
};
