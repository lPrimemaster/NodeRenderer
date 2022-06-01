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
    constexpr Vector3(float x, float y, float z) : x(x), y(y), z(z) {  }
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

bool operator==(const Vector2& lhs, const Vector2& rhs);
bool operator==(const Vector3& lhs, const Vector3& rhs);
bool operator==(const Vector4& lhs, const Vector4& rhs);
bool operator!=(const Vector2& lhs, const Vector2& rhs);
bool operator!=(const Vector3& lhs, const Vector3& rhs);
bool operator!=(const Vector4& lhs, const Vector4& rhs);

Vector2 operator+(const Vector2& lhs, const Vector2& rhs);
Vector2 operator-(const Vector2& lhs, const Vector2& rhs);
Vector2 operator*(const Vector2& lhs, const Vector2& rhs);
Vector2 operator/(const Vector2& lhs, const Vector2& rhs);

Vector3 operator+(const Vector3& lhs, const Vector3& rhs);
Vector3 operator-(const Vector3& lhs, const Vector3& rhs);
Vector3 operator*(const Vector3& lhs, const Vector3& rhs);
Vector3 operator/(const Vector3& lhs, const Vector3& rhs);

Vector4 operator+(const Vector4& lhs, const Vector4& rhs);
Vector4 operator-(const Vector4& lhs, const Vector4& rhs);
Vector4 operator*(const Vector4& lhs, const Vector4& rhs);
Vector4 operator/(const Vector4& lhs, const Vector4& rhs);
