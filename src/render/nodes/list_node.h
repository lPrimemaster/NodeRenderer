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

    inline ListNode() : PropertyNode()
    {
        static int inc = 0;
        setInputsOrdered(
            {
                "count"
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
        data.resetDataUpdate();

        disconnectInputIfNotOfType<unsigned int>("count");

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

        auto listsizeLocal = inputs_named.find("count");
        bool funcChanged = false;
        if(listsizeLocal != inputs_named.end())
        {
            unsigned int size = listsizeLocal->second->data.getValue<unsigned int>();
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

            ImGui::Text("Semi-colon separated. [ex.: \"a;b;\"]");
            if(ImGui::InputText("Extra variables", _extra_vars, 128))
            {
                std::vector<std::string> strings = { "count" };
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

                px.ClearVar();
                py.ClearVar();
                pz.ClearVar();
                pw.ClearVar();

                _vars.clear();

                px.DefineVar("i", &i_var);
                py.DefineVar("i", &i_var);
                pz.DefineVar("i", &i_var);
                pw.DefineVar("i", &i_var);

                // Ignore fix variable "count"
                _vars_name = std::vector<std::string>(strings.begin() + 1, strings.end());
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

            if(ImGui::InputText("x(i)", _expr_str_0, 128))
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
            if(currenttypeid > 2 && ImGui::InputText("y(i)", _expr_str_1, 128))
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
            if(currenttypeid > 3 && ImGui::InputText("z(i)", _expr_str_2, 128))
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
            if(currenttypeid > 4 && ImGui::InputText("w(i)", _expr_str_3, 128))
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
                            for(i_it = 0; i_it < size; i_it++)
                            {
                                i_var = i_it;
                                double x = px.Eval();
                                dataVal->at(i_it) = (float)x;
                            }
                        }
                        break;
                    case Type::INT:
                        {
                            std::vector<int>* dataVal = data.getValuePtr<std::vector<int>>();
                            for(i_it = 0; i_it < size; i_it++)
                            {
                                i_var = i_it;
                                double x = px.Eval();
                                dataVal->at(i_it) = (int)x;
                            }
                        }
                        break;
                    case Type::UINT:
                        {
                            std::vector<unsigned int>* dataVal = data.getValuePtr<std::vector<unsigned int>>();
                            for(i_it = 0; i_it < size; i_it++)
                            {
                                i_var = i_it;
                                double x = px.Eval();
                                dataVal->at(i_it) = (unsigned int)x;
                            }
                        }
                        break;
                    case Type::VECTOR2:
                        {
                            std::vector<Vector2>* dataVal = data.getValuePtr<std::vector<Vector2>>();
                            for(i_it = 0; i_it < size; i_it++)
                            {
                                i_var = i_it;
                                double x = px.Eval();
                                double y = py.Eval();
                                dataVal->at(i_it) = Vector2((float)x, (float)y);
                            }
                        }
                        break;
                    case Type::VECTOR3:
                        {
                            std::vector<Vector3>* dataVal = data.getValuePtr<std::vector<Vector3>>();
                            for(i_it = 0; i_it < size; i_it++)
                            {
                                i_var = i_it;
                                double x = px.Eval();
                                double y = py.Eval(); 0.0;
                                double z = pz.Eval(); 0.0;
                                (*dataVal)[i_it] = Vector3((float)x, (float)y, (float)z);
                            }
                        }
                        break;
                    case Type::VECTOR4:
                        {
                            std::vector<Vector4>* dataVal = data.getValuePtr<std::vector<Vector4>>();
                            for(i_it = 0; i_it < size; i_it++)
                            {
                                i_var = i_it;
                                double x = px.Eval();
                                double y = py.Eval();
                                double z = pz.Eval();
                                double w = pw.Eval();
                                dataVal->at(i_it) = Vector4((float)x, (float)y, (float)z, (float)w);
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
    }

private:
    unsigned int listsize = 0;
    int currenttypeid = 0;
    int lasttypeid = 0;

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
    std::vector<double> _vars;
    std::vector<double> _vars_last;
    std::vector<std::string> _vars_name;
};
