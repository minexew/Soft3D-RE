#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace soft3d {

class Renderable {
public:
    virtual ~Renderable() = default;
    virtual void Render() = 0;

    int32_t sortKey = 0;
};

class RenderQueues {
public:
    static constexpr std::size_t sceneQueueCount = 6;
    static constexpr std::size_t queueCount = 7;
    static constexpr std::size_t queueCapacity = 100;

    void Clear();
    bool Submit(Renderable* renderable, std::size_t queue);
    void SortSceneQueues(bool reverse);
    void RenderSceneQueues();
    void RenderOverlayQueue();

    std::size_t GetCount(std::size_t queue) const;

private:
    struct Queue {
        std::size_t count = 0;
        std::array<Renderable*, queueCapacity> entries{};
    };

    std::array<Queue, queueCount> queues;
};

} // namespace soft3d
