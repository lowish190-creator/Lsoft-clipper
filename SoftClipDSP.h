#pragma once
#include <cmath>

namespace sc
{
// Curva de clipping normalizada (el techo es 1.0).
// shape = 0  -> hard clip (recorte duro)
// shape = 1  -> saturacion suave tipo tanh desde cero
// Entre medias: zona lineal hasta la "rodilla" y luego curva suave (tanh)
// que se acerca a 1.0 sin pasarse. La curva es continua y derivable (C1).
inline float clip (float x, float shape) noexcept
{
    const float knee = 1.0f - shape;
    const float ax = std::abs (x);

    if (ax <= knee)
        return x;

    float y;
    if (knee >= 0.999f)
    {
        y = 1.0f;
    }
    else
    {
        const float w = 1.0f - knee;
        y = knee + w * std::tanh ((ax - knee) / w);
    }

    return std::copysign (y, x);
}
} // namespace sc
