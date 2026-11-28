#include "soft3d/Bsp.hpp"
#include "soft3d/StateMachine.hpp"
#include "soft3d/Units.hpp"
#include "soft3d/s3dResManager.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
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

void AppendFloat(std::vector<uint8_t>& bytes, float value)
{
    uint32_t bits;
    std::memcpy(&bits, &value, sizeof(bits));
    Append32(bytes, bits);
}

void Write32(std::vector<uint8_t>& bytes, std::size_t offset, uint32_t value)
{
    for (unsigned shift = 0; shift < 32; shift += 8) {
        bytes.at(offset + shift / 8) = static_cast<uint8_t>(value >> shift);
    }
}

using Chunks = std::array<std::vector<uint8_t>, soft3d::sbpChunkCount>;

std::vector<uint8_t> MakeSbp(const Chunks& chunks)
{
    std::vector<uint8_t> bytes(soft3d::sbpHeaderBytes, 0);
    std::copy_n("S3D", 4, bytes.begin());
    const float version = 7.6f;
    std::memcpy(bytes.data() + 4, &version, 4);
    uint32_t offset = soft3d::sbpHeaderBytes;
    for (std::size_t index = 0; index < chunks.size(); ++index) {
        Write32(bytes, 0x20 + index * 8, offset);
        Write32(bytes, 0x24 + index * 8, static_cast<uint32_t>(chunks[index].size()));
        bytes.insert(bytes.end(), chunks[index].begin(), chunks[index].end());
        offset += static_cast<uint32_t>(chunks[index].size());
    }
    return bytes;
}

std::vector<uint8_t> MakeArchive(const std::string& entryName,
                                 const std::vector<uint8_t>& data)
{
    std::vector<uint8_t> bytes;
    Append16(bytes, 1);
    std::array<char, soft3d::pakPathBytes> name{};
    std::copy(entryName.begin(), entryName.end(), name.begin());
    bytes.insert(bytes.end(), name.begin(), name.end());
    Append32(bytes, static_cast<uint32_t>(2 + soft3d::pakEntryBytes));
    bytes.insert(bytes.end(), data.begin(), data.end());
    return bytes;
}

class TempArchive {
public:
    TempArchive(const std::string& entryName, const std::vector<uint8_t>& data,
                const char* fileName = "soft3d-unit-tests.spk")
        : path(std::filesystem::temp_directory_path() / fileName)
    {
        Reset(entryName, data);
    }

    void Reset(const std::string& entryName, const std::vector<uint8_t>& data)
    {
        const std::vector<uint8_t> bytes = MakeArchive(entryName, data);
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        output.write(reinterpret_cast<const char*>(bytes.data()),
                     static_cast<std::streamsize>(bytes.size()));
        output.close();
        CHECK(output.good());
    }

    ~TempArchive()
    {
        std::error_code error;
        std::filesystem::remove(path, error);
    }

    std::filesystem::path path;
};

std::vector<uint8_t> Plane(float x, float y, float z, float distance, uint32_t type = 0)
{
    std::vector<uint8_t> bytes;
    AppendFloat(bytes, x);
    AppendFloat(bytes, y);
    AppendFloat(bytes, z);
    AppendFloat(bytes, distance);
    Append32(bytes, type);
    return bytes;
}

void AppendPlane(std::vector<uint8_t>& bytes, float x, float y, float z,
                 float distance, uint32_t type = 0)
{
    const std::vector<uint8_t> plane = Plane(x, y, z, distance, type);
    bytes.insert(bytes.end(), plane.begin(), plane.end());
}

std::vector<uint8_t> PathNode(float x, float y, float z, uint16_t firstPair, int16_t count)
{
    std::vector<uint8_t> bytes;
    AppendFloat(bytes, x);
    AppendFloat(bytes, y);
    AppendFloat(bytes, z);
    Append16(bytes, firstPair);
    Append16(bytes, static_cast<uint16_t>(count));
    return bytes;
}

Chunks CollisionChunks()
{
    Chunks chunks;
    AppendPlane(chunks[soft3d::SbpPlanes], 1, 0, 0, -1, 0);
    AppendPlane(chunks[soft3d::SbpPlanes], -1, 0, 0, -1, 0);
    AppendPlane(chunks[soft3d::SbpPlanes], 0, 1, 0, -1, 1);
    AppendPlane(chunks[soft3d::SbpPlanes], 0, -1, 0, -1, 1);
    AppendPlane(chunks[soft3d::SbpPlanes], 0, 0, 1, -1, 2);
    AppendPlane(chunks[soft3d::SbpPlanes], 0, 0, -1, -1, 2);
    AppendPlane(chunks[soft3d::SbpPlanes], 1, 0, 0, 0, 0);
    chunks[soft3d::SbpBrushes] = {0, 0, 6, 0};
    for (uint32_t index = 0; index < 6; ++index) {
        Append32(chunks[soft3d::SbpBrushSides], ((index + 10) << 16) | index);
    }
    chunks[soft3d::SbpLeafBrushes] = {0, 0, 0, 0};
    chunks[soft3d::SbpLeaves].resize(32, 0);
    for (unsigned leaf = 0; leaf < 2; ++leaf) {
        Append16(chunks[soft3d::SbpLeaves], 0xfffe);
        Append16(chunks[soft3d::SbpLeaves], 0xfffe);
        Append16(chunks[soft3d::SbpLeaves], 0xfffe);
        Append16(chunks[soft3d::SbpLeaves], 2);
        Append16(chunks[soft3d::SbpLeaves], 2);
        Append16(chunks[soft3d::SbpLeaves], 2);
        Append16(chunks[soft3d::SbpLeaves], static_cast<uint16_t>(leaf));
        Append16(chunks[soft3d::SbpLeaves], leaf == 0 ? 1 : 0);
        Append16(chunks[soft3d::SbpLeaves], 0);
        Append16(chunks[soft3d::SbpLeaves], 0);
        Append16(chunks[soft3d::SbpLeaves], 0);
        Append16(chunks[soft3d::SbpLeaves], 0);
        Append16(chunks[soft3d::SbpLeaves], 0);
        Append16(chunks[soft3d::SbpLeaves], leaf == 0 ? 1 : 0);
        Append16(chunks[soft3d::SbpLeaves], 0);
        Append16(chunks[soft3d::SbpLeaves], 0);
    }
    Append32(chunks[soft3d::SbpNodes], 6);
    Append16(chunks[soft3d::SbpNodes], 0xffff);
    Append16(chunks[soft3d::SbpNodes], 0xfffe);
    for (unsigned index = 0; index < 6; ++index) {
        Append16(chunks[soft3d::SbpNodes], 0);
    }
    Append16(chunks[soft3d::SbpNodes], 0);
    Append16(chunks[soft3d::SbpNodes], 0);
    chunks[soft3d::SbpPathNodes] = PathNode(-3, 0, 0, 0, 0);
    return chunks;
}

std::unique_ptr<soft3d::Bsp> LoadSynthetic(TempArchive& archive,
                                           soft3d::s3dResManager& manager,
                                           soft3d::Renderer& renderer)
{
    archive.Reset(".\\common\\units.sbp", MakeSbp(CollisionChunks()));
    CHECK(soft3d::OpenGameResourceArchive(&manager, archive.path.u16string().c_str()) ==
          soft3d::S3D_OK);
    auto bsp = std::make_unique<soft3d::Bsp>(&renderer, &manager);
    CHECK(soft3d::LoadSbpMapChunks(bsp.get(), u".\\common\\", u"units.sbp", nullptr) ==
          soft3d::S3D_OK);
    return bsp;
}

class ReceiverUnit : public soft3d::GameUnit {
public:
    soft3d::MsgReceiver* GetMsgReceiver() override { return &receiver; }
    void ProcessMessages() override
    {
        for (std::size_t index = 0; index < receiver.GetMessageCount(); ++index) {
            total += receiver.GetMessage(index).value;
        }
        receiver.ClearMessages();
        ++processCount;
    }

    soft3d::MsgReceiver receiver;
    uint32_t total = 0;
    int processCount = 0;
};

void TestMessagingAndGeometry()
{
    ReceiverUnit unit;
    CHECK(unit.GetUnitType() == 0 && unit.GetUnitId() == -1);
    CHECK(unit.IsVisible() && unit.IsCollisionEnabled());
    CHECK(unit.GetMsgReceiver() == &unit.receiver);
    soft3d::SendUnitMessage(&unit, 7, 11, 22);
    soft3d::SendUnitMessage(&unit, 8, 13, 23);
    CHECK(unit.receiver.GetMessageCount() == 2);
    CHECK(unit.receiver.GetMessage(1).type == 8);
    CHECK(unit.receiver.GetMessage(1).value == 13);
    CHECK(unit.receiver.GetMessage(1).data == 23);
    unit.ProcessMessages();
    CHECK(unit.total == 24 && unit.processCount == 1);
    CHECK(unit.receiver.GetMessageCount() == 0);
    for (unsigned index = 0; index < 21; ++index) {
        unit.receiver.AddMessage(index, index, index);
    }
    CHECK(unit.receiver.GetMessageCount() == soft3d::MsgReceiver::capacity);

    soft3d::ObjectBounds bounds{{-0x10000, -0x20000, -0x30000},
                                {0x10000, 0x20000, 0x30000}};
    unit.SetCollisionBounds(bounds);
    CHECK(unit.QueryPoint({0, 0, 0}) == &unit);
    CHECK(unit.QueryPoint({0x10001, 0, 0}) == nullptr);
    CHECK(unit.QuerySegment({-0x20000, 0, 0}, {0x20000, 0, 0}) == &unit);
    CHECK(unit.QuerySegment({-0x20000, 0x30000, 0}, {0x20000, 0x30000, 0}) == nullptr);
    CHECK(unit.QueryBounds({{0x8000, 0, 0}, {0x18000, 0x10000, 0x10000}}) == &unit);
    CHECK(unit.QueryBounds({{0x10001, 0, 0x30001}, {0x18000, 1, 0x38000}}) == nullptr);
    soft3d::ObjectBounds moved = soft3d::Translate(bounds, {0x10000, 0x20000, 0x30000});
    CHECK(moved.minimum.x == 0 && moved.maximum.z == 0x60000);
    CHECK(soft3d::Center(bounds).x == 0 && soft3d::Center(bounds).y == 0);
    const soft3d::ObjectBounds ordered = soft3d::Bounds({2, 3, 4}, {-2, 5, -4});
    CHECK(ordered.minimum.x == -2 && ordered.minimum.y == 3 && ordered.minimum.z == -4);
    CHECK(ordered.maximum.x == 2 && ordered.maximum.y == 5 && ordered.maximum.z == 4);
    soft3d::ResetBounds(moved);
    CHECK(moved.minimum.x == INT32_MAX && moved.maximum.x == INT32_MIN);
    soft3d::Expand(moved, {-2, 3, -4});
    soft3d::Expand(moved, {2, 5, 4});
    CHECK(moved.minimum.x == -2 && moved.minimum.y == 3 && moved.minimum.z == -4);
    CHECK(moved.maximum.x == 2 && moved.maximum.y == 5 && moved.maximum.z == 4);
    unit.SetCollisionEnabled(false);
    CHECK(unit.QueryPoint({0, 0, 0}) == nullptr);
    soft3d::GameUnit base;
    CHECK(base.GetMsgReceiver() == nullptr);
}

struct ManagedUnit : soft3d::GameUnit {
    ManagedUnit(int id, int* destructions = nullptr) : destructions(destructions) { unitId = id; }
    ~ManagedUnit() override
    {
        if (destructions != nullptr) {
            ++*destructions;
        }
    }
    void Update() override { ++updates; }
    void PostUpdate() override { ++postUpdates; }
    void SubmitRenderables(soft3d::RenderQueues&) override { ++submits; }

    int updates = 0;
    int postUpdates = 0;
    int submits = 0;
    int* destructions;
};

void TestManager()
{
    soft3d::GameUnitManager manager;
    ManagedUnit first(10);
    ManagedUnit second(20);
    ManagedUnit third(30);
    first.SetCollisionBounds({{-0x10000, -0x10000, -0x10000},
                              {0x10000, 0x10000, 0x10000}});
    second.SetCollisionBounds(first.GetCollisionBounds());
    third.SetCollisionBounds(first.GetCollisionBounds());
    manager.AddUnit(&first, 0);
    manager.AddUnit(&second, 2);
    manager.AddUnit(&third, 4);
    CHECK(manager.GetBucketCount(0) == 1 && manager.GetBucketCount(4) == 1);
    CHECK(manager.FindUnit(2, 20) == &second);
    CHECK(manager.FindUnit(2, 30) == nullptr);
    manager.UpdateUnits();
    manager.PostUpdateUnits();
    soft3d::RenderQueues queues;
    manager.SubmitUnits(queues);
    CHECK(first.updates == 1 && second.updates == 1 && third.updates == 1);
    CHECK(first.postUpdates == 1 && second.postUpdates == 1 && third.postUpdates == 1);
    CHECK(first.submits == 1 && second.submits == 1 && third.submits == 1);

    CHECK(manager.QueryPoint({0, 0, 0}).count == 2);
    CHECK(manager.QueryPoint({0, 0, 0}, 4).count == 1);
    first.SetVisible(false);
    CHECK(manager.QueryPoint({0, 0, 0}).count == 1);
    CHECK(manager.QueryPoint({0, 0, 0}, 0).count == 1);
    CHECK(manager.QueryBounds({{-1, -1, -1}, {1, 1, 1}}).count == 1);
    CHECK(manager.QueryBounds({{-1, -1, -1}, {1, 1, 1}}, 0).count == 1);
    first.SetVisible(true);
    CHECK(manager.QuerySegment({-0x20000, 0, 0}, {0x20000, 0, 0}).count == 2);
    CHECK(manager.QueryBounds({{-1, -1, -1}, {1, 1, 1}}).count == 2);
    CHECK(manager.QueryBounds({{-1, -1, -1}, {1, 1, 1}}, -1, &first).count == 1);
    second.SetCollisionEnabled(false);
    CHECK(manager.QueryPoint({0, 0, 0}).count == 1);

    int destructions = 0;
    manager.AddUnit(new ManagedUnit(40, &destructions), 0);
    manager.RemoveUnitById(40, 0, true);
    CHECK(destructions == 1 && manager.FindUnit(0, 40) == nullptr);
    manager.AddUnit(new ManagedUnit(50, &destructions), 1);
    manager.ClearBucket(1, true);
    CHECK(destructions == 2 && manager.GetBucketCount(1) == 0);
    manager.RemoveUnit(&third, 4, false);
    CHECK(manager.GetBucketCount(4) == 0);
    manager.ResetBuckets();
    CHECK(manager.GetBucketCount(0) == 0 && manager.GetBucketCount(2) == 0);
}

struct StateOwner {
    std::vector<std::string> events;
};

struct TestState final : soft3d::State<StateOwner> {
    TestState(std::string name, soft3d::State<StateOwner>* transition = nullptr)
        : name(std::move(name)), transition(transition) {}
    void Enter(StateOwner* owner) override { owner->events.push_back("enter:" + name); }
    void Exit(StateOwner* owner) override { owner->events.push_back("exit:" + name); }
    soft3d::State<StateOwner>* Update(StateOwner* owner) override
    {
        owner->events.push_back("update:" + name);
        return transition;
    }

    std::string name;
    soft3d::State<StateOwner>* transition;
};

void TestStateMachine()
{
    StateOwner owner;
    soft3d::StateSet<StateOwner> states;
    TestState next("next");
    TestState child("child", &next);
    TestState root("root");
    root.SetNext(&child);
    root.Register(&states, 0);
    child.Register(&states, 1);
    next.Register(&states, 2);
    CHECK(root.IsEnabled() && child.IsEnabled() && next.IsEnabled());
    CHECK(states.GetRegisteredState(1) == &child);
    CHECK(states.GetRegisteredState(9) == nullptr);

    soft3d::StateMachine<StateOwner> machine(&states);
    machine.Reset(&owner);
    CHECK(machine.GetCurrentState() == &root);
    CHECK(owner.events == std::vector<std::string>({"enter:child", "enter:root"}));
    machine.Update(&owner);
    CHECK(machine.GetCurrentState() == &next);
    CHECK(owner.events == std::vector<std::string>({
        "enter:child", "enter:root", "update:child", "exit:root", "exit:child", "enter:next"}));
    machine.Exit(&owner);
    CHECK(owner.events.back() == "exit:next");

    auto pending = std::make_unique<TestState>("pending");
    machine.SetPendingState(std::move(pending));
    CHECK(machine.GetPendingState() != nullptr);
    machine.SetPendingState(nullptr);
    CHECK(machine.GetPendingState() == nullptr);
}

void TestBspCollision()
{
    TempArchive archive(".\\common\\unused", {});
    soft3d::s3dResManager manager;
    soft3d::Renderer renderer(32, 32);
    std::unique_ptr<soft3d::Bsp> bsp = LoadSynthetic(archive, manager, renderer);
    CHECK(bsp->FindLeaf({0x8000, 0, 0}) == 1);
    CHECK(bsp->FindLeaf({-0x8000, 0, 0}) == 2);
    CHECK(bsp->GetLightIntensity({0, 0, 0}) == soft3d::fixedOne);

    const soft3d::BspTraceResult hit = bsp->Trace({-0x20000, 0, 0}, {0x20000, 0, 0});
    CHECK(hit.fraction > 0x3000 && hit.fraction < 0x5000);
    CHECK(hit.planeNormal.x == -soft3d::fixedOne);
    CHECK(hit.sideIndex == 11);
    CHECK(hit.endPosition.x < -0x10000 && hit.endPosition.x > -0x11000);
    CHECK(hit.startSolid && !hit.allSolid);

    const soft3d::BspTraceResult solid = bsp->Trace({0, 0, 0}, {0x20000, 0, 0});
    CHECK(solid.brushStartSolid && !solid.allSolid);
    const soft3d::BspTraceResult point = bsp->Trace({0, 0, 0}, {0, 0, 0});
    CHECK(point.startSolid && point.brushStartSolid && point.allSolid);
    const soft3d::BspTraceResult empty = bsp->Trace({-0x18000, 0, 0}, {-0x18000, 0, 0});
    CHECK(!empty.startSolid);
    const soft3d::ObjectBounds offsetBounds{{0x20000, 0, 0}, {0x20000, 0, 0}};
    CHECK(bsp->Trace({}, {}, offsetBounds).startSolid);

    const soft3d::BspPathNode* node = bsp->FindNearestPathNode({-0x30000, 0, 0}, {});
    CHECK(node != nullptr && node->position.x == -0x30000);
    CHECK(bsp->GetPathNode(0) == node);
    CHECK(bsp->GetPathNode(0xffff) == nullptr);
}

void TestPhysics()
{
    TempArchive archive(".\\common\\unused", {});
    soft3d::s3dResManager resources;
    soft3d::Renderer renderer(32, 32);
    std::unique_ptr<soft3d::Bsp> bsp = LoadSynthetic(archive, resources, renderer);

    soft3d::PhysicUnit unit(nullptr, 3, nullptr, nullptr, bsp.get());
    unit.SetPosition({-0x20000, 0, 0});
    unit.SetVelocity({0x40000, 0, 0});
    unit.MoveSimple();
    CHECK(unit.GetPosition().x < -0x10000 && unit.GetPosition().x > -0x11000);

    unit.SetPosition({-0x20000, 0, 0});
    unit.SetVelocity({0x40000, 0, 0});
    unit.MoveSlide();
    CHECK(unit.GetPosition().x < -0x10000 && unit.GetPosition().x > -0x11000);
    CHECK(unit.GetVelocity().x <= 0);

    unit.SetPhysicsFlags(soft3d::PhysicSlideMovement);
    unit.SetLocalBounds({{-0x1000, -0x1000, -0x1000}, {0x1000, 0x1000, 0x1000}});
    unit.SetPosition({-0x20000, 0, 0});
    unit.SetVelocity({0x10000, 0, 0});
    unit.Update();
    CHECK(unit.GetCollisionBounds().minimum.x == unit.GetPosition().x - 0x1000);
    CHECK(unit.GetCollisionBounds().maximum.x == unit.GetPosition().x + 0x1000);

    soft3d::GameUnitManager units;
    ManagedUnit blocker(1);
    blocker.SetCollisionBounds({{-0x10000, -0x10000, -0x10000},
                                {0x10000, 0x10000, 0x10000}});
    units.AddUnit(&blocker, 0);
    unit.SetUnitManager(&units);
    unit.SetPosition({-0x20000, 0, 0});
    unit.SetVelocity({0x40000, 0, 0});
    unit.MoveSimple();
    CHECK(unit.GetVelocity().x == 0 && unit.GetPosition().x == -0x20000);
    CHECK(unit.HasUnitCollision());

    units.AddUnit(new soft3d::Things(nullptr, 3, nullptr, nullptr, bsp.get()), 2);
    soft3d::Things* thing = static_cast<soft3d::Things*>(units.GetFirstUnit(2));
    thing->SetUnitManager(&units);
    thing->StartDestruction(2);
    CHECK(thing->IsDestroying());
    units.UpdateUnits();
    CHECK(units.GetBucketCount(2) == 1);
    units.UpdateUnits();
    CHECK(units.GetBucketCount(2) == 0);
    units.ResetBuckets();
}

void TestCreature()
{
    soft3d::Creature creature;
    creature.SetPosition({0, 0, 0});
    CHECK(creature.GetUnitType() == 6);
    CHECK(creature.GetNavigationSpeed() == 0x8000);
    creature.SetNavigationEnabled(true);
    CHECK(creature.IsNavigationEnabled());
    CHECK(creature.NavigateTo({0x1000, 0, 0}, 0x2000));
    CHECK(creature.GetPosition().x == 0x1000);
    CHECK(creature.HearSound({0x1000, 0, 0}, 0x2000, 7));
    CHECK(!creature.HearSound({0x1000, 0, 0}, 0x2000, 7));
    CHECK(!creature.HearSound({0x40000, 0, 0}, 0x2000, 7));
    CHECK(creature.HearSound({0x2000, 0, 0}, 0x3000, -1));
    CHECK(!creature.HearSound({0x2000, 0, 0}, 0x3000, -1));
    CHECK(creature.IsWithinRadius({0x1000, 0, 0}, 0));
    soft3d::Things thing;
    CHECK(!thing.MoveTowards({0x2000, 0, 0}, 0x1000));
    CHECK(thing.GetPosition().x == 0x1000);
    CHECK(thing.MoveTowards({0x2000, 0, 0}, 0x1000));
    CHECK(thing.GetPosition().x == 0x2000);
}

soft3d::BspEntity* CreateEntity(soft3d::BspEntityRecord*, void*, soft3d::BspEntityRecord*)
{
    return new soft3d::BspEntity;
}

void TestRealResources()
{
#ifdef SOFT3D_SPK_ENGLISH_LAYOUT
    std::cout << "SKIP: real archive uses the Chinese layout\n";
#else
    soft3d::s3dResManager manager;
    CHECK(soft3d::OpenGameResourceArchive(&manager, SOFT3D_TEST_ARCHIVE) == soft3d::S3D_OK);
    soft3d::Renderer renderer(320, 240);
    soft3d::Bsp bsp(&renderer, &manager);
    bsp.SetEntityFactory(CreateEntity, nullptr);
    CHECK(soft3d::LoadSbpMapChunks(&bsp, u".\\common\\", u"menu.sbp", nullptr) == soft3d::S3D_OK);
    const soft3d::Vec3 position = bsp.GetVertices().front();
    const int32_t leaf = bsp.FindLeaf(position);
    CHECK(leaf >= 0 && static_cast<std::size_t>(leaf) < bsp.GetLeaves().size());
    const soft3d::BspTraceResult trace = bsp.Trace(position,
        {position.x + 0x1000, position.y, position.z});
    CHECK(trace.fraction >= 0 && trace.fraction <= soft3d::fixedOne);
    const soft3d::BspTraceResult point = bsp.Trace(position, position);
    CHECK(point.endPosition.x == position.x && point.endPosition.y == position.y &&
          point.endPosition.z == position.z);

    soft3d::VisUnit visual(&manager, 0, &renderer, nullptr);
    CHECK(visual.GetUnitType() == 1);
    CHECK(visual.LoadObjects(u"cg_bully_tuomao_a", nullptr, 2) == soft3d::S3D_OK);
    CHECK(visual.LoadMaterials() == soft3d::S3D_OK);
    CHECK(visual.GetObjectCount() == 2);
    visual.SetPosition({0x10000, 0x20000, 0x30000});
    visual.Update();
    CHECK(visual.GetObject(0) != nullptr && visual.GetObject(1) != nullptr);
    CHECK(visual.GetObject(0)->GetModelMatrix().values[12] == 0x10000);
    visual.SetDirection({0x10000, 0, 0x10000});
    visual.EnableShadow(true);
    visual.Update();
    soft3d::RenderQueues queues;
    visual.SubmitRenderables(queues);
    CHECK(queues.GetCount(3) + queues.GetCount(4) == 1);
#endif
}

} // namespace

int main()
{
    try {
        TestMessagingAndGeometry();
        TestManager();
        TestStateMachine();
        TestBspCollision();
        TestPhysics();
        TestCreature();
        TestRealResources();
        std::cout << "Unit, state, and physics tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
