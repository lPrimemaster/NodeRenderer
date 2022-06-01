#pragma once
#include "node.h"
#include "../../math/vector.h"
#include "../../../glm/glm/glm.hpp"
#include "../../../muparser/include/muParser.h"
#include <sstream>

struct ListNode final : public PropertyNode
{
    enum class Type
    {
        FLOAT,
        INT,
        UINT,
        VECTOR2,
        VECTOR3,
        VECTOR4
    } type;

    enum class Dim
    {
        D1,
        D2,
        D3
    } dim;

    inline ListNode() : PropertyNode()
    {
        static int inc = 0;
        setInputsOrdered(
            {
                "sizex"
            }
        );
        _output_count = 1;

        px.DefineVar("i", &i_var);
        py.DefineVar("i", &i_var);
        pz.DefineVar("i", &i_var);
        pw.DefineVar("i", &i_var);

        _expr_str_0[0] = '\0';
        _expr_str_1[0] = '\0';
        _expr_str_2[0] = '\0';
        _expr_str_3[0] = '\0';
        _extra_vars[0] = '\0';

        name = "List Node #" + std::to_string(inc++);
    }
    
    ~ListNode()
    {
        
    }

    inline virtual void render() override
    {
        static const char* const type_names[] = {
            "float",
            "int",
            "uint",
            "Vector2",
            "Vector3",
            "Vector4"
        };

        ImGui::Combo("Type", &currenttypeid, type_names, sizeof(type_names) / sizeof(type_names[0]));
        type = static_cast<Type>(currenttypeid);

        static const char* const dim_names[] = {
            "1D",
            "2D",
            "3D"
        };

        dim_changed = ImGui::Combo("Dim", &currentdimid, dim_names, sizeof(dim_names) / sizeof(dim_names[0]));
        dim = static_cast<Dim>(currentdimid);

        if(dim_changed)
        {
            switch (dim)
            {
            case Dim::D1:
                disconnectInputIfNotOfType<PropertyNode::EmptyType>("sizey");
                disconnectInputIfNotOfType<PropertyNode::EmptyType>("sizez");
                setInputsOrdered(
                    {
                        "sizex"
                    }
                );
                break;
            case Dim::D2:
                disconnectInputIfNotOfType<PropertyNode::EmptyType>("sizez");
                setInputsOrdered(
                    {
                        "sizex",
                        "sizey"
                    }
                );
                break;
            case Dim::D3:
                setInputsOrdered(
                    {
                        "sizex",
                        "sizey",
                        "sizez"
                    }
                );
                break;
            
            default:
                break;
            }
        }
                
        std::string vars;
        switch (dim)
        {
            case Dim::D1: vars = "i";       break;
            case Dim::D2: vars = "i, j";    break;
            case Dim::D3: vars = "i, j, k"; break;
            default: break;
        }

        _expr_changed0 = false;
        _expr_changed1 = false;
        _expr_changed2 = false;
        _expr_changed3 = false;

        if(has_a_dim)
        {
            ImGui::Text("Semi-colon separated. [ex.: \"a;b;\"]");
            extra_vars_changed = ImGui::InputText("Extra variables", _extra_vars, 128);
            _expr_changed0 = ImGui::InputText((std::string("x(") + vars + ")").c_str(), _expr_str_0, 128);
            _expr_changed1 = currenttypeid > 2 && ImGui::InputText((std::string("y(") + vars + ")").c_str(), _expr_str_1, 128);
            _expr_changed2 = currenttypeid > 3 && ImGui::InputText((std::string("z(") + vars + ")").c_str(), _expr_str_2, 128);
            _expr_changed3 = currenttypeid > 4 && ImGui::InputText((std::string("w(") + vars + ")").c_str(), _expr_str_3, 128);
        }
    }

    inline virtual void update() override
    {
        data.resetDataUpdate();

        disconnectInputIfNotOfType<unsigned int>("sizex");
        disconnectInputIfNotOfType<unsigned int>("sizey");
        disconnectInputIfNotOfType<unsigned int>("sizez");

        unsigned int size_x = 1;
        unsigned int size_y = 1;
        unsigned int size_z = 1;
        has_a_dim = false;

        switch (dim)
        {
        case Dim::D3:
        {
            auto listsizeLocal = inputs_named.find("sizez");
            if(listsizeLocal != inputs_named.end())
            {
                size_z = listsizeLocal->second->data.getValue<unsigned int>();
                has_a_dim = true;
            }
        }
        [[fallthrough]];
        case Dim::D2:
        {
            auto listsizeLocal = inputs_named.find("sizey");
            if(listsizeLocal != inputs_named.end())
            {
                size_y = listsizeLocal->second->data.getValue<unsigned int>();
                has_a_dim = true;
            }
        }
        [[fallthrough]];
        case Dim::D1:
        {
            auto listsizeLocal = inputs_named.find("sizex");
            if(listsizeLocal != inputs_named.end())
            {
                size_x = listsizeLocal->second->data.getValue<unsigned int>();
                has_a_dim = true;
            }
        }
        break;
        
        default:
            break;
        }

        bool funcChanged = false;
        if(has_a_dim)
        {
            unsigned int size = size_x * size_y * size_z;
            bool types_or_size_diff = (listsize != size) || (currenttypeid != lasttypeid);
            if(types_or_size_diff)
            {
                switch (type)
                {
                case Type::FLOAT:
                    if(currenttypeid != lasttypeid)
                    {
                        std::vector<float> newData;
                        newData.resize(size, 0.0f);
                        data.setValue<std::vector<float>>(newData);
                        lasttypeid = currenttypeid;
                    }
                    else
                    {
                        std::vector<float> newData;
                        newData.resize(size, 0.0f);
                        data.setValue<std::vector<float>>(newData);
                    }
                    break;
                case Type::INT:
                    if(currenttypeid != lasttypeid)
                    {
                        std::vector<int> newData;
                        newData.resize(size, 0);
                        data.setValue<std::vector<int>>(newData);
                        lasttypeid = currenttypeid;
                    }
                    else
                    {
                        std::vector<int> newData;
                        newData.resize(size, 0);
                        data.setValue<std::vector<int>>(newData);
                    }
                    break;
                case Type::UINT:
                    if(currenttypeid != lasttypeid)
                    {
                        std::vector<unsigned int> newData;
                        newData.resize(size, 0);
                        data.setValue<std::vector<unsigned int>>(newData);
                        lasttypeid = currenttypeid;
                    }
                    else
                    {
                        std::vector<unsigned int> newData;
                        newData.resize(size, 0);
                        data.setValue<std::vector<unsigned int>>(newData);
                    }
                    break;
                case Type::VECTOR2:
                    if(currenttypeid != lasttypeid)
                    {
                        std::vector<Vector2> newData;
                        newData.resize(size, Vector2(0, 0));
                        data.setValue<std::vector<Vector2>>(newData);
                        lasttypeid = currenttypeid;
                    }
                    else
                    {
                        std::vector<Vector2> newData;
                        newData.resize(size, Vector2(0, 0));
                        data.setValue<std::vector<Vector2>>(newData);
                    }
                    break;
                case Type::VECTOR3:
                    if(currenttypeid != lasttypeid)
                    {
                        std::vector<Vector3> newData;
                        newData.resize(size, Vector3(0, 0, 0));
                        data.setValue<std::vector<Vector3>>(newData);
                        lasttypeid = currenttypeid;
                    }
                    else
                    {
                        std::vector<Vector3> newData;
                        newData.resize(size, Vector3(0, 0, 0));
                        data.setValue<std::vector<Vector3>>(newData);
                    }
                    break;
                case Type::VECTOR4:
                    if(currenttypeid != lasttypeid)
                    {
                        std::vector<Vector4> newData;
                        newData.resize(size, Vector4(0, 0, 0, 0));
                        data.setValue<std::vector<Vector4>>(newData);
                        lasttypeid = currenttypeid;
                    }
                    else
                    {
                        std::vector<Vector4> newData;
                        newData.resize(size, Vector4(0, 0, 0, 0));
                        data.setValue<std::vector<Vector4>>(newData);
                    }
                    break;
                
                default:
                    break;
                }
                listsize = size;
            }

            if(extra_vars_changed || dim_changed)
            {
                std::vector<std::string> strings;
                int it_inc = 1 + static_cast<int>(dim);

                px.ClearVar();
                py.ClearVar();
                pz.ClearVar();
                pw.ClearVar();

                _vars.clear();

                switch (dim)
                {
                case Dim::D1: strings = { "sizex"                   }; break;
                case Dim::D2: strings = { "sizex", "sizey"          }; break;
                case Dim::D3: strings = { "sizex", "sizey", "sizez" }; break;
                default: break;
                }

                switch (dim)
                {
                case Dim::D3:
                    px.DefineVar("k", &k_var);
                    py.DefineVar("k", &k_var);
                    pz.DefineVar("k", &k_var);
                    pw.DefineVar("k", &k_var);
                    [[fallthrough]];
                case Dim::D2:
                    px.DefineVar("j", &j_var);
                    py.DefineVar("j", &j_var);
                    pz.DefineVar("j", &j_var);
                    pw.DefineVar("j", &j_var);
                    [[fallthrough]];
                case Dim::D1:
                    px.DefineVar("i", &i_var);
                    py.DefineVar("i", &i_var);
                    pz.DefineVar("i", &i_var);
                    pw.DefineVar("i", &i_var);
                    break;
                default:
                    break;
                }

                std::istringstream f(_extra_vars);
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
                _vars_name = std::vector<std::string>(strings.begin() + it_inc, strings.end());
                for(auto s : _vars_name)
                {
                    L_TRACE("Found additional var = %s", s.c_str());
                }

                _vars.reserve(strings.size() - 1);
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
                    _vars[i] = _var_it->second->data.getValue<float>();
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

            if(_expr_changed0)
            {
                try
                {
                    px.SetExpr(std::string(_expr_str_0));
                    funcChanged = true;
                }
                catch(mu::Parser::exception_type &e)
                {
                    L_ERROR("Function x set failed: %s", e.GetMsg().c_str());
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
                    L_ERROR("Function y set failed: %s", e.GetMsg().c_str());
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
                    L_ERROR("Function z set failed: %s", e.GetMsg().c_str());
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
                    L_ERROR("Function w set failed: %s", e.GetMsg().c_str());
                }
            }

            if(funcChanged || types_or_size_diff || variables_changed)
            {
                data.setDataChanged();

                try
                {
                    switch (type)
                    {
                    case Type::FLOAT:
                        {
                            std::vector<float>* dataVal = data.getValuePtr<std::vector<float>>();
                            for(k_it = 0; k_it < (int)size_z; k_it++)
                            {
                                k_var = k_it;
                                for(j_it = 0; j_it < (int)size_y; j_it++)
                                {
                                    j_var = j_it;
                                    for(i_it = 0; i_it < (int)size_x; i_it++)
                                    {
                                        i_var = i_it;
                                        double x = px.Eval();
                                        dataVal->at(i_it + j_it * size_x + k_it * size_y * size_z) = (float)x;
                                    }
                                }
                            }
                        }
                        break;
                    case Type::INT:
                        {
                            std::vector<int>* dataVal = data.getValuePtr<std::vector<int>>();
                            for(k_it = 0; k_it < (int)size_z; k_it++)
                            {
                                k_var = k_it;
                                for(j_it = 0; j_it < (int)size_y; j_it++)
                                {
                                    j_var = j_it;
                                    for(i_it = 0; i_it < (int)size_x; i_it++)
                                    {
                                        i_var = i_it;
                                        double x = px.Eval();
                                        dataVal->at(i_it + j_it * size_x + k_it * size_y * size_z) = (int)x;
                                    }
                                }
                            }
                        }
                        break;
                    case Type::UINT:
                        {
                            std::vector<unsigned int>* dataVal = data.getValuePtr<std::vector<unsigned int>>();
                            for(k_it = 0; k_it < (int)size_z; k_it++)
                            {
                                k_var = k_it;
                                for(j_it = 0; j_it < (int)size_y; j_it++)
                                {
                                    j_var = j_it;
                                    for(i_it = 0; i_it < (int)size_x; i_it++)
                                    {
                                        i_var = i_it;
                                        double x = px.Eval();
                                        dataVal->at(i_it + j_it * size_x + k_it * size_y * size_z) = (unsigned int)x;
                                    }
                                }
                            }
                        }
                        break;
                    case Type::VECTOR2:
                        {
                            std::vector<Vector2>* dataVal = data.getValuePtr<std::vector<Vector2>>();
                            for(k_it = 0; k_it < (int)size_z; k_it++)
                            {
                                k_var = k_it;
                                for(j_it = 0; j_it < (int)size_y; j_it++)
                                {
                                    j_var = j_it;
                                    for(i_it = 0; i_it < (int)size_x; i_it++)
                                    {
                                        i_var = i_it;
                                        double x = px.Eval();
                                        double y = py.Eval();
                                        dataVal->at(i_it + j_it * size_x + k_it * size_y * size_z) = Vector2((float)x, (float)y);
                                    }
                                }
                            }
                        }
                        break;
                    case Type::VECTOR3:
                        {
                            std::vector<Vector3>* dataVal = data.getValuePtr<std::vector<Vector3>>();
                            for(k_it = 0; k_it < (int)size_z; k_it++)
                            {
                                k_var = k_it;
                                for(j_it = 0; j_it < (int)size_y; j_it++)
                                {
                                    j_var = j_it;
                                    for(i_it = 0; i_it < (int)size_x; i_it++)
                                    {
                                        i_var = i_it;
                                        double x = px.Eval();
                                        double y = py.Eval();
                                        double z = pz.Eval();
                                        dataVal->at(i_it + j_it * size_x + k_it * size_y * size_z) = Vector3((float)x, (float)y, (float)z);
                                    }
                                }
                            }
                        }
                        break;
                    case Type::VECTOR4:
                        {
                            std::vector<Vector4>* dataVal = data.getValuePtr<std::vector<Vector4>>();
                            for(k_it = 0; k_it < (int)size_z; k_it++)
                            {
                                k_var = k_it;
                                for(j_it = 0; j_it < (int)size_y; j_it++)
                                {
                                    j_var = j_it;
                                    for(i_it = 0; i_it < (int)size_x; i_it++)
                                    {
                                        i_var = i_it;
                                        double x = px.Eval();
                                        double y = py.Eval();
                                        double z = pz.Eval();
                                        double w = pw.Eval();
                                        dataVal->at(i_it + j_it * size_x + k_it * size_y * size_z) = Vector4((float)x, (float)y, (float)z, (float)w);
                                    }
                                }
                            }
                        }
                        break;
                    
                    default:
                        break;
                    }
                }
                catch(mu::Parser::exception_type &e)
                {
                    L_ERROR("Function evaluation failed: %s", e.GetMsg().c_str());
                }
            }
        }
        else if(dim_changed) // Change current accepted it vars (i,j,k), even if no inputs are connected
        {
            px.ClearVar();
            py.ClearVar();
            pz.ClearVar();
            pw.ClearVar();

            switch (dim)
            {
            case Dim::D3:
                px.DefineVar("k", &k_var);
                py.DefineVar("k", &k_var);
                pz.DefineVar("k", &k_var);
                pw.DefineVar("k", &k_var);
                [[fallthrough]];
            case Dim::D2:
                px.DefineVar("j", &j_var);
                py.DefineVar("j", &j_var);
                pz.DefineVar("j", &j_var);
                pw.DefineVar("j", &j_var);
                [[fallthrough]];
            case Dim::D1:
                px.DefineVar("i", &i_var);
                py.DefineVar("i", &i_var);
                pz.DefineVar("i", &i_var);
                pw.DefineVar("i", &i_var);
                break;
            default:
                break;
            }
        }
    }

private:
    unsigned int listsize = 0;
    int currenttypeid = 0;
    int lasttypeid = 0;

    int currentdimid = 0;
    int lastdimid = 0;
    
    bool has_a_dim = false;
    bool dim_changed;
    bool extra_vars_changed;
    bool _expr_changed0;
    bool _expr_changed1;
    bool _expr_changed2;
    bool _expr_changed3;

    char _expr_str_0[128];
    char _expr_str_1[128];
    char _expr_str_2[128];
    char _expr_str_3[128];
    char _extra_vars[128];

    mu::Parser px;
    mu::Parser py;
    mu::Parser pz;
    mu::Parser pw;

    double i_var = 0.0;
    int i_it = 0;

    double j_var = 0.0;
    int j_it = 0;

    double k_var = 0.0;
    int k_it = 0;

    std::vector<double> _vars;
    std::vector<double> _vars_last;
    std::vector<std::string> _vars_name;
};
