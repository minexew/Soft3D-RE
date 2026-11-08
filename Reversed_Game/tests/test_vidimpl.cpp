#include "platform/dummy/DummyVideoBackend.hpp"
#include "soft3d/VID_impl.hpp"

#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, int line)
{
    if (!condition) {
        throw std::runtime_error("check failed at line " + std::to_string(line));
    }
}

#define CHECK(condition) Check((condition), __LINE__)

void TestLifecycleAndPresentation()
{
    auto backend = std::make_unique<soft3d::platform::DummyVideoBackend>();
    auto* dummy = backend.get();
    soft3d::VID_impl video(3, 2, std::move(backend));

    CHECK(!video.IsStarted());
    CHECK(dummy->IsStarted());
    CHECK(dummy->GetStartCount() == 1);
    CHECK(video.Start());
    CHECK(video.IsStarted());
    CHECK(dummy->GetStartCount() == 2);
    CHECK(video.Start());
    CHECK(dummy->GetStartCount() == 2);
    CHECK(std::string(video.GetBackendName()) == "dummy");

    const soft3d::VideoBufferInfo& info = video.GetBufferInfo();
    CHECK(info.width == 3);
    CHECK(info.height == 2);
    CHECK(info.pitch == 12);
    CHECK(info.pixelShift == 2);

    uint32_t* pixels = video.GetBackBufferPtr();
    CHECK(pixels != nullptr);
    CHECK(pixels == info.address);
    CHECK(dummy->GetBeginUpdateCount() == 1);
    const std::vector<uint32_t> expected = {
        0x01020304, 0x11121314, 0x21222324,
        0x31323334, 0x41424344, 0x51525354,
    };
    for (std::size_t i = 0; i < expected.size(); ++i) {
        pixels[i] = expected[i];
    }

    CHECK(video.GetBackBufferPtr() == pixels);
    CHECK(dummy->GetBeginUpdateCount() == 2);
    CHECK(video.Update());
    CHECK(dummy->GetUpdateCount() == 1);
    CHECK(dummy->GetPresentedPixels() == expected);

    video.Stop();
    CHECK(!video.IsStarted());
    CHECK(!dummy->IsStarted());
    CHECK(dummy->GetStopCount() == 1);
    video.Stop();
    CHECK(dummy->GetStopCount() == 1);

    CHECK(video.Start());
    CHECK(dummy->GetStartCount() == 3);
}

void TestBeginUpdateFailureKeepsCachedAddress()
{
    auto backend = std::make_unique<soft3d::platform::DummyVideoBackend>();
    auto* dummy = backend.get();
    dummy->SetBeginUpdateError(-17);
    soft3d::VID_impl video(2, 2, std::move(backend));

    uint32_t* pixels = video.GetBackBufferPtr();
    CHECK(pixels == video.GetBufferInfo().address);
    CHECK(dummy->GetBeginUpdateCount() == 1);
    CHECK(video.Update());
}

void TestBackendFailures()
{
    auto backend = std::make_unique<soft3d::platform::DummyVideoBackend>();
    auto* dummy = backend.get();
    dummy->SetStartResult(false);
    soft3d::VID_impl video(1, 1, std::move(backend));

    CHECK(!video.IsStarted());
    CHECK(dummy->GetStartCount() == 1);
    dummy->SetStartResult(true);
    CHECK(video.Start());
    CHECK(dummy->GetStartCount() == 2);

    dummy->SetUpdateResult(false);
    CHECK(!video.Update());
    CHECK(dummy->GetUpdateCount() == 1);
    CHECK(dummy->GetPresentedPixels().empty());
}

} // namespace

int main()
{
    try {
        TestLifecycleAndPresentation();
        TestBeginUpdateFailureKeepsCachedAddress();
        TestBackendFailures();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
