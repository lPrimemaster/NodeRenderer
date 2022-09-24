#pragma once
#include <string>
#include <map>
#include <unordered_map>
#include <type_traits>
#include <typeinfo>
#include <typeindex>
#include <algorithm>
#include <iterator>
#include <vector>
#include "../../../imgui/imgui.h"
#include "../../log/logger.h"
#include "../../math/vector.h"
#include "../../util/serialization.inl"
#include "../node_outputs.h"

struct NodeRenderData : public Serializable
{
    ImVec2 pos;
    ImVec2 size;

    virtual ByteBuffer serialize() const override 
    {
        ByteBuffer out;

        out.add(pos.x);
        out.add(pos.y);

        out.add(size.x);
        out.add(size.y);

        return out;
    }

    virtual void deserialize(ByteBuffer& buffer) override 
    {
        float x, y, w, h;
        buffer.get(&x);
        buffer.get(&y);
        buffer.get(&w);
        buffer.get(&h);

        pos = ImVec2(x, y);
        size = ImVec2(w, h);
    }

};

struct IOIdxData
{
    unsigned char self_idx;
    unsigned char other_idx;
    int self_id;

    bool operator==(const IOIdxData& o) const
    {
        return self_idx == o.self_idx && other_idx == o.other_idx && self_id == o.self_id; // ?
    }

};

struct IOIdxDataHashFn
{
    size_t operator()(const IOIdxData& d) const
    {
        return std::hash<unsigned char>()(d.self_idx) ^ std::hash<unsigned char>()(d.other_idx) ^ std::hash<int>()(d.self_id);
    }
};


struct PropertyNode;
struct EmptyTypeDec {  };

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

        RENDER_DATA,
        MESH_NODE_DATA,
        MESH_INTERP_LIST_DATA,

        LIST_FLOAT = 100,
        LIST_INT,
        LIST_UINT,
        LIST_VECTOR2,
        LIST_VECTOR3,
        LIST_VECTOR4
    };

    // Create compile time map for friendly valid type names
    template<typename T> struct ValidTypeMap;
    #define VT_FRIENDLY_NAME(key, name) template <> struct ValidTypeMap<key> { static constexpr const char* value = name; }

    VT_FRIENDLY_NAME(EmptyTypeDec, "No Type");

    VT_FRIENDLY_NAME(float,        "Float");
    VT_FRIENDLY_NAME(int,          "Int");
    VT_FRIENDLY_NAME(unsigned int, "Uint");
    VT_FRIENDLY_NAME(Vector2,      "Vector2");
    VT_FRIENDLY_NAME(Vector3,      "Vector3");
    VT_FRIENDLY_NAME(Vector4,      "Vector4");
    
    VT_FRIENDLY_NAME(RenderNodeData,     "Render Data");
    VT_FRIENDLY_NAME(MeshNodeData,       "Mesh Data");
    VT_FRIENDLY_NAME(MeshInterpListData, "Mesh Data List");

    VT_FRIENDLY_NAME(std::vector<float>,        "Float List");
    VT_FRIENDLY_NAME(std::vector<int>,          "Int List");
    VT_FRIENDLY_NAME(std::vector<unsigned int>, "Uint List");
    VT_FRIENDLY_NAME(std::vector<Vector2>,      "Vector2 List");
    VT_FRIENDLY_NAME(std::vector<Vector3>,      "Vector3 List");
    VT_FRIENDLY_NAME(std::vector<Vector4>,      "Vector4 List");

    #undef VT_FRIENDLY_NAME

    template<typename T>
    PropertyGenericData(T value, PropertyNode* data_holder) : type(typeid(T)), _data_holder_instance(data_holder)
    {
        size = sizeof(T);
        data = new T(value);
        _data_changed = true;
        checkAssignTypeEnum();
    }

    template<typename T, size_t N>
    PropertyGenericData(const T (&arr)[N], PropertyNode* data_holder) : type(typeid(T*)), _data_holder_instance(data_holder)
    {
        size = N * sizeof(T);
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

                if constexpr(is_std_vector<T>::value)
                {
                    setSizeForVector<T::value_type>();
                }
            }
            else
            {
                L_ERROR("This PropertyGenericData stores an array. Attempted method call without a pointer type.");
            }
        }
        else
        {
            L_DEBUG("PropertyGenericData changed base datatype.");
            if(data != nullptr)
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

            value_type_name = ValidTypeMap<T>::value;
            is_fixed_array = false;
            data = new T(value);
            type = std::type_index(typeid(T));
            size = sizeof(T);
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
                size = N * sizeof(T);
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
            if(data != nullptr)
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

            value_type_name = ValidTypeMap<T>::value;
            size = N * sizeof(T);
            data = new T[N];
            memcpy(data, arr, N * sizeof(T));
            is_fixed_array = true;
            type = std::type_index(typeid(T*));
            _data_changed = true;
        }
    }

    inline void* getListData()
    {
        if(is_list)
        {
            switch (vtype)
            {
            case ValidType::LIST_FLOAT:   return (void*)((std::vector<float>*)data)->data();
            case ValidType::LIST_INT:     return (void*)((std::vector<int>*)data)->data();
            case ValidType::LIST_UINT:    return (void*)((std::vector<unsigned int>*)data)->data();
            case ValidType::LIST_VECTOR2: return (void*)((std::vector<Vector2>*)data)->data();
            case ValidType::LIST_VECTOR3: return (void*)((std::vector<Vector3>*)data)->data();
            case ValidType::LIST_VECTOR4: return (void*)((std::vector<Vector4>*)data)->data();
            default: L_ERROR("getListData(): Received a non valid type."); return nullptr;
            }
        }
        else
        {
            L_ERROR("This PropertyGenericData does not store a list. Ignoring getListData().");
            return nullptr;
        }
    }

    struct TypeDataBuffer
    {
        ValidType vtype;
        void* data;
    };

    inline void fromListData(const TypeDataBuffer& b)
    {
        switch (b.vtype)
        {
        case ValidType::LIST_FLOAT:
        {
            float* ptr = (float*)b.data;
            std::vector<float> vec(ptr, ptr + size / sizeof(float));
            setValue(vec);
        } break;
        case ValidType::LIST_INT:
        {
            int* ptr = (int*)b.data;
            std::vector<int> vec(ptr, ptr + size / sizeof(int));
            setValue(vec);
        } break;
        case ValidType::LIST_UINT:
        {
            unsigned int* ptr = (unsigned int*)b.data;
            std::vector<unsigned int> vec(ptr, ptr + size / sizeof(unsigned int));
            setValue(vec);
        } break;
        case ValidType::LIST_VECTOR2:
        {
            Vector2* ptr = (Vector2*)b.data;
            std::vector<Vector2> vec(ptr, ptr + size / sizeof(Vector2));
            setValue(vec);
        } break;
        case ValidType::LIST_VECTOR3:
        {
            Vector3* ptr = (Vector3*)b.data;
            std::vector<Vector3> vec(ptr, ptr + size / sizeof(Vector3));
            setValue(vec);
        } break;
        case ValidType::LIST_VECTOR4:
        {
            Vector4* ptr = (Vector4*)b.data;
            std::vector<Vector4> vec(ptr, ptr + size / sizeof(Vector4));
            setValue(vec);
        } break;
        default: L_ERROR("fromListData(): Received a non valid type."); break;
        }
    }

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
        // Simple values for node outputs
        case ValidType::FLOAT:   setValue<float>(*(float*)b.data); break;
        case ValidType::INT:     setValue<int>(*(int*)b.data); break;
        case ValidType::UINT:    setValue<unsigned int>(*(unsigned int*)b.data); break;
        case ValidType::VECTOR2: setValue<Vector2>(*(Vector2*)b.data); break;
        case ValidType::VECTOR3: setValue<Vector3>(*(Vector3*)b.data); break;
        case ValidType::VECTOR4: setValue<Vector4>(*(Vector4*)b.data); break;

        // List values for node outputs
        case ValidType::LIST_FLOAT:   setValue<std::vector<float>>(*(std::vector<float>*)b.data); break;
        case ValidType::LIST_INT:     setValue<std::vector<int>>(*(std::vector<int>*)b.data); break;
        case ValidType::LIST_UINT:    setValue<std::vector<unsigned int>>(*(std::vector<unsigned int>*)b.data); break;
        case ValidType::LIST_VECTOR2: setValue<std::vector<Vector2>>(*(std::vector<Vector2>*)b.data); break;
        case ValidType::LIST_VECTOR3: setValue<std::vector<Vector3>>(*(std::vector<Vector3>*)b.data); break;
        case ValidType::LIST_VECTOR4: setValue<std::vector<Vector4>>(*(std::vector<Vector4>*)b.data); break;

        // Custom values for node outputs
        case ValidType::RENDER_DATA:           setValue<RenderNodeData>(*(RenderNodeData*)b.data); break;
        case ValidType::MESH_NODE_DATA:        setValue<MeshNodeData>(MeshNodeData()); break; // Mesh node data is just loaded from disk
        case ValidType::MESH_INTERP_LIST_DATA: setValue<MeshInterpListData>(*(MeshInterpListData*)b.data); L_ERROR("MESH_INTERP_LIST_DATA deserialization not implemented."); exit(-1); break; // TODO

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
    bool is_list = false;
    bool _data_changed = false;
    PropertyNode* _data_holder_instance = nullptr;
    std::string value_type_name = "No Type";

private:
    // template<typename V>
    // inline std::vector<V> createVectorFromData(const TypeDataBuffer& b)
    // {

    // }

    // template<typename V>
    // inline TypeDataBuffer createDataFromVector(const std::vector<V>& vec)
    // {

    // }

    template<typename>
    struct is_std_vector : std::false_type {};

    template<typename T, typename A>
    struct is_std_vector<std::vector<T,A>> : std::true_type {};

    template<typename T>
    inline void setSizeForVector()
    {
        size = sizeof(T) * ((std::vector<T>*)data)->size();
    }

    inline void checkAssignTypeEnum()
    {
             if(isOfType<float>())        vtype = ValidType::FLOAT;
        else if(isOfType<int>())          vtype = ValidType::INT;
        else if(isOfType<unsigned int>()) vtype = ValidType::UINT;
        else if(isOfType<Vector2>())      vtype = ValidType::VECTOR2;
        else if(isOfType<Vector3>())      vtype = ValidType::VECTOR3;
        else if(isOfType<Vector4>())      vtype = ValidType::VECTOR4;
        else if(isOfType<std::vector<float>>())        { vtype = ValidType::LIST_FLOAT;   setSizeForVector<float>(); }
        else if(isOfType<std::vector<int>>())          { vtype = ValidType::LIST_INT;     setSizeForVector<int>(); }
        else if(isOfType<std::vector<unsigned int>>()) { vtype = ValidType::LIST_UINT;    setSizeForVector<unsigned int>(); }
        else if(isOfType<std::vector<Vector2>>())      { vtype = ValidType::LIST_VECTOR2; setSizeForVector<Vector2>(); }
        else if(isOfType<std::vector<Vector3>>())      { vtype = ValidType::LIST_VECTOR3; setSizeForVector<Vector3>(); }
        else if(isOfType<std::vector<Vector4>>())      { vtype = ValidType::LIST_VECTOR4; setSizeForVector<Vector4>(); }
        else if(isOfType<RenderNodeData>()) vtype = ValidType::RENDER_DATA;
        else if(isOfType<MeshNodeData>()) vtype = ValidType::MESH_NODE_DATA;
        else if(isOfType<MeshInterpListData>()) vtype = ValidType::MESH_INTERP_LIST_DATA;

        is_list = (static_cast<int>(vtype) >= 100);
    }
};

// TODO: Node and in/out types color
struct PropertyNode
{
    enum class Priority
    {
        NORMAL = 0,
        FEEDBACK = 1,
        RENDER = 2 // This is for deserialization only (for the first update)
    };

    enum class Type
    {
        VALUE,
        COLOR,
        MATH,
        FUNCTION,
        TIME,
        WORLDPOS,
        RENDER,
        LIST,
        LISTACCESS,
        LISTJOIN,
        MESH,
        PATH,
        CAMERA,
        AUDIO,
        DISPLAY,
        FEEDBACK,
        TEST,
        MESHINTERP,
        HISTOGRAM
    };

    using EmptyType = EmptyTypeDec;

    PropertyNode
    (
        Type type,
        int inputs_count = 0,
        std::vector<std::string> inputs_name = std::vector<std::string>(),
        int outputs_count = 1,
        std::vector<std::string> outputs_name = std::vector<std::string>()
    ) : type(type)
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
    Type type;

    // Node update priority
    Priority priority = Priority::NORMAL;
    
    // Inputs
    int _input_count = 0;
    std::vector<std::string> _input_labels;
    std::vector<std::string> _output_labels;
    
    float _input_max_pad_px = 0.0f;
    float _output_max_pad_px = 0.0f;
    inline static constexpr float _text_pad_pad_px = 20.0f;

    std::unordered_map<IOIdxData, PropertyGenericData*, IOIdxDataHashFn> inputs;
    std::map<std::string, PropertyGenericData*> inputs_named;

    // Mapped with _input_labels
    // Points to static declaration inside the disconnectInputIfNotOfType() function
    std::map<std::string, const std::vector<std::string>*> allowed_inputs_type_name;
    std::map<std::string, std::string> inputs_description;
    // TODO
    std::map<std::string, std::string> outputs_description;
    // std::map<std::string, const std::vector<std::string>*> outputs_type_name;
    
    // Outputs
    int _output_count = 0;
    std::vector<IOIdxData> output_dependencies;

    // This is the "main" output
    std::vector<PropertyGenericData*> outputs;
    std::map<std::string, PropertyGenericData*> outputs_named;

    // Display
    NodeRenderData _render_data;

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

    template<typename... Args>
    inline bool disconnectInputIfNotOfType(const std::string& inputName)
    {
        static const std::vector<std::string> local_inputs = {
            PropertyGenericData::ValidTypeMap<Args>::value...
        };
        allowed_inputs_type_name[inputName] = &local_inputs;

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

                L_WARNING("Node \"%s\" requires an input with types:", name.c_str());
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

    template<typename... Args>
    inline bool disconnectAllInputsIfNotOfType()
    {
        bool disconnected = false;
        if(!inputs_named.empty())
        {
            for(auto it = inputs_named.begin(); it != inputs_named.end(); it++)
            {
                disconnected = disconnectInputIfNotOfType<Args...>(it->first);
            }
        }
        return disconnected;
    }

    inline void setInputsOrdered(std::vector<std::string> in)
    {
        static const std::vector<std::string> const_any_type = { "Any Type" };
        _input_count = (int)in.size();
        _input_labels = in;

        for(auto k : in)
        {
            allowed_inputs_type_name[k] = &const_any_type;
        }

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
    inline virtual void onConnection(const std::string& inputName) {  }

    inline virtual ByteBuffer serialize() const
    {
        // Serialize all of the parent values for later
        ByteBuffer out;

        // int id;
        out.add(id);

        // std::string name;
        out.add(name);

        // Priority priority = Priority::NORMAL;
        out.add(priority);

        // int _input_count = 0;
        out.add(_input_count);

        // std::vector<std::string> _input_labels;
        out.add(_input_labels);

        // NOTE: This is fixed for now
        // std::vector<std::string> _output_labels;
        // out.add(_output_labels);
        
        // float _input_max_pad_px = 0.0f;
        out.add(_input_max_pad_px);

        // float _output_max_pad_px = 0.0f;
        out.add(_output_max_pad_px);

        // std::map<IOIdxData, PropertyGenericData*> inputs;
        // There is enough info to reconstruct this after deserialization off all nodes

        // std::map<std::string, PropertyGenericData*> inputs_named;
        // There is enough info to reconstruct this after deserialization off all nodes

        // NOTE: This is fixed for now
        // int _output_count = 0;
        // out.add(_output_count);

        // std::vector<IOIdxData> output_dependencies;
        out.add(output_dependencies);

        // Save the output data values
        // std::vector<PropertyGenericData*> outputs;
        for(auto o : outputs)
        {
            out.add(o->size);
            out.add(o->is_fixed_array);
            out.add(o->is_list);
            out.add(o->vtype);

            if(!o->is_list)
            {
                out.addRawData((unsigned char*)o->data, o->size);
            }
            else
            {
                out.addRawData((unsigned char*)o->getListData(), o->size);
            }
        }

        // std::map<std::string, PropertyGenericData*> outputs_named;
        // There is enough info to reconstruct this on deserialization
        
        // NodeRenderData _render_data;
        out.add(_render_data);
        
        return out;
    }

    inline virtual void deserialize(ByteBuffer& buffer)
    {
        // int id;
        buffer.get(&id);

        // std::string name;
        buffer.get(&name);

        // Priority priority = Priority::NORMAL;
        buffer.get(&priority);

        // int _input_count = 0;
        buffer.get(&_input_count);

        // std::vector<std::string> _input_labels;
        buffer.get(&_input_labels);

        // std::vector<std::string> _output_labels;
        // buffer.get(&_output_labels);
        
        // float _input_max_pad_px = 0.0f;
        buffer.get(&_input_max_pad_px);

        // float _output_max_pad_px = 0.0f;
        buffer.get(&_output_max_pad_px);

        // Remapped from IOIdxData -> output_dependencies
        // std::map<IOIdxData, PropertyGenericData*> inputs;
        // std::map<std::string, PropertyGenericData*> inputs_named;

        // int _output_count = 0;
        // buffer.get(&_output_count);

        // std::vector<IOIdxData> output_dependencies;
        buffer.get(&output_dependencies);

        // Save the output data values
        // std::vector<PropertyGenericData*> outputs;
        // Outputs should already have been initialized from the derived class' constructor
        unsigned char cpybuffer[10240];
        for(auto o : outputs)
        {
            buffer.get(&o->size);
            buffer.get(&o->is_fixed_array);
            buffer.get(&o->is_list);
            buffer.get(&o->vtype);

            PropertyGenericData::TypeDataBuffer dvalue;
            buffer.getRawData(cpybuffer, o->size);
            dvalue.data = (void*)cpybuffer;
            dvalue.vtype = o->vtype;

            if(!o->is_list)
            {
                o->setValueDynamic(dvalue);
            }
            else
            {
                o->fromListData(dvalue);
            }
        }

        // std::map<std::string, PropertyGenericData*> outputs_named;
        // This is already handled in the derived constructor when it calls setOutputsOrdered()
        
        // NodeRenderData _render_data;
        buffer.get(&_render_data);
    }
};

