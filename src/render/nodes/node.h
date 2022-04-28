#pragma once
#include <string>
#include <map>
#include <type_traits>
#include <typeinfo>
#include <typeindex>
#include <algorithm>
#include <iterator>
#include "../../../imgui/imgui.h"
#include "../../log/logger.h"

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

struct PropertyGenericData
{
    template<typename T>
    PropertyGenericData(T value) : type(typeid(T))
    {
        data = new T(value);
    }

    template<typename T, size_t N>
    PropertyGenericData(const T (&arr)[N]) : type(typeid(T*))
    {
        size = N;
        data = new T[N];
        memcpy(data, arr, N * sizeof(T));
        is_fixed_array = true;
    }

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
        return std::type_index(typeid(T)) == type || std::type_index(typeid(U)) == type || isOfType<Args...>();
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
            // TODO : Handle this return somehow
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

    template<typename T>
    inline void setValue(T value)
    {
        if(isOfType<T>())
        {
            if(!is_fixed_array)
            {
                (*(T*)data) = value;
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
        }
    }


    std::type_index type;
    void* data = nullptr;
    size_t size = 0ULL;
    bool is_fixed_array = false;
};

// TODO: Node and in/out types color
struct PropertyNode
{
    struct EmptyType {  };

    PropertyNode() : data(PropertyGenericData(EmptyType())) {  }

    template<typename T>
    PropertyNode(T value) : data(PropertyGenericData(value)) {  }

    template<typename T, size_t N>
    PropertyNode(const T (&arr)[N]) : data(PropertyGenericData(arr)) {  }
    
    // Identification
    int id;
    std::string name;
    
    // Inputs
    int _input_count = 0;
    std::vector<std::string> _input_labels;
    float _input_max_pad_px = 0.0f;
    inline static constexpr float _text_pad_pad_px = 20.0f;
    std::map<IOIdxData, PropertyNode*> inputs;
    
    // Outputs
    int _output_count = 0;
    std::vector<IOIdxData> output_dependencies;

    PropertyGenericData data;

    // Display
    NodeRenderData _render_data;

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

    inline virtual void render()
    {
        ImGui::Text("A sample node");
    }

    inline virtual ~PropertyNode() = 0 {  }
};