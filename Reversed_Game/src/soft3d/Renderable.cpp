#include "soft3d/Renderable.hpp"

#include <algorithm>

namespace soft3d {

void RenderQueues::Clear()
{
    for (Queue& queue : queues) {
        queue.count = 0;
    }
}

bool RenderQueues::Submit(Renderable* renderable, std::size_t queueIndex)
{
    if (queueIndex >= queueCount) {
        return false;
    }
    Queue& queue = queues[queueIndex];
    if (queue.count >= queueCapacity) {
        return false;
    }
    queue.entries[queue.count++] = renderable;
    return true;
}

void RenderQueues::SortSceneQueues(bool reverse)
{
    for (std::size_t index = 0; index < sceneQueueCount; ++index) {
        Queue& queue = queues[index];
        if (queue.count > 1) {
            if (reverse) {
                std::sort(queue.entries.begin(), queue.entries.begin() + queue.count,
                          [](const Renderable* left, const Renderable* right) {
                              return left->sortKey > right->sortKey;
                          });
            } else {
                std::sort(queue.entries.begin(), queue.entries.begin() + queue.count,
                          [](const Renderable* left, const Renderable* right) {
                              return left->sortKey < right->sortKey;
                          });
            }
        }
    }
}

void RenderQueues::RenderSceneQueues()
{
    for (std::size_t index = 0; index < sceneQueueCount; ++index) {
        Queue& queue = queues[index];
        for (std::size_t entry = 0; entry < queue.count; ++entry) {
            queue.entries[entry]->Render();
        }
    }
}

void RenderQueues::RenderOverlayQueue()
{
    Queue& queue = queues[sceneQueueCount];
    for (std::size_t entry = 0; entry < queue.count; ++entry) {
        queue.entries[entry]->Render();
    }
}

std::size_t RenderQueues::GetCount(std::size_t queueIndex) const
{
    return queueIndex < queueCount ? queues[queueIndex].count : 0;
}

} // namespace soft3d
