#include "soft3d/AudioData.hpp"
#include "soft3d/AudioManager.hpp"
#include "soft3d/Dialog.hpp"
#include "soft3d/Input.hpp"
#include "soft3d/SoundInstance.hpp"
#include "soft3d/TextureRes.hpp"
#include "soft3d/UiManager.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <limits>
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

void Append16(std::vector<uint8_t>& bytes, uint16_t value)
{
    bytes.push_back(static_cast<uint8_t>(value));
    bytes.push_back(static_cast<uint8_t>(value >> 8));
}

void Append32(std::vector<uint8_t>& bytes, uint32_t value)
{
    for (unsigned shift = 0; shift < 32; shift += 8) {
        bytes.push_back(static_cast<uint8_t>(value >> shift));
    }
}

void CheckRect(const soft3d::ScreenRect& rectangle,
               int32_t left, int32_t top, int32_t right, int32_t bottom)
{
    if (rectangle.left != left || rectangle.top != top ||
        rectangle.right != right || rectangle.bottom != bottom) {
        std::cerr << "rectangle expected (" << left << ',' << top << ',' << right << ',' << bottom
                  << "), got (" << rectangle.left << ',' << rectangle.top << ','
                  << rectangle.right << ',' << rectangle.bottom << ")\n";
    }
    CHECK(rectangle.left == left && rectangle.top == top &&
          rectangle.right == right && rectangle.bottom == bottom);
}

void TestAnyKeyQueries()
{
    using soft3d::Key;
    soft3d::Input input;
    CHECK(!input.IsAnyKeyFirstDown());
    CHECK(!input.IsAnyKeyDown());
    CHECK(!input.IsAnyKeyPress());
    CHECK(input.IsAnyKeyUp());
    CHECK(!input.IsAnyKeyClick());
    CHECK(!input.IsAnyKeyDownOrClick());

    for (std::size_t index = 1; index < soft3d::Input::keySlotCount; ++index) {
        const Key key = static_cast<Key>(index);
        soft3d::Input oneKey;
        oneKey.KeyDown(key);
        CHECK(oneKey.IsAnyKeyFirstDown());
        CHECK(oneKey.IsAnyKeyDown());
        CHECK(oneKey.IsAnyKeyDownOrClick());
        CHECK(oneKey.IsAnyKeyUp());
        oneKey.Update();
        CHECK(!oneKey.IsAnyKeyFirstDown());
        oneKey.KeyPress(key);
        CHECK(oneKey.IsAnyKeyPress());
        oneKey.KeyUp(key);
        CHECK(oneKey.IsAnyKeyClick());
        CHECK(oneKey.IsAnyKeyDownOrClick());
        CHECK(oneKey.IsAnyKeyUp());
        oneKey.Update();
        CHECK(!oneKey.IsAnyKeyClick());
        CHECK(!oneKey.IsAnyKeyDownOrClick());
    }

    soft3d::Input allDown;
    for (std::size_t index = 1; index < soft3d::Input::keySlotCount; ++index) {
        allDown.KeyDown(static_cast<Key>(index));
    }
    CHECK(!allDown.IsAnyKeyUp());
    allDown.KeyUp(Key::Accept);
    CHECK(allDown.IsAnyKeyUp());
    CHECK(allDown.IsAnyKeyClick());

    soft3d::Input reset;
    reset.KeyDown(Key::Left);
    reset.SetAnyKeyDown(0);
    CHECK(reset.GetKeyState(Key::Left) == 3);
    reset.SetAnyKeyDown(2);
    CHECK(reset.GetKeyState(Key::Left) == 0);
    reset.KeyDown(Key::Right, true);
    CHECK(reset.GetKeyState(Key::Right) == 0);
    reset.KeyDown(Key::SoftLeft, true);
    CHECK(reset.GetKeyState(Key::SoftLeft) == 3);
    reset.SetAnyKeyDown(2);
    CHECK(reset.GetKeyState(Key::SoftLeft) == 3);
    reset.SetAnyKeyDown(1);
    CHECK(reset.GetKeyState(Key::SoftLeft) == 0);
}

void TestSoundResetAndReuse()
{
    soft3d::AudioData data;
    soft3d::AudioManager manager(nullptr, nullptr);
    soft3d::SoundInstance instance;
    CHECK(instance.Initialize(&manager, &data, true, false) == 0);
    instance.SetPosition({3 << 16, 4 << 16, 0});
    instance.SetListenerPosition({});
    instance.CalculateSpatialVolume();
    CHECK(instance.GetDistance() == 5 << 16);
    CHECK(instance.GetVolume() == 255);
    CHECK(manager.AddSoundInstance(&instance) == 0);
    CHECK(instance.GetListIndex() == 0);
    manager.Update();
    CHECK(instance.GetData() == &data);
    CHECK(instance.IsActive() && instance.GetVoice() == 1);

    instance.Reset();
    CHECK(instance.GetData() == nullptr);
    CHECK(instance.GetVoice() == -1 && !instance.IsActive());
    CHECK(instance.GetListIndex() == -1);
    CHECK(instance.GetVolume() == 0 && instance.GetDistance() == 0);
    instance.CalculateSpatialVolume();
    CHECK(instance.GetDistance() == 5 << 16);
    CHECK(instance.GetVolume() == 255);
    manager.ClearSoundInstances();

    CHECK(instance.Initialize(&manager, &data, false, false) == 0);
    CHECK(manager.AddSoundInstance(&instance) == 0);
    manager.Update();
    CHECK(instance.IsActive());
    instance.Stop();
    CHECK(instance.GetVoice() == -1 && instance.GetListIndex() == -1);
    instance.Reset();
    manager.ClearSoundInstances();

    soft3d::SoundInstance* owned = new soft3d::SoundInstance();
    owned->Initialize(&manager, &data, false, true);
    owned->Reset();
    CHECK(manager.AddSoundInstance(owned) == 0);
    manager.ClearSoundInstances();
}

void TestDialogLinksAndTemplates()
{
    soft3d::Dialog first(nullptr, 0);
    soft3d::Dialog second(nullptr, 1);
    soft3d::Dialog third(nullptr, 2);
    CHECK(first.GetNextDialog() == &first && first.GetPreviousDialog() == &first);
    first.SetLinkedDialog(nullptr);
    CHECK(first.GetNextDialog() == &first && first.GetPreviousDialog() == &first);
    first.SetLinkedDialog(&second);
    CHECK(first.GetNextDialog() == &second && second.GetPreviousDialog() == &first);
    CHECK(first.GetPreviousDialog() == &first && second.GetNextDialog() == &second);
    first.SetLinkedDialog(&third);
    CHECK(first.GetNextDialog() == &third && third.GetPreviousDialog() == &first);
    CHECK(second.GetPreviousDialog() == &first);

    const soft3d::DialogElementTemplate* initial = first.GetElementTemplate(0);
    CHECK(initial != nullptr && initial->value0 == 0 && initial->id == -1);
    CHECK(initial->value2 == -1 && initial->value3 == -1 && initial->value4 == -1);
    CHECK(initial->value5 == -9999);
    for (int32_t index = 0; index < 20; ++index) {
        first.AddElement(100 + index, index, 200 + index,
                         300 + index, 400 + index, 500 + index);
        const soft3d::DialogElementTemplate* element = first.GetElementTemplate(index);
        CHECK(element != nullptr && element->value0 == 100 + index && element->id == index);
        CHECK(element->value2 == 200 + index && element->value3 == 300 + index);
        CHECK(element->value4 == 400 + index && element->value5 == 500 + index);
        if (index != 19) {
            CHECK(first.GetElementTemplate(index + 1)->id == -1);
        }
    }
    first.AddElement(1, 1, 2, 3, 4, 5);
    CHECK(first.GetElementTemplate(19)->value0 == 119);
    CHECK(first.GetElementTemplate(20) == nullptr);

    for (uint32_t mode = 0; mode <= 5; ++mode) {
        first.SetBlendMode(mode);
        CHECK(first.GetBlendMode() == mode);
    }
}

void CheckVisual(const soft3d::SdtVisual& actual, const soft3d::SdtVisual& expected)
{
    CHECK(actual.resourceIndex == expected.resourceIndex && actual.resource == expected.resource);
    CHECK(actual.x == expected.x && actual.y == expected.y);
    CHECK(actual.width == expected.width && actual.height == expected.height);
    CHECK(actual.blitMode == expected.blitMode);
    CHECK(actual.sourceX == expected.sourceX && actual.sourceY == expected.sourceY);
    CHECK(actual.sourceWidth == expected.sourceWidth && actual.sourceHeight == expected.sourceHeight);
    CHECK(actual.widthDivisor == expected.widthDivisor &&
          actual.heightDivisor == expected.heightDivisor);
    CHECK(actual.extra == expected.extra && actual.hidden == expected.hidden);
}

soft3d::SdtAnimationKey MakeKey(int32_t time, int32_t x, int32_t y)
{
    soft3d::SdtAnimationKey key;
    key.time = time;
    key.x = x;
    key.y = y;
    key.width = 1;
    key.height = 1;
    key.sourceWidth = 1;
    key.sourceHeight = 1;
    key.widthDivisor = 1;
    key.heightDivisor = 1;
    return key;
}

void TestDialogCurrentVisualAndStateKeys()
{
    soft3d::Dialog dialog(nullptr, 0);
    CHECK(dialog.GetCurrentVisual() == nullptr);
    soft3d::TextureRes resource;
    soft3d::SdtVisual source;
    source.resourceIndex = 7;
    source.resource = &resource;
    source.x = -3;
    source.y = 11;
    source.width = 20;
    source.height = 21;
    source.blitMode = 4;
    source.sourceX = 2;
    source.sourceY = 3;
    source.sourceWidth = 4;
    source.sourceHeight = 5;
    source.widthDivisor = 6;
    source.heightDivisor = 7;
    source.extra = 88;
    source.hidden = true;
    dialog.SetCurrentVisual(source);
    const soft3d::SdtVisual* current = dialog.GetCurrentVisual();
    CHECK(current != nullptr && current != &source);
    CheckVisual(*current, source);
    source.x = 99;
    CHECK(current->x == -3);
    source.x = 100;
    source.hidden = false;
    dialog.SetCurrentVisual(source);
    CHECK(dialog.GetCurrentVisual() == current);
    CheckVisual(*current, source);

    std::vector<uint8_t> bytes;
    Append32(bytes, 1U | (0x90U << 8) | (0xffffU << 16));
    Append32(bytes, 1);
    Append32(bytes, 1);
    Append16(bytes, 0xf800);
    Append16(bytes, 0);
    bytes.push_back(0);
    CHECK(resource.GetTexture()->ParseStx(bytes.data()) == soft3d::S3D_OK);
    soft3d::Texture target;
    CHECK(target.Resize(16, 24, 2) == soft3d::S3D_OK);
    std::fill_n(reinterpret_cast<uint32_t*>(target.GetPixels()), 16 * 24, 0);
    soft3d::ScreenBlitter blitter;
    blitter.SetTexture(&target);
    soft3d::UiManager manager;
    manager.SetScreenBlitter(&blitter);
    soft3d::Dialog rendered(&manager, 0);
    rendered.GetData().visuals.resize(2);
    rendered.GetData().visuals[0].resource = &resource;
    rendered.GetData().visuals[1].resource = &resource;
    rendered.GetData().states.resize(1);
    rendered.GetData().states[0].elements.resize(2);
    rendered.GetData().states[0].elements[0].visualIndex = 0;
    rendered.GetData().states[0].elements[1].visualIndex = 1;
    rendered.GetData().states[0].elements[0].keys = {MakeKey(0, 0, 0)};
    rendered.GetData().states[0].elements[1].keys = {
        MakeKey(0, 2, 3), MakeKey(10, 12, 23)
    };

    CHECK(rendered.GetElementX(0, 1, 5) == 7);
    CHECK(rendered.GetElementY(0, 1, 5) == 13);
    rendered.SetStateKeyFromElement(0, 0, 0, 1, 5);
    const soft3d::SdtAnimationKey* targetKey = rendered.GetKey(0, 0, 0);
    CHECK(targetKey != nullptr && targetKey->x == 7 && targetKey->y == 13 && targetKey->mode == 0);
    CHECK(rendered.GetVisual(0)->x == 7 && rendered.GetVisual(0)->y == 13);
    const uint32_t* pixels = reinterpret_cast<const uint32_t*>(target.GetPixels());
    CHECK(pixels[13 * 16 + 7] == 0x7ffff800U);

    rendered.InitializeRuntimeTimes();
    CHECK(!rendered.ApplyInterpolatedState(0, 0, 0, 1));
    CHECK(rendered.GetElementCurrentTime(0, 1) == 1);
    CHECK(targetKey->x == 2 && targetKey->y == 3);
    CHECK(pixels[3 * 16 + 2] == 0x7ffff800U);
    for (int32_t time = 1; time <= 10; ++time) {
        CHECK(rendered.ApplyInterpolatedState(0, 0, 0, 1) == (time == 10));
        CHECK(targetKey->x == 2 + time && targetKey->y == 3 + 2 * time);
    }
    CHECK(rendered.GetElementCurrentTime(0, 1) == 0);
    CHECK(targetKey->x == 12 && targetKey->y == 23);
    CHECK(pixels[23 * 16 + 12] == 0x7ffff800U);
    CHECK(!rendered.ApplyInterpolatedState(0, 0, 0, 1));
    CHECK(targetKey->x == 2 && targetKey->y == 3);
    rendered.SetElementCurrentTime(0, 1, 0x10000);
    CHECK(!rendered.ApplyInterpolatedState(0, 0, 0, 1));
    CHECK(rendered.GetElementCurrentTime(0, 1) == 1);
    CHECK(targetKey->x == 2 && targetKey->y == 3);
    CHECK(rendered.ApplyInterpolatedState(99, 0, 0, 1));

    rendered.SetKeyBlitMode(0, 0, 0, 4);
    CHECK(targetKey->mode == 4);
    CHECK(rendered.GetElementX(0, 1, -1) == -9999);
    CHECK(rendered.GetElementY(0, 1, 11) == -9999);
}

void TestPointsAndRectangles()
{
    soft3d::S3dPoint point;
    CHECK(point.x == 0 && point.y == 0);
    point.Set(-3, 7);
    CHECK(point.x == -3 && point.y == 7);

    soft3d::ScreenRect zero;
    CHECK(zero.IsZero() && zero.IsEmpty());
    CHECK(zero.Contains({0, 0}));
    soft3d::ScreenRect rectangle(-1, 2, 3, 8);
    CheckRect(rectangle, -1, 2, 3, 8);
    CHECK(!rectangle.IsZero() && !rectangle.IsEmpty());
    CHECK(rectangle.GetWidth() == 4 && rectangle.GetHeight() == 6);
    CHECK(rectangle.Contains({-1, 2}) && rectangle.Contains({3, 8}));
    CHECK(!rectangle.Contains({-2, 2}) && !rectangle.Contains({3, 9}));
    soft3d::ScreenRect fromSize(soft3d::S3dPoint{-2, 5}, soft3d::S3dPoint{4, 3});
    CheckRect(fromSize, -2, 5, 2, 8);
    soft3d::ScreenRect copied(fromSize);
    soft3d::ScreenRect assigned;
    assigned = rectangle;
    CHECK(copied == fromSize && assigned == rectangle);
    CHECK(copied != assigned);

    rectangle.AssignPosition({-8, 9});
    CheckRect(rectangle, -8, 9, 3, 8);
    soft3d::SdtAnimationKey key;
    key.x = 99;
    key.y = 6;
    key.width = -7;
    rectangle.AssignSdtPosition(key);
    CheckRect(rectangle, 6, -7, 3, 8);

    soft3d::ScreenRect moved(1, 2, 5, 8);
    moved.SetLeft(-5);
    CheckRect(moved, -5, 2, -1, 8);
    moved.SetTop(-4);
    CheckRect(moved, -5, -4, -1, 2);
    moved.SetPosition(7, 9);
    CheckRect(moved, 7, 9, 11, 15);
    moved.Offset(2, -3);
    CheckRect(moved, 9, 6, 13, 12);

    struct OffsetCase {
        int operation;
        soft3d::ScreenRect expected;
    };
    const std::array<OffsetCase, 5> cases = {{
        {0, {12, 23, 32, 43}},
        {1, {12, 23, 28, 37}},
        {2, {11, 22, 27, 36}},
        {3, {8, 17, 32, 43}},
        {4, {9, 18, 33, 44}},
    }};
    for (const OffsetCase& item : cases) {
        soft3d::ScreenRect value(10, 20, 30, 40);
        switch (item.operation) {
        case 0: value.Offset(2, 3); break;
        case 1: value.OffsetMirrored(2, 3); break;
        case 2: value.OffsetMirroredBoth(1, 2, 3, 4); break;
        case 3: value.OffsetReverse(2, 3); break;
        default: value.OffsetReverseBoth(1, 2, 3, 4); break;
        }
        CHECK(value == item.expected);
    }

    soft3d::ScreenRect united(10, 20, 30, 40);
    united.Include({0, 25, 35, 50});
    CheckRect(united, 0, 20, 35, 50);
    united.Include({1, 21, 34, 49});
    CheckRect(united, 0, 20, 35, 50);
    united.Clamp({5, 10, 30, 45});
    CheckRect(united, 5, 20, 30, 45);
    united.Clamp({40, 50, 60, 70});
    CheckRect(united, 40, 50, 30, 45);
    CHECK(united.IsEmpty());
    united.Set(1, 2, 3, 4);
    CheckRect(united, 1, 2, 3, 4);
    united.Zero();
    CHECK(united.IsZero());

    soft3d::ScreenRect wrapped(std::numeric_limits<int32_t>::max(), 0,
                               std::numeric_limits<int32_t>::min(), 1);
    CHECK(wrapped.GetWidth() == 1 && wrapped.GetHeight() == 1 && !wrapped.IsEmpty());
    wrapped.Offset(1, 0);
    CHECK(wrapped.left == std::numeric_limits<int32_t>::min());
    CHECK(wrapped.right == std::numeric_limits<int32_t>::min() + 1);
}

} // namespace

int main()
{
    try {
        TestAnyKeyQueries();
        TestSoundResetAndReuse();
        TestDialogLinksAndTemplates();
        TestDialogCurrentVisualAndStateKeys();
        TestPointsAndRectangles();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::cout << "stage-33 input, audio, dialog, and screen-geometry API tests passed\n";
    return 0;
}
