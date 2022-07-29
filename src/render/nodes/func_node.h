#pragma once
#include "node.h"
#include "../../math/vector.h"
#include "../../../muparser/include/muParser.h"

struct FunctionNode final : public PropertyNode
{
    inline FunctionNode() : PropertyNode(Type::FUNCTION, 1, { "x" }, 1, { "value" })
    {
        static int inc = 0;
        name = "Function Node #" + std::to_string(inc++);

        _vars_name = std::vector<std::string>(1, "x");
        for(auto s : _vars_name)
        {
            L_TRACE("Found var = %s", s.c_str());
        }

        _vars.reserve(1);
        for(auto s : _vars_name)
        {
            _vars.push_back(0.0);
            double* v = &*(_vars.end() - 1);
            px.DefineVar(s, v);
            py.DefineVar(s, v);
            pz.DefineVar(s, v);
            pw.DefineVar(s, v);
        }
        _vars_last = _vars;

        outputs[0]->setValue(0.0f);

        px.SetExpr("x");
        py.SetExpr("x");
        pz.SetExpr("x");
        pw.SetExpr("x");

        strcpy(_vars_str, "x;");

        _expr_str_0[0] = 'x';
        _expr_str_0[1] = '\0';

        _expr_str_1[0] = 'x';
        _expr_str_1[1] = '\0';

        _expr_str_2[0] = 'x';
        _expr_str_2[1] = '\0';

        _expr_str_3[0] = 'x';
        _expr_str_3[1] = '\0';
    }

    
    ~FunctionNode() { }

    inline virtual void update() override 
    {
        resetOutputsDataUpdate();
        
        if(vars_changed)
        {
            px.ClearVar();
            py.ClearVar();
            pz.ClearVar();
            pw.ClearVar();
            _vars.clear();

            std::vector<std::string> strings;
            std::istringstream f(_vars_str);
            std::string s;
            while (std::getline(f, s, ';'))
            {
                strings.push_back(s);
            }

            // For now disconnect the inputs if there are any
            // Forcing the user to reconnect to the node again
            for(auto v : _vars_name)
            {
                disconnectInputIfNotOfType<PropertyNode::EmptyType>(v);
            }

            // TODO: Create a new input set were the currently connected ones are mutated to allow for on the fly name change
            setInputsOrdered(strings);

            // Ignore fix variable "count"
            _vars_name = std::vector<std::string>(strings.begin(), strings.end());
            for(auto s : _vars_name)
            {
                L_TRACE("Found var = %s", s.c_str());
            }

            _vars.reserve(strings.size());
            for(auto s : _vars_name)
            {
                _vars.push_back(0.0);
                double* v = &*(_vars.end() - 1);
                px.DefineVar(s, v);
                py.DefineVar(s, v);
                pz.DefineVar(s, v);
                pw.DefineVar(s, v);
            }
            _vars_last = _vars;
        }

        for(auto s : _vars_name)
        {
            disconnectInputIfNotOfType<float>(s);
        }

        bool variables_changed = false;
        for(int i = 0; i < _vars.size(); i++)
        {
            auto _var_it = inputs_named.find(_vars_name[i]);
            if(_var_it != inputs_named.end())
            {
                _vars[i] = _var_it->second->getValue<float>();
            }
            else
            {
                _vars[i] = 0.0;
            }

            if(_vars[i] != _vars_last[i])
            {
                _vars_last[i] = _vars[i];
                variables_changed = true;
            }
        }

        bool funcChanged = false;
        if(_expr_changed0)
        {
            try
            {
                px.SetExpr(std::string(_expr_str_0));
                funcChanged = true;
            }
            catch(mu::Parser::exception_type &e)
            {
                L_ERROR("Function fx set failed: %s", e.GetMsg().c_str());
            }
        }
        if(_expr_changed1)
        {
            try
            {
                py.SetExpr(std::string(_expr_str_1));
                funcChanged = true;
            }
            catch(mu::Parser::exception_type &e)
            {
                L_ERROR("Function fy set failed: %s", e.GetMsg().c_str());
            }
        }
        if(_expr_changed2)
        {
            try
            {
                pz.SetExpr(std::string(_expr_str_2));
                funcChanged = true;
            }
            catch(mu::Parser::exception_type &e)
            {
                L_ERROR("Function fz set failed: %s", e.GetMsg().c_str());
            }
        }
        if(_expr_changed3)
        {
            try
            {
                pw.SetExpr(std::string(_expr_str_3));
                funcChanged = true;
            }
            catch(mu::Parser::exception_type &e)
            {
                L_ERROR("Function fw set failed: %s", e.GetMsg().c_str());
            }
        }

        if(funcChanged || variables_changed || out_type_changed)
        {
            try
            {
                switch (currentmodeid)
                {
                case 0: outputs[0]->setValue((float)px.Eval()); break;
                case 1: outputs[0]->setValue(Vector2((float)px.Eval(), (float)py.Eval())); break;
                case 2: outputs[0]->setValue(Vector3((float)px.Eval(), (float)py.Eval(), (float)pz.Eval())); break;
                case 3: outputs[0]->setValue(Vector4((float)px.Eval(), (float)py.Eval(), (float)pz.Eval(), (float)pw.Eval())); break;
                default: break;
                }
            }
            catch(mu::Parser::exception_type &e)
            {
                L_ERROR("Function evaluation failed: %s", e.GetMsg().c_str());
            }
        }
    }

    inline virtual void render() override
    {
        static const char* const out_mode_names[] = {
            "scalar",
            "vector2",
            "vector3",
            "vector4"
        };

        ImGui::Combo("Out Type", &currentmodeid, out_mode_names, sizeof(out_mode_names) / sizeof(out_mode_names[0]));
        
        if(currentmodeid != lastmodeid)
        {
            lastmodeid = currentmodeid;
            out_type_changed = true;
        }
        else
        {
            out_type_changed = false;
        }
        
        ImGui::Text("Semi-colon separated. [ex.: \"a;b;\"]");
        vars_changed = ImGui::InputText("Variables", _vars_str, 128);

        _expr_changed0 = ImGui::InputText("fx", _expr_str_0, 512);
        if(currentmodeid > 0) _expr_changed1 = ImGui::InputText("fy", _expr_str_1, 512);
        if(currentmodeid > 1) _expr_changed2 = ImGui::InputText("fz", _expr_str_2, 512);
        if(currentmodeid > 2) _expr_changed3 = ImGui::InputText("fw", _expr_str_3, 512);
    }

    inline virtual ByteBuffer serialize() const override
    {
        ByteBuffer buffer = PropertyNode::serialize();

        buffer.add(currentmodeid);

        buffer.add(std::string(_vars_str));

        buffer.add(std::string(_expr_str_0));
        buffer.add(std::string(_expr_str_1));
        buffer.add(std::string(_expr_str_2));
        buffer.add(std::string(_expr_str_3));

        return buffer;
    }

    inline virtual void deserialize(ByteBuffer& buffer) override
    {
        PropertyNode::deserialize(buffer);

        buffer.get(&currentmodeid);
        lastmodeid = currentmodeid;

        std::string varscpy;

        buffer.get(&varscpy);
        strcpy(_vars_str, varscpy.c_str());

        std::vector<std::string> strings;
        std::istringstream f(_vars_str);
        std::string s;
        while (std::getline(f, s, ';'))
        {
            strings.push_back(s);
        }
        setInputsOrdered(strings);

        std::string expressions[4];

        buffer.get(&expressions[0]);
        buffer.get(&expressions[1]);
        buffer.get(&expressions[2]);
        buffer.get(&expressions[3]);

        strcpy(_expr_str_0, expressions[0].c_str());
        strcpy(_expr_str_1, expressions[1].c_str());
        strcpy(_expr_str_2, expressions[2].c_str());
        strcpy(_expr_str_3, expressions[3].c_str());

        _expr_changed0 = true;
        _expr_changed1 = true;
        _expr_changed2 = true;
        _expr_changed3 = true;

        vars_changed = false;

        // Ignore fix variable "count"
        _vars_name = std::vector<std::string>(strings.begin(), strings.end());
        for(auto s : _vars_name)
        {
            L_TRACE("Found var = %s", s.c_str());
        }

        _vars.clear();
        _vars.reserve(strings.size());
        for(auto s : _vars_name)
        {
            _vars.push_back(0.0);
            double* v = &*(_vars.end() - 1);
            px.DefineVar(s, v);
            py.DefineVar(s, v);
            pz.DefineVar(s, v);
            pw.DefineVar(s, v);
        }
        _vars_last = _vars;
    }

private:
    int currentmodeid = 0;
    int lastmodeid = 0;

    bool vars_changed;
    bool _expr_changed0;
    bool _expr_changed1;
    bool _expr_changed2;
    bool _expr_changed3;
    bool out_type_changed;

    mu::Parser px;
    mu::Parser py;
    mu::Parser pz;
    mu::Parser pw;

    char _expr_str_0[512];
    char _expr_str_1[512];
    char _expr_str_2[512];
    char _expr_str_3[512];
    char _vars_str[128];

    std::vector<double> _vars;
    std::vector<double> _vars_last;
    std::vector<std::string> _vars_name;
};
