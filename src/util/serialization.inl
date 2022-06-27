#pragma once
#include <vector>

class ByteBuffer;

struct Serializable
{
    virtual ByteBuffer serialize() const = 0;
    virtual void deserialize(ByteBuffer& buffer) = 0;
};

class ByteBuffer
{
public:
    ByteBuffer() = default;
    ~ByteBuffer() = default;

private:
    using Byte = unsigned char;

    inline void addInternal(Byte value)
    {
        data.push_back(value);
    }

    inline void addInternal(Byte* value, size_t count)
    {
        int missing_size = (int)data.capacity() - (int)data.size() - (int)count;

        if(missing_size > 0) data.reserve(data.capacity() + missing_size);

        for(size_t i = 0; i < count; i++)
        {
            data.push_back(value[i]);
        }
    }
    
    inline Byte* dataptr()
    {
        return data.data() + read_offset;
    }

    inline void inc(size_t i)
    {
        read_offset += i;
    }

    inline void advanceToNextNullTerminator()
    {
        while(*(char*)dataptr() != '\0')
        {
            inc(1);
        }
        inc(1);
    }

    template<typename U>
    inline void add(const ByteBuffer& buffer)  requires std::is_same<U, ByteBuffer>::value
    {
        data.insert(data.end(), buffer.data.begin(), buffer.data.end());
    }

public:
    inline void addRawData(Byte* data, size_t size)
    {
        addInternal(data, size);
    }

    template<typename U>
    inline void add(const U& data)
    {
        Byte bytes[sizeof(U)];
        memcpy((void*)bytes, &data, sizeof(U));
        addInternal(bytes, sizeof(U));
    }

    template<typename U>
    inline void add(const U& buffer)  requires std::is_same<U, ByteBuffer>::value
    {
        data.insert(data.end(), buffer.data.begin(), buffer.data.end());
    }

    template<typename U>
    inline void add(const U& data) requires std::is_base_of<Serializable, U>::value
    {
        add(data.serialize());
    }

    template<typename U>
    inline void add(const U& string) requires std::is_same<U, std::string>::value
    {
        addInternal((Byte*)string.c_str(), string.size() + 1);
    }

    template<typename U>
    inline void add(const std::vector<U>& vec)
    {
        add(vec.size());

        size_t size = vec.size() * sizeof(U);
        Byte* bytes = new Byte[size];
        memcpy((void*)bytes, vec.data(), size);
        addInternal(bytes, size);
        delete[] bytes;
    }

    template<typename U>
    inline void add(const std::vector<U>& vec) requires std::is_base_of<Serializable, U>::value
    {
        add(vec.size());
        for(auto v : vec)
        {
            add(v.serialize());
        }
    }
    
    template<typename U>
    inline void add(const std::vector<U>& vec) requires std::is_same<U, std::string>::value
    {
        add(vec.size());
        for(auto s : vec)
        {
            addInternal((Byte*)s.c_str(), s.size() + 1);
        }
    }

    inline void getRawData(Byte* value, size_t size)
    {
        memcpy(value, dataptr(), size);
        inc(size);
    }

    template<typename U>
    inline void get(U* value)
    {
        *value = *(U*)dataptr();
        inc(sizeof(U));
    }

    template<typename U>
    inline void get(U* serializable) requires std::is_base_of<Serializable, U>::value
    {
        serializable->deserialize(*this);
    }
    
    template<typename U>
    inline void get(U* string) requires std::is_same<U, std::string>::value
    {
        const char* str = (const char*)dataptr();
        *string = std::string(str);
        advanceToNextNullTerminator();
    }

    template<typename U>
    inline void get(std::vector<U>* vec)
    {
        size_t count = *(size_t*)dataptr();
        inc(sizeof(size_t));
        vec->reserve(count);

        for(size_t i = 0; i < count; i++)
        {
            U value = *(U*)dataptr();
            inc(sizeof(U));
            vec->push_back(value);
        }
    }

    template<typename U>
    inline void get(std::vector<U>* vec) requires std::is_base_of<Serializable, U>::value
    {
        size_t count = *(size_t*)dataptr();
        inc(sizeof(size_t));
        vec->reserve(count);

        for(size_t i = 0; i < count; i++)
        {
            U value;
            value.deserialize(*this);
            vec->push_back(value);
        }
    }

    template<typename U>
    inline void get(std::vector<U>* vec) requires std::is_same<U, std::string>::value
    {
        size_t count = *(size_t*)dataptr();
        inc(sizeof(size_t));
        vec->reserve(count);

        for(size_t i = 0; i < count; i++)
        {
            std::string str;
            get(&str);
            vec->push_back(str);
        }
    }

    inline Byte* front()
    {
        return data.data();
    }

    inline size_t size() const
    {
        return data.size();
    }

    inline void clear()
    {
        data.clear();
    }

private:
    std::vector<Byte> data;
    size_t read_offset = 0;
};
