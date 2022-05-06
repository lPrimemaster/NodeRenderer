#pragma once
#include "node.h"
#include "../../math/vector.h"
#include "../../../muparser/include/muParser.h"

struct FunctionNode final : public PropertyNode
{
    inline FunctionNode() : PropertyNode()
    {
        static int inc = 0;
        setInputsOrdered(
            {
                "x"
            }
        );
        _output_count = 1;
        name = "Function Node #" + std::to_string(inc++);

        px.DefineVar("x", &var_x);
        px.DefineVar("y", &var_y);
        px.DefineVar("z", &var_z);

        px.SetExpr("x");
        _expr_str[0] = 'x';
        _expr_str[1] = '\0';
    }

    
    ~FunctionNode() { }

    inline virtual void render() override
    {
        data.resetDataUpdate();

        disconnectInputIfNotOfType<float>("x");
        disconnectInputIfNotOfType<float>("y");
        disconnectInputIfNotOfType<float>("z");

        static const char* const mode_names[] = {
            "float -> float",
            "Vec2  -> float",
            "Vec3  -> float"
        };

        ImGui::Combo("In/Out Dims", &currentmodeid, mode_names, sizeof(mode_names) / sizeof(mode_names[0]));

        
        if(ImGui::InputText("Function", _expr_str, 512))
        {
            px.SetExpr(std::string(_expr_str));
        }

        if(currentmodeid != lastmodeid)
        {
            // Function type changed
            switch (currentmodeid)
            {
            case 0:
                setInputsOrdered(
                    {
                        "x"
                    }
                );
                break;
            case 1:
                setInputsOrdered(
                    {
                        "x",
                        "y"
                    }
                );
                break;
            case 2:
                setInputsOrdered(
                    {
                        "x",
                        "y",
                        "z"
                    }
                );
                break;
            default:
                break;
            }
            lastmodeid = currentmodeid;
        }

        if(!inputs.empty())
        {
            // BUG: Order matters here!
            switch (_input_count)
            {
                case 1:
                    var_x = inputs.begin()->second->data.getValue<float>();
                    break;
                case 2:
                    {
                        auto it = inputs.begin();
                        auto it2 = ++inputs.begin();
                        var_x = it->second->data.getValue<float>();

                        if(it2 != inputs.end())
                        {
                            var_y = it2->second->data.getValue<float>();
                        }
                        else
                        {
                            var_y = 0;
                        }
                    }
                    break;
                case 3:
                    {
                        auto it = inputs.begin();
                        auto it2 = ++inputs.begin();
                        auto it3 = ++++inputs.begin();
                        var_x = it->second->data.getValue<float>();

                        if(it2 != inputs.end())
                        {
                            var_y = it2->second->data.getValue<float>();

                            if(it3 != inputs.end())
                            {
                                var_z = it3->second->data.getValue<float>();
                            }
                            else
                            {
                                var_z = 0.0;
                            }
                        }
                        else
                        {
                            var_y = 0.0;
                            var_z = 0.0;
                        }
                    }
                    break;
                default:
                    break;
            }

            float r;
            try
            {
                r = px.Eval();
            }
            catch(mu::Parser::exception_type &e)
            {
                L_ERROR("Function evaluation failed: %s", e.GetMsg().c_str());
            }

            data.setValue(r);
            ImGui::BeginDisabled();
            ImGui::InputFloat("Result", &data.getValue<float>());
            ImGui::EndDisabled();
        }
    }

private:
    mu::Parser px;
    mu::Parser py;
    mu::Parser pz;
    mu::Parser pw;

    int currentmodeid = 0;
    int lastmodeid = 0;
    char _expr_str[512];

    double var_x = 0.0;
    double var_y = 0.0;
    double var_z = 0.0;
};
