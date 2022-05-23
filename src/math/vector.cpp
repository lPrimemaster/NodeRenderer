#include "vector.h"

bool operator==(const Vector2& lhs, const Vector2& rhs)
{
    return lhs.x == rhs.x && lhs.y == rhs.y;
}

bool operator==(const Vector3& lhs, const Vector3& rhs)
{
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

bool operator==(const Vector4& lhs, const Vector4& rhs)
{
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z && lhs.w == rhs.w;
}

bool operator!=(const Vector2& lhs, const Vector2& rhs)
{
    return !operator==(lhs, rhs);
}

bool operator!=(const Vector3& lhs, const Vector3& rhs)
{
    return !operator==(lhs, rhs);
}

bool operator!=(const Vector4& lhs, const Vector4& rhs)
{
    return !operator==(lhs, rhs);
}

Vector2 operator+(const Vector2& lhs, const Vector2& rhs) { return Vector2(lhs.x + rhs.x, lhs.y + rhs.y); }
Vector2 operator-(const Vector2& lhs, const Vector2& rhs) { return Vector2(lhs.x - rhs.x, lhs.y - rhs.y); }
Vector2 operator*(const Vector2& lhs, const Vector2& rhs) { return Vector2(lhs.x * rhs.x, lhs.y * rhs.y); }
Vector2 operator/(const Vector2& lhs, const Vector2& rhs) { return Vector2(lhs.x / rhs.x, lhs.y / rhs.y); }

Vector3 operator+(const Vector3& lhs, const Vector3& rhs) { return Vector3(lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z); }
Vector3 operator-(const Vector3& lhs, const Vector3& rhs) { return Vector3(lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z); }
Vector3 operator*(const Vector3& lhs, const Vector3& rhs) { return Vector3(lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z); }
Vector3 operator/(const Vector3& lhs, const Vector3& rhs) { return Vector3(lhs.x / rhs.x, lhs.y / rhs.y, lhs.z / rhs.z); }

Vector4 operator+(const Vector4& lhs, const Vector4& rhs) { return Vector4(lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z, lhs.w + rhs.w); }
Vector4 operator-(const Vector4& lhs, const Vector4& rhs) { return Vector4(lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z, lhs.w - rhs.w); }
Vector4 operator*(const Vector4& lhs, const Vector4& rhs) { return Vector4(lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z, lhs.w * rhs.w); }
Vector4 operator/(const Vector4& lhs, const Vector4& rhs) { return Vector4(lhs.x / rhs.x, lhs.y / rhs.y, lhs.z / rhs.z, lhs.w / rhs.w); }