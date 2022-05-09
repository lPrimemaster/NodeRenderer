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