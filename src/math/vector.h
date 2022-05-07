#pragma once

struct Vector2
{
    Vector2() { x = 0.0f; y = 0.0f; }
    Vector2(float x, float y) : x(x), y(y) {  }
    union
    {
        struct
        {
            float x;
            float y;
        };
        float data[2];
    };

};

struct Vector3
{
    Vector3() { x = 0.0f; y = 0.0f; z = 0.0f; }
    Vector3(float x, float y, float z) : x(x), y(y), z(z) {  }
    union
    {
        struct
        {
            float x;
            float y;
            float z;
        };
        float data[3];
    };
};

struct Vector4
{
    Vector4() { x = 0.0f; y = 0.0f; z = 0.0f; w = 0.0f; }
    Vector4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {  }
    union
    {
        struct
        {
            float x;
            float y;
            float z;
            float w;
        };
        float data[4];
    };
};
