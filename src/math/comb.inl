#pragma once

namespace Math
{
    inline constexpr float Factorial(int n)
    {
        return n > 0 ? n * Factorial(n - 1) : 1;
    }

    inline constexpr float Binomial(int n, int i)
    {
        return Factorial(n) / (Factorial(i) * Factorial(n - i));
    }
}
