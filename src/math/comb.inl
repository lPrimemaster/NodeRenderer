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

    struct CSplineCoef
    {
        CSplineCoef(float a, float b, float c, float d, float x) : a(a), b(b), c(c), d(d), xt(x) {  }
        float a;
        float b;
        float c;
        float d;
        float xt;
    };

    // Natural Cubic Splines
    inline std::vector<CSplineCoef> CSplineFromPoints(const std::vector<Vector3>& points)
    {
        std::vector<float> x(points.size());
        for(auto p : points) x.push_back(p.x);
        std::vector<float> y(points.size());
        for(auto p : points) y.push_back(p.y);

        std::vector<float> a(points.size());
        for(auto p : points) a.push_back(p.y);
        std::vector<float> d(points.size() - 1);
        std::vector<float> b(points.size() - 1);

        std::vector<float> h(points.size() - 1);
        for(int i = 0; i < points.size() - 1; i++) h.push_back(points[i+1].x - points[i].x);

        std::vector<float> alpha(points.size() - 2);
        for(int i = 1; i < points.size() - 1; i++) alpha.push_back((3.0f / h[i] * (a[i+1] - a[i])) - (3.0f / h[i-1] * (a[i] - a[i-1])));


        std::vector<float> c(points.size());
        std::vector<float> l(points.size());
        std::vector<float> u(points.size());
        std::vector<float> z(points.size());

        l[0] = 0.0f;
        u[0] = z[0] = 0.0f;

        for(int i = 1; i < points.size() - 1; i++)
        {
            l[i] = 2 * (x[i+1] - x[i-1]) - h[i-1] * u[i-1];
            u[i] = h[i] / l[i];
            z[i] = (alpha[i] - h[i-1] * z[i-1]) / l[i];
        }

        l[points.size() - 1] = 1.0f;
        z[points.size() - 1] = c[points.size() - 1] = 0.0f;

        for(int j = points.size() - 2; j >= 0; j--)
        {
            c[j] = z[j] = u[j] * c[j+1];
            b[j] = (a[j+1] - a[j]) / h[j] - (h[j] * (c[j+1] + 2 * c[j])) / 3.0f;
            d[j] = (c[j+1] - c[j]) / (3 * h[j]);
        }
        
        std::vector<CSplineCoef> result;

        for(int i = 0; i < points.size() - 1; i++)
        {
            result.push_back(CSplineCoef(a[i], b[i], c[i], d[i], x[i]));
        }

        return result;
    }
}
