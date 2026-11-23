#pragma once

#include "soft3d/AudioManager.hpp"
#include "soft3d/Effects.hpp"
#include "soft3d/Object.hpp"
#include "soft3d/SoundInstance.hpp"
#include "soft3d/Stream.hpp"
#include "soft3d/s3dBase.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace soft3d {

class Bsp;
struct BspPathNode;
class GameUnit;
class GameUnitManager;
class MsgReceiver;

struct UnitMessage {
    uint32_t type = 0;
    uint32_t value = 0;
    uint32_t data = 0;
};

class MsgReceiverInterface {
public:
    virtual ~MsgReceiverInterface() = default;
    virtual MsgReceiver* GetMsgReceiver();
    virtual void ProcessMessages();
};

class MsgReceiver : public MsgReceiverInterface {
public:
    static constexpr std::size_t capacity = 20;

    MsgReceiver();

    void AddMessage(uint32_t type, uint32_t value, uint32_t data);
    void ClearMessages();
    std::size_t GetMessageCount() const;
    const UnitMessage& GetMessage(std::size_t index) const;

private:
    std::array<UnitMessage, capacity> messages{};
    std::size_t messageCount = 0;
};

void SendUnitMessage(MsgReceiverInterface* receiver, uint32_t type, uint32_t value, uint32_t data);

class GameUnit : public MsgReceiverInterface {
public:
    GameUnit();
    ~GameUnit() override = default;

    virtual GameUnit* QueryPoint(const Vec3& point);
    virtual GameUnit* QuerySegment(const Vec3& start, const Vec3& end);
    virtual GameUnit* QueryBounds(const ObjectBounds& bounds);
    virtual void SubmitRenderables(RenderQueues& queues);
    virtual void Update();
    virtual void PostUpdate();
    virtual void SaveState(OutputStream& stream) const;
    virtual void LoadState(InputStream& stream);

    int32_t GetUnitType() const;
    int32_t GetUnitId() const;
    GameUnit* GetNextUnit() const { return nextUnit; }
    const Vec3& GetPosition() const;
    const ObjectBounds& GetLocalBounds() const;
    const ObjectBounds& GetCollisionBounds() const;
    bool IsVisible() const;
    bool IsCollisionEnabled() const;

    void SetUnitType(int32_t value);
    void SetUnitId(int32_t value);
    void SetPosition(const Vec3& value);
    void SetLocalBounds(const ObjectBounds& value);
    void SetCollisionBounds(const ObjectBounds& value);
    void SetVisible(bool value);
    void SetCollisionEnabled(bool value);

protected:
    int32_t unitType = 0;
    int32_t unitId = -1;
    Vec3 position;
    ObjectBounds localBounds;
    ObjectBounds collisionBounds;
    GameUnit* nextUnit = nullptr;
    bool visible = true;
    bool collisionEnabled = true;

    friend class GameUnitManager;
};

class VisUnit : public GameUnit {
public:
    VisUnit(s3dResManager* resources = nullptr, int cacheLevel = 3,
            Renderer* renderer = nullptr, AudioManager* audio = nullptr);
    ~VisUnit() override;

    void SubmitRenderables(RenderQueues& queues) override;
    void Update() override;
    void PostUpdate() override;
    void SaveState(OutputStream& stream) const override;
    void LoadState(InputStream& stream) override;
    virtual result LoadMaterials(const char16_t* directory = nullptr);
    virtual void SetAnimation(const Animation* animation, int32_t direction = 1,
                              bool loop = true);
    virtual void SetDirection(const Vec3& direction);
    virtual void SetAxisX(const Vec3& axis);
    virtual void SetAxisY(const Vec3& axis);
    virtual void SetAxisZ(const Vec3& axis);

    result LoadObjects(const char16_t* name, const char16_t* directory, int count);
    result LoadAlternateObject(const char16_t* name, const char16_t* directory = nullptr);
    result LoadAnimation(const char16_t* name, const char16_t* directory = nullptr);
    void EnableShadow(bool enabled);
    void SetPositionalAudio(bool enabled);
    void StopSound();
    int PlaySound(AudioData* data, bool looping = false);
    int PlaySound(int nameIndex, bool looping = false);
    void SetMaterialIntensityBounce(Fixed minimum, Fixed maximum, int32_t period);
    void SetDepthBias(Fixed bias);
    void SetAnimationPaused(bool paused);
    void SetAnimationTime(int32_t time);
    void SetAnimationSpeed(int32_t speed);
    void SetAnimationPlayback(int32_t direction, bool loop, int32_t speed);

    Object* GetObject(std::size_t index = 0) const;
    Object* GetShadowObject() const { return shadowObject.get(); }
    std::size_t GetObjectCount() const;
    const Animation* GetAnimation() const;
    bool IsAnimationFinished() const;
    bool IsShadowEnabled() const;
    bool IsShadowTransformDirty() const;
    bool IsPositionalAudio() const;
    int GetCurrentObjectIndex() const;
    Renderer* GetRenderer() const { return renderer; }

protected:
    struct ObjectGroup {
        std::vector<std::unique_ptr<Object>> objects;
        std::size_t current = 0;
        Fixed cameraDepth = 0;
    };

    void UpdateVisuals();
    void UpdateShadowTransform(const Matrix4& matrix);

    std::unique_ptr<ObjectGroup> objectGroup;
    Object* primaryObject = nullptr;
    bool shadowEnabled = false;
    bool shadowTransformDirty = false;
    std::unique_ptr<Object> shadowObject;
    std::unique_ptr<FakeShadow> fakeShadow;
    const Animation* animation = nullptr;
    bool animationFinished = false;
    s3dResManager* resourceManager;
    Renderer* renderer;
    int cacheLevel;
    AudioManager* audioManager;
    bool positionalAudio = false;
    int soundVoice = -1;
    std::unique_ptr<SoundInstance> soundInstance;
};

enum PhysicUnitFlags : uint32_t {
    PhysicApplyFriction = 1U << 0,
    PhysicSlideMovement = 1U << 1,
    PhysicSimpleMovement = 1U << 2,
    PhysicStepMovement = 1U << 3,
    PhysicStepOffset = 1U << 4,
    PhysicRestoreX = 1U << 5,
    PhysicRestoreY = 1U << 6,
    PhysicRestoreZ = 1U << 7,
    PhysicKeepDirection = 1U << 8,
    PhysicSeparateAxisCollision = 1U << 9,
};

struct BspTraceResult;

struct UnitQueryResult {
    int32_t count = 0;
    std::array<GameUnit*, 64> units{};
};

class PhysicUnit : public VisUnit {
public:
    PhysicUnit(s3dResManager* resources = nullptr, int cacheLevel = 3,
               Renderer* renderer = nullptr, AudioManager* audio = nullptr, Bsp* bsp = nullptr);
    ~PhysicUnit() override;

    void Update() override;
    void SetAnimation(const Animation* animation, int32_t direction = 1,
                      bool loop = true) override;
    virtual void QueryUnitCollision();
    virtual void MoveSlide();
    virtual void MoveSimple();

    void SetPhysicsFlags(uint32_t value);
    void SetVelocity(const Vec3& value);
    void SetFriction(Fixed value);
    void SetYawVelocity(uint16_t value);
    void SetUnitManager(GameUnitManager* manager);
    void SetBsp(Bsp* value);
    uint32_t GetPhysicsFlags() const;
    const Vec3& GetVelocity() const;
    uint16_t GetYawVelocity() const { return yawVelocity; }
    const BspTraceResult& GetGroundTrace() const;
    const UnitQueryResult& GetUnitQueryResult() const;
    bool HasUnitCollision() const;
    const std::array<bool, 3>& GetAxisCollisionFlags() const;

protected:
    bool IsOnGround() const;
    bool IsVelocityNearlyZero() const;
    void CheckGround();
    void UpdateShadow();

    uint32_t physicsFlags = 0xf;
    Vec3 previousPosition;
    Vec3 velocity;
    uint16_t yawVelocity = 0;
    Fixed friction = 0x8000;
    ObjectBounds secondaryLocalBounds;
    ObjectBounds secondaryCollisionBounds;
    std::unique_ptr<BspTraceResult> groundTrace;
    Bsp* bsp;
    GameUnitManager* unitManager = nullptr;
    std::unique_ptr<BspTraceResult> shadowTrace;
    UnitQueryResult unitQueryResult;
    bool unitCollision = false;
    std::array<bool, 3> axisCollision{};
};

class Things : public PhysicUnit {
public:
    Things(s3dResManager* resources = nullptr, int cacheLevel = 3,
           Renderer* renderer = nullptr, AudioManager* audio = nullptr, Bsp* bsp = nullptr);
    ~Things() override;

    GameUnit* QueryPoint(const Vec3& point) override;
    GameUnit* QuerySegment(const Vec3& start, const Vec3& end) override;
    GameUnit* QueryBounds(const ObjectBounds& bounds) override;
    void SubmitRenderables(RenderQueues& queues) override;
    void Update() override;
    void PostUpdate() override;
    void SaveState(OutputStream& stream) const override;
    void LoadState(InputStream& stream) override;
    result LoadMaterials(const char16_t* directory = nullptr) override;
    void SetAnimation(const Animation* animation, int32_t direction = 1,
                      bool loop = true) override;
    virtual void StartDestruction(uint16_t duration);

    result LoadAlternateModel(const char16_t* name, const char16_t* directory = nullptr);
    result SetAlternateAnimation(const char16_t* name, const char16_t* directory = nullptr);
    bool MoveTowards(const Vec3& target, Fixed distance);
    void UpdateCurrentLeaf();
    bool IsDestroying() const;
    uint16_t GetDestructionTimer() const;
    uint16_t GetDestructionDuration() const { return destructionDuration; }
    int32_t GetCurrentLeaf() const { return currentLeaf; }
    int32_t GetSerializedState67c() const { return serializedState67c; }
    void SetSerializedState67c(int32_t value) { serializedState67c = value; }
    Object* GetAlternateObject() const { return alternateObject.get(); }
    const Animation* GetAlternateAnimation() const { return alternateAnimation; }
    void SetCurrentLeaf(int32_t value) { currentLeaf = value; }

private:
    bool placedOnGround = true;
    bool destroying = false;
    uint16_t destructionDuration = 0;
    uint16_t destructionTimer = 0;
    std::unique_ptr<Object> alternateObject;
    const Animation* alternateAnimation = nullptr;
    int32_t serializedState67c = 0;
    int32_t currentLeaf = -1;
};

class Creature : public PhysicUnit {
public:
    Creature(s3dResManager* resources = nullptr, int cacheLevel = 3,
             Renderer* renderer = nullptr, AudioManager* audio = nullptr, Bsp* bsp = nullptr);
    ~Creature() override = default;

    virtual bool NavigateTo(const Vec3& target, Fixed radius);
    virtual void SetNavigationEnabled(bool enabled);
    virtual bool IsPointInside(const Vec3& point);
    virtual bool IsSegmentInside(const Vec3& start, const Vec3& end);
    virtual bool IsBoundsInside(const ObjectBounds& bounds);
    virtual bool IsWithinRadius(const Vec3& center, Fixed radius);

    bool HearSound(const Vec3& source, Fixed range, int32_t soundId);
    void SnapToGround();
    void SetNavigationSpeed(Fixed value);
    Fixed GetNavigationSpeed() const;
    const Vec3& GetNavigationTarget() const;
    bool IsNavigationEnabled() const;
    void ResetNavigationPath();

protected:
    void RestoreNavigationEnabled(bool enabled) { navigationEnabled = enabled; }
    void ResetNavigationPathNodes();

private:
    void SetNavigationVelocity(const Vec3& target);

    Fixed navigationSpeed = 0x8000;
    const BspPathNode* previousPathNode = nullptr;
    const BspPathNode* currentPathNode = nullptr;
    bool reachedCurrentPathNode = false;
    bool navigationEnabled = false;
    bool destinationReached = false;
    Vec3 navigationTarget;
    Vec3 lastHeardPosition;
    int32_t lastHeardSound = 0;
};

struct UnitManagerContext {
    s3dResManager* resources = nullptr;
    Renderer* renderer = nullptr;
    Bsp* bsp = nullptr;
    AudioManager* audio = nullptr;
    void* gameContext = nullptr;
    void* sceneContext = nullptr;
};

class GameUnitManager : public s3dBase {
public:
    explicit GameUnitManager(const UnitManagerContext& context = {});
    ~GameUnitManager() override = default;

    void SetEngine(Engine* engine) override;

    void AddUnit(GameUnit* unit, int bucket);
    void RemoveUnit(GameUnit* unit, int bucket, bool destroy);
    void RemoveUnitById(int32_t id, int bucket, bool destroy);
    void ClearBucket(int bucket, bool destroy);
    void ResetBuckets();
    GameUnit* FindUnit(int bucket, int32_t id) const;

    void UpdateUnits();
    void PostUpdateUnits();
    void SubmitUnits(RenderQueues& queues);

    const UnitQueryResult& QueryPoint(const Vec3& point, int bucket = -1,
                                      const GameUnit* excluded = nullptr);
    const UnitQueryResult& QuerySegment(const Vec3& start, const Vec3& end, int bucket = -1,
                                        const GameUnit* excluded = nullptr);
    const UnitQueryResult& QueryBounds(const ObjectBounds& bounds, int bucket = -1,
                                       const GameUnit* excluded = nullptr);

    std::size_t GetBucketCount(int bucket) const;
    GameUnit* GetFirstUnit(int bucket) const;
    const UnitManagerContext& GetContext() const;

private:
    struct Bucket {
        std::size_t count = 0;
        GameUnit* first = nullptr;
    };

    bool ValidBucket(int bucket) const;
    void ResetQuery();

    UnitManagerContext context;
    std::array<Bucket, 8> buckets{};
    UnitQueryResult queryResult;
};

} // namespace soft3d
