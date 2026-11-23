#pragma once

#include "soft3d/BasicShader.hpp"

#include <cstdint>
#include <vector>

namespace soft3d {

class Engine;
class Renderer;

class MotionBlurShader : public BasicShader {
public:
    MotionBlurShader(int width, int height);

    void Render(const void* source, uint32_t* destination) override;
    void ResetHistory();

private:
    std::vector<uint32_t> history;
    int resetHistory = 1;
};

class GrayShader : public BasicShader {
public:
    using BasicShader::BasicShader;

    void Render(const void* source, uint32_t* destination) override;
};

class FadeShader : public BasicShader {
public:
    FadeShader(int width, int height, Renderer* renderer = nullptr);

    void Render(const void* source, uint32_t* destination) override;
    void StartFade(int fadeOut, uint16_t color, int frameCount, int mode);
    bool IsCompleted() const;
    bool IsActive() const;
    void Complete();

private:
    Renderer* renderer;
    int frame = 0;
    int frameCount = 0;
    int active = 0;
    int completed = 0;
    int fadeOut = 0;
    uint16_t color = 0;
    int mode = 0;
};

class InkShader : public BasicShader {
public:
    using BasicShader::BasicShader;

    void Render(const void* source, uint32_t* destination) override;
    void SetEngine(Engine* engine);
    void Reset();
    void StartInk(Engine* engine, int reserved, uint32_t* initialHistory, int mode);
    void StopInk();
    bool IsCompleted() const;
    bool IsActive() const;

private:
    Engine* engine = nullptr;
    int active = 0;
    int completed = 0;
    int mode = 0;
    uint32_t* previousDestination = nullptr;
};

class BlindShader : public BasicShader {
public:
    using BasicShader::BasicShader;

    void Render(const void* source, uint32_t* destination) override;
};

class TwistShader : public BasicShader {
public:
    using BasicShader::BasicShader;

    void Render(const void* source, uint32_t* destination) override;
    void SetAmplitude(int value) { amplitude = value; }
    int GetAmplitude() const { return amplitude; }

private:
    int amplitude = 2;
    int offset = 0;
    int velocity = 1;
};

} // namespace soft3d
