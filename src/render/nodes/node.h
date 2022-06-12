#pragma once
#include <string>
#include <map>
#include <type_traits>
#include <typeinfo>
#include <typeindex>
#include <algorithm>
#include <iterator>
#include <vector>
#include "../../../imgui/imgui.h"
#include "../../log/logger.h"
#include "../../math/vector.h"

struct NodeRenderData
{
    ImVec2 pos;
    ImVec2 size;
};

struct IOIdxData
{
    unsigned char self_idx;
    unsigned char other_idx;

    bool operator<(const IOIdxData &o) const
    {
        return (int)(self_idx + (int)(other_idx << 8)) < (int)(o.self_idx + (int)(o.other_idx << 8));
    }
};

struct PropertyNode;


struct PropertyGenericData
{

    enum class ValidType
    {
        FLOAT,
        INT,
        UINT,
        VECTOR2,
        VECTOR3,
        VECTOR4,

        LIST_FLOAT,
        LIST_INT,
        LIST_UINT,
        LIST_VECTOR2,
        LIST_VECTOR3,
        LIST_VECTOR4
    };

    template<typename T>
    PropertyGenericData(T value, PropertyNode* data_holder) : type(typeid(T)), _data_holder_instance(data_holder)
    {
        data = new T(value);
        _data_changed = true;
    }

    template<typename T, size_t N>
    PropertyGenericData(const T (&arr)[N], PropertyNode* data_holder) : type(typeid(T*)), _data_holder_instance(data_holder)
    {
        size = N;
        data = new T[N];
        memcpy(data, arr, N * sizeof(T));
        is_fixed_array = true;
        _data_changed = true;
    }

    // No copy
    PropertyGenericData(const PropertyGenericData& p) = delete;

    ~PropertyGenericData()
    {
        if(data)
        {
            if(!is_fixed_array)
            {
                delete data;
            }
            else
            {
                delete[] data;
            }
        }
    }
    
    template<typename T>
    inline bool isOfType()
    {
        return std::type_index(typeid(T)) == type;
    }

    template<typename T, typename U, typename... Args>
    inline bool isOfType()
    {
        return std::type_index(typeid(T)) == type || isOfType<U, Args...>();
    }

    template<typename T, typename = std::enable_if_t<!std::is_pointer<T>::value>>
    inline T& getValue()
    {
        if(!is_fixed_array)
        {
            return (*(T*)data);
        }
        else
        {
            L_ERROR("This PropertyGenericData stores an array. Attempted method call without a pointer type.");
            // NOTE : This value is invalid
            return (*(T*)data);
        }
    }

    template<typename T, typename = std::enable_if_t<std::is_pointer<T>::value>>
    inline T getValue()
    {
        if(is_fixed_array)
        {
            return ((T)data);
        }
        else
        {
            L_ERROR("This PropertyGenericData stores a value. Attempted method call with a pointer type.");
            return nullptr;
        }
    }

    // NOTE: May be unsafe. Use with caution.
    template<typename T>
    inline T* getValuePtr()
    {
        return ((T*)data);
    }

    template<typename T>
    inline void setValue(T value)
    {
        if(isOfType<T>())
        {
            if(!is_fixed_array)
            {
                (*(T*)data) = value;
                _data_changed = true;
            }
            else
            {
                L_ERROR("This PropertyGenericData stores an array. Attempted method call without a pointer type.");
            }
        }
        else
        {
            L_DEBUG("PropertyGenericData changed base datatype.");
            if(!is_fixed_array)
            {
                delete data;
            }
            else
            {
                delete[] data;
            }

            is_fixed_array = false;
            data = new T(value);
            type = std::type_index(typeid(T));
            checkAssignTypeEnum();
            _data_changed = true;
        }
    }

    template<typename T, size_t N>
    inline void setValue(const T (&arr)[N])
    {
        if(isOfType<T*>())
        {
            if(is_fixed_array)
            {
                size = N;
                memcpy(data, arr, N * sizeof(T));
                _data_changed = true;
            }
            else
            {
                L_ERROR("This PropertyGenericData stores a value. Attempted method call with a pointer type.");
            }
        }
        else
        {
            L_DEBUG("PropertyGenericData changed base datatype.");
            if(!is_fixed_array)
            {
                delete data;
            }
            else
            {
                delete[] data;
            }
            size = N;
            data = new T[N];
            memcpy(data, arr, N * sizeof(T));
            is_fixed_array = true;
            type = std::type_index(typeid(T*));
            _data_changed = true;
        }
    }

    struct TypeDataBuffer
    {
        ValidType vtype;
        void* data;
    };

    inline TypeDataBuffer getValueDynamic()
    {
        TypeDataBuffer tdb;
        tdb.vtype = vtype;
        tdb.data = data;
        return tdb;
    }

    inline void setValueDynamic(const TypeDataBuffer& b)
    {
        switch (b.vtype)
        {
        case ValidType::FLOAT:   setValue<float>(*(float*)b.data); break;
        case ValidType::INT:     setValue<int>(*(int*)b.data); break;
        case ValidType::UINT:    setValue<unsigned int>(*(unsigned int*)b.data); break;
        case ValidType::VECTOR2: setValue<Vector2>(*(Vector2*)b.data); break;
        case ValidType::VECTOR3: setValue<Vector3>(*(Vector3*)b.data); break;
        case ValidType::VECTOR4: setValue<Vector4>(*(Vector4*)b.data); break;

        case ValidType::LIST_FLOAT:   setValue<std::vector<float>>(*(std::vector<float>*)b.data); break;
        case ValidType::LIST_INT:     setValue<std::vector<int>>(*(std::vector<int>*)b.data); break;
        case ValidType::LIST_UINT:    setValue<std::vector<unsigned int>>(*(std::vector<unsigned int>*)b.data); break;
        case ValidType::LIST_VECTOR2: setValue<std::vector<Vector2>>(*(std::vector<Vector2>*)b.data); break;
        case ValidType::LIST_VECTOR3: setValue<std::vector<Vector3>>(*(std::vector<Vector3>*)b.data); break;
        case ValidType::LIST_VECTOR4: setValue<std::vector<Vector4>>(*(std::vector<Vector4>*)b.data); break;
        default: L_ERROR("setValueDynamic(): Received a non valid type."); break;
        }
    }

    inline bool dataChanged() const
    {
        return _data_changed;
    }

    inline void resetDataUpdate()
    {
        _data_changed = false;
    }

    inline void setDataChanged()
    {
        _data_changed = true;
    }

    ValidType vtype;
    std::type_index type;
    void* data = nullptr;
    size_t size = 0ULL;
    bool is_fixed_array = false;
    bool _data_changed = false;
    PropertyNode* _data_holder_instance = nullptr;

private:
    inline void checkAssignTypeEnum()
    {
             if(isOfType<float>())        vtype = ValidType::FLOAT;
        else if(isOfType<int>())          vtype = ValidType::INT;
        else if(isOfType<unsigned int>()) vtype = ValidType::UINT;
        else if(isOfType<Vector2>())      vtype = ValidType::VECTOR2;
        else if(isOfType<Vector3>())      vtype = ValidType::VECTOR3;
        else if(isOfType<Vector4>())      vtype = ValidType::VECTOR4;
        else if(isOfType<std::vector<float>>())        vtype = ValidType::LIST_FLOAT;
        else if(isOfType<std::vector<int>>())          vtype = ValidType::LIST_INT;
        else if(isOfType<std::vector<unsigned int>>()) vtype = ValidType::LIST_UINT;
        else if(isOfType<std::vector<Vector2>>())      vtype = ValidType::LIST_VECTOR2;
        else if(isOfType<std::vector<Vector3>>())      vtype = ValidType::LIST_VECTOR3;
        else if(isOfType<std::vector<Vector4>>())      vtype = ValidType::LIST_VECTOR4;
    }
};

// TODO: Node and in/out types color
struct PropertyNode
{
    struct EmptyType {  };

    PropertyNode
    (
        int inputs_count = 0, 
        std::vector<std::string> inputs_name = std::vector<std::string>(), 
        int outputs_count = 1, 
        std::vector<std::string> outputs_name = std::vector<std::string>()
    )
    {
        if(inputs_count > 0)
        {
            if(inputs_name.size() != inputs_count)
            {
                L_ERROR("PropertyNode set to hold %d inputs but %d names were specified.", inputs_count, inputs_name.size());
            }
            setInputsOrdered(inputs_name);
        }
        if(outputs_count >= 1)
        {
            if(outputs_name.size() == 0 && outputs_count == 1)
            {
                outputs.push_back(new PropertyGenericData(EmptyType(), this));
            }
            else if(outputs_name.size() == outputs_count)
            {
                setOutputsOrdered(outputs_name);
            }
            else
            {
                L_ERROR("PropertyNode set to hold %d inputs but %d names were specified.", outputs_count, outputs_name.size());
            }
        }
        else
        {
            L_ERROR("A PropertyNode needs to have at least one output.");
        }
    }

    inline virtual ~PropertyNode()
    { 
        for(auto data : outputs)
        {
            delete data;
        }
    }
    
    // Identification
    int id;
    std::string name;
    
    // Inputs
    int _input_count = 0;
    std::vector<std::string> _input_labels;
    std::vector<std::string> _output_labels;
    
    float _input_max_pad_px = 0.0f;
    float _output_max_pad_px = 0.0f;
    inline static constexpr float _text_pad_pad_px = 20.0f;

    std::map<IOIdxData, PropertyGenericData*> inputs;
    std::map<std::string, PropertyGenericData*> inputs_named;
    
    // Outputs
    int _output_count = 0;
    std::vector<IOIdxData> output_dependencies;

    // This is the "main" output
    std::vector<PropertyGenericData*> outputs;
    std::map<std::string, PropertyGenericData*> outputs_named;

    template<typename T>
    inline bool setNamedOutput(const std::string& name, T data)
    {
        auto output = outputs_named.find(name);
        if(output != outputs_named.end())
        {
            output->second->setValue<T>(data);
            return true;
        }
        L_ERROR("Node %s does not have an output named %s.", this->name.c_str(), name.c_str());
        return false;
    }

    template<typename T, size_t N>
    inline bool setNamedOutput(const std::string& name, const T (&arr)[N])
    {
        auto output = outputs_named.find(name);
        if(output != outputs_named.end())
        {
            output->second->setValue<T, N>(arr);
            return true;
        }
        L_ERROR("Node %s does not have an output named %s.", this->name.c_str(), name.c_str());
        return false;
    }

    inline void resetOutputsDataUpdate()
    {
        for(int i = 0; i < outputs.size(); i++)
        {
            outputs[i]->resetDataUpdate();
        }
    }

    // Display
    NodeRenderData _render_data;

    template<typename... Args>
    inline bool disconnectInputIfNotOfType(const std::string& inputName)
    {
        auto input = inputs_named.find(inputName);
        if(input != inputs_named.end())
        {
            PropertyNode* other = input->second->_data_holder_instance;
            PropertyGenericData* other_data = input->second;
            if(!other_data->isOfType<Args...>())
            {
                // Clear the input dependencies of the node links
                for(IOIdxData out_dep : other->output_dependencies)
                {
                    auto fit = inputs.find(out_dep);
                    if(fit != inputs.end())
                    {
                        inputs.erase(fit);
                        inputs_named.erase(inputName);
                    }
                }

                L_WARNING("This node requires an input with types:");
                for(auto tid : { std::type_index(typeid(Args))... })
                {
                    L_WARNING("%s", tid.name());
                }
                L_WARNING("Supplied type:\n%s", other_data->type.name());
                return true;
            }
        }
        return false;
    }

    inline void setInputsOrdered(std::vector<std::string> in)
    {
        _input_count = (int)in.size();
        _input_labels = in;

        _input_max_pad_px = ImGui::CalcTextSize(
            std::max_element(
                in.begin(),
                in.end(),
                [](const auto& a, const auto& b) {
                    return a.size() < b.size();
                }
            )->c_str()
        ).x + _text_pad_pad_px;
    }

    inline void setOutputsOrdered(std::vector<std::string> out)
    {
        int diff = (int)(out.size() - outputs.size());
        if(diff > 0)
        {
            // Grow
            int old_size = (int)outputs.size();
            int new_size = old_size + diff;
            outputs.resize(new_size);
            for(int i = old_size; i < new_size; i++)
            {
                outputs[i] = new PropertyGenericData(EmptyType(), this);
            }
            outputs_named.clear();
            for(int i = 0; i < out.size(); i++)
            {
                outputs_named.emplace(out[i], outputs[i]);
            }
        }
        else if(diff < 0)
        {
            // Shrink
            int old_size = (int)outputs.size();
            int new_size = old_size + diff;
            for(int i = old_size - 1; i > old_size + diff - 1; i--)
            {
                delete outputs[i];
            }
            outputs.resize(new_size);
            outputs_named.clear();
            for(int i = 0; i < (int)out.size(); i++)
            {
                outputs_named.emplace(out[i], outputs[i]);
            }
        }
        else
        {
            // Only rename
            outputs_named.clear();
            for(int i = 0; i < (int)out.size(); i++)
            {
                outputs_named.emplace(out[i], outputs[i]);
            }
        }
        _output_count = (int)out.size();
        _output_labels = out;

        _output_max_pad_px = ImGui::CalcTextSize(
            std::max_element(
                out.begin(),
                out.end(),
                [](const auto& a, const auto& b) {
                    return a.size() < b.size();
                }
            )->c_str()
        ).x + _text_pad_pad_px;
    }

    inline const ImVec2 getInputPos(int i) const
    { 
        return ImVec2(
            _render_data.pos.x,
            _render_data.pos.y + _render_data.size.y * ((float)i + 1) / ((float)_input_count + 1)
        );
    }

    inline const ImVec2 getOutputPos(int i) const 
    { 
        return ImVec2(
            _render_data.pos.x + _render_data.size.x, 
            _render_data.pos.y + _render_data.size.y * ((float)i + 1) / ((float)_output_count + 1)
        ); 
    }

    inline virtual void render() = 0;
    inline virtual void update() {  }
};

