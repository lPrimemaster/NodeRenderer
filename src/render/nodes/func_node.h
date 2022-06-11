#pragma once
#include "node.h"
#include "../../math/vector.h"
#include "../../../muparser/include/muParser.h"

struct FunctionNode final : public PropertyNode
{
    inline FunctionNode() : PropertyNode(1, { "x" }, 1, { "f1" })
    {
        static int inc = 0;
        name = "Function Node #" + std::to_string(inc++);

        px.DefineVar("x", &var_x);
        px.DefineVar("y", &var_y);
        px.DefineVar("z", &var_z);

        py.DefineVar("x", &var_x);
        py.DefineVar("y", &var_y);
        py.DefineVar("z", &var_z);

        pz.DefineVar("x", &var_x);
        pz.DefineVar("y", &var_y);
        pz.DefineVar("z", &var_z);

        px.SetExpr("x");
        py.SetExpr("x");
        pz.SetExpr("x");

        _expr_str0[0] = 'x';
        _expr_str0[1] = '\0';

        _expr_str1[0] = 'x';
        _expr_str1[1] = '\0';

        _expr_str2[0] = 'x';
        _expr_str2[1] = '\0';
    }

    
    ~FunctionNode() { }

    inline virtual void update() override 
    {
        resetOutputsDataUpdate();
        
        auto x = inputs_named.find("x");
        auto y = inputs_named.find("y");
        auto z = inputs_named.find("z");

        if(x != inputs_named.end())
        {
            var_x = (double)x->second->getValue<float>();
        }
        else
        {
            var_x = 0.0;
        }

        if(y != inputs_named.end())
        {
            var_y = (double)y->second->getValue<float>();
        }
        else
        {
            var_y = 0.0;
        }

        if(z != inputs_named.end())
        {
            var_z = (double)z->second->getValue<float>();
        }
        else
        {
            var_z = 0.0;
        }

        switch (outcurrentmodeid)
        {
        case 2:
        {
            try
            {
                f3 = (float)pz.Eval();
            }
            catch(mu::Parser::exception_type &e)
            {
                L_ERROR("Function evaluation failed: %s", e.GetMsg().c_str());
            }
            setNamedOutput("f3", f3);
        }
        [[fallthrought]]
        case 1:
        {
            try
            {
                f2 = (float)py.Eval();
            }
            catch(mu::Parser::exception_type &e)
            {
                L_ERROR("Function evaluation failed: %s", e.GetMsg().c_str());
            }
            setNamedOutput("f2", f2);
        }
        [[fallthrought]]
        case 0:
        {
            try
            {
                f1 = (float)px.Eval();
            }
            catch(mu::Parser::exception_type &e)
            {
                L_ERROR("Function evaluation failed: %s", e.GetMsg().c_str());
            }
            setNamedOutput("f1", f1);
        }
        break;

        default:
            break;
        }
    }

    inline virtual void render() override
    {
        resetOutputsDataUpdate();

        disconnectInputIfNotOfType<float>("x");
        disconnectInputIfNotOfType<float>("y");
        disconnectInputIfNotOfType<float>("z");

        static const char* const in_mode_names[] = {
            "(x)",
            "(x,y)",
            "(x,y,z)"
        };

        static const char* const out_mode_names[] = {
            "(f)",
            "(f1,f2)",
            "(f1,f2,f3)"
        };

        ImGui::Combo("In Dims", &incurrentmodeid, in_mode_names, sizeof(in_mode_names) / sizeof(in_mode_names[0]));
        ImGui::Combo("Out Dims", &outcurrentmodeid, out_mode_names, sizeof(out_mode_names) / sizeof(out_mode_names[0]));
        
        if(ImGui::InputText((std::string("f1") + in_mode_names[incurrentmodeid]).c_str(), _expr_str0, 512))
        {
            px.SetExpr(std::string(_expr_str0));
        }

        if(outcurrentmodeid > 0)
        {
            if(ImGui::InputText((std::string("f2") + in_mode_names[incurrentmodeid]).c_str(), _expr_str1, 512))
            {
                py.SetExpr(std::string(_expr_str1));
            }
        }
        if(outcurrentmodeid > 1)
        {
            if(ImGui::InputText((std::string("f3") + in_mode_names[incurrentmodeid]).c_str(), _expr_str2, 512))
            {
                pz.SetExpr(std::string(_expr_str2));
            }
        }

        ImGui::BeginDisabled();
        ImGui::InputFloat("f1", &f1);
        if(outcurrentmodeid > 0) ImGui::InputFloat("f2", &f2);
        if(outcurrentmodeid > 1) ImGui::InputFloat("f3", &f3);
        ImGui::EndDisabled();

        if(incurrentmodeid != inlastmodeid)
        {
            // Function type changed
            switch (incurrentmodeid)
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
            inlastmodeid = incurrentmodeid;
        }

        if(outcurrentmodeid != outlastmodeid)
        {
            // Function type changed
            switch (outcurrentmodeid)
            {
            case 0:
                setOutputsOrdered(
                    {
                        "f1"
                    }
                );
                setNamedOutput("f1", f1);
                break;
            case 1:
                setOutputsOrdered(
                    {
                        "f1",
                        "f2"
                    }
                );
                setNamedOutput("f1", f1);
                setNamedOutput("f2", f2);
                break;
            case 2:
                setOutputsOrdered(
                    {
                        "f1",
                        "f2",
                        "f3"
                    }
                );
                setNamedOutput("f1", f1);
                setNamedOutput("f2", f2);
                setNamedOutput("f3", f3);
                break;
            default:
                break;
            }
            outlastmodeid = outcurrentmodeid;
        }
    }

private:
    mu::Parser px;
    mu::Parser py;
    mu::Parser pz;
    mu::Parser pw;

    int incurrentmodeid = 0;
    int outcurrentmodeid = 0;
    int inlastmodeid = 0;
    int outlastmodeid = 0;
    char _expr_str0[512];
    char _expr_str1[512];
    char _expr_str2[512];

    double var_x = 0.0;
    double var_y = 0.0;
    double var_z = 0.0;

    float f1 = 0.0f;
    float f2 = 0.0f;
    float f3 = 0.0f;
};
