# Soft Clipper (VST3 / Standalone)

Plugin de clipping suave inspirado en el Soft Clipper de FL Studio, hecho con JUCE 8.

## Controles
- **Threshold** (-30 a 0 dB): nivel a partir del cual empieza a actuar el clipper y techo de salida.
- **Post gain** (-24 a +12 dB): ganancia de salida.
- **Shape** (0 a 1): 0 = hard clip, 1 = saturacion suave tipo tanh. Intermedio: zona lineal + rodilla suave.
- **Oversampling** (Off, 2x, 4x, 8x): reduce aliasing (anade latencia, que se reporta al host).
- **Bypass** y medidor de pico de salida.

## Compilar
Requisitos: CMake >= 3.22, compilador C++17 (Visual Studio 2022 en Windows, Xcode en macOS, GCC/Clang + libs de JUCE en Linux) y acceso a GitHub (JUCE se descarga solo).

```
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

En Windows el VST3 se copia a `C:\Program Files\Common Files\VST3` (ejecuta la terminal como administrador, o desactiva `COPY_PLUGIN_AFTER_BUILD`). Despues, en FL Studio: Options > Manage plugins > Find installed plugins.
