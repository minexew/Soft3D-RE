#include "game/Scene.hpp"

#include "game/MapUnits.hpp"
#include "game/Triggers.hpp"
#include "soft3d/AudioData.hpp"
#include "soft3d/GameUIDlg.hpp"
#include "soft3d/TextureRes.hpp"

#include <stdexcept>

namespace game {
namespace {

void RequireStatus(soft3d::result status, const char* operation)
{
    if (status != soft3d::S3D_OK) {
        throw std::runtime_error(std::string(operation) + " failed with Soft3D result " +
                                 std::to_string(status));
    }
}

uint16_t PackRgb565(int32_t red, int32_t green, int32_t blue)
{
    return static_cast<uint16_t>(((red & 0xff) >> 3) << 11 |
                                 ((green & 0xff) >> 2) << 5 | ((blue & 0xff) >> 3));
}

soft3d::ObjectBounds TranslateBounds(const soft3d::ObjectBounds& bounds,
                                     const soft3d::Vec3& position)
{
    return {soft3d::Add(bounds.minimum, position), soft3d::Add(bounds.maximum, position)};
}

} // namespace

const soft3d::Vec3& SceneCameraSpot::GetPosition() const
{
    static const soft3d::Vec3 empty;
    return unit != nullptr ? unit->GetPosition() : empty;
}

void SceneCameraSpot::SetRight(const soft3d::Vec3& value)
{
    right = value;
    if (unit != nullptr) {
        unit->SetAxisX(value);
    }
}

void SceneCameraSpot::SetUp(const soft3d::Vec3& value)
{
    up = value;
    if (unit != nullptr) {
        unit->SetAxisY(value);
    }
}

void SceneCameraSpot::SetForward(const soft3d::Vec3& value)
{
    forward = value;
    if (unit != nullptr) {
        unit->SetAxisZ(value);
    }
}

Scene::Scene(soft3d::GameEngine& owner)
    : engine(owner)
    , bsp(std::make_unique<soft3d::Bsp>(&owner))
    , cameraPaths(std::make_unique<soft3d::CameraPathCollection>(64))
    , inkShader(owner.GetWidth(), owner.GetHeight())
    , fadeShader(owner.GetWidth(), owner.GetHeight(), &owner.GetRenderer())
    , grayShader(owner.GetWidth(), owner.GetHeight())
    , blindShader(owner.GetWidth(), owner.GetHeight())
    , twistShader(owner.GetWidth(), owner.GetHeight())
    , motionBlurShader(owner.GetWidth(), owner.GetHeight())
{
    ResetEnvironmentAudio();
}

Scene::~Scene()
{
    UnloadLevel();
}

soft3d::result Scene::Initialize()
{
    if (initialized) {
        return soft3d::S3D_OK;
    }
    soft3d::UnitManagerContext context;
    context.resources = &engine.GetResourceManager();
    context.renderer = &engine.GetRenderer();
    context.bsp = bsp.get();
    context.audio = &engine.GetAudioManager();
    context.gameContext = &engine;
    context.sceneContext = this;
    unitManager = std::make_unique<MyGameUnitMgr>(context);
    unitManager->SetEngine(&engine);
    soft3d::result status = unitManager->Initialize(engine);
    if (status != soft3d::S3D_OK) {
        return status;
    }

    player = std::make_unique<Player>(&engine.GetResourceManager(), &engine.GetRenderer(),
                                      bsp.get(), this, &engine.GetAudioManager());
    player->SetUnitManager(unitManager.get());
    status = player->Initialize(unitManager->GetData());
    if (status != soft3d::S3D_OK) {
        return status;
    }
    player->SetDirection(spawnDirection);
    playerSpot.SetUnit(player.get());
    playerSpot.SetForward(spawnDirection);
    unitManager->ConfigureScene(&cameraController, player.get());
    cameraController.SetServices(&engine.GetCamera(), bsp.get(), &engine.GetControlMap(),
                                 &engine.GetInput(), unitManager->GetRandomState());
    cameraController.Bind(&playerSpot, true);
    cameraController.SetMode(CameraController::OrbitFollow, {},
                             soft3d::Normalize({0, -0x6666, -0x10000}),
                             0x1a8000, nullptr, 0, false, 0);

    doorModel = std::make_unique<soft3d::VisUnit>(&engine.GetResourceManager(), 3,
                                                   &engine.GetRenderer(), &engine.GetAudioManager());
    bsp->SetEntityFactory(CreateGameEntity, this);
    engine.GetRenderer().SetMinimumIntensity(0x0ccc);
    inkShader.SetEngine(&engine);

    soft3d::s3dFilePak* filePak = engine.GetResourceManager().GetFilePak();
    status = soft3d::LoadS3dStyleResource(bsp.get(), u".\\common\\default.s3dsty", filePak);
    if (status != soft3d::S3D_OK) {
        return status;
    }
    status = soft3d::LoadSplCameraResource(*cameraPaths, u".\\common\\1000.spl", filePak);
    if (status != soft3d::S3D_OK) {
        return status;
    }
    initialized = true;
    return soft3d::S3D_OK;
}

void Scene::StartNewGame(const soft3d::NewGameOptions& options)
{
    if (!initialized) {
        throw std::runtime_error("Scene is not initialized");
    }
    newGameOptions = options;
    engine.GetScriptEngine().Reset();
    unitManager->Reset();
    restoreSavedState = 0;
    saveSlotPending = 0;
    nextLevelPending = 0;
    nextMapId = 0;
    nextEpisodeId = 0;
    transitionStage = 0;
    transitionTimer = 0;
    mapProgress = -1;
    useDay1Fallback = 0;
    cameraWarmupCounter = 0;
    useCameraListener = 0;
    gameOverStage = 0;
    specialDoorIndex = -1;
    eventFlags.fill(0);
    player->ResetAll(unitManager->GetData());
    player->SetPosition({});
    player->SetDirection({0, 0, soft3d::fixedOne});
    playerSpot.SetForward({0, 0, soft3d::fixedOne});
    player->SetVisible(true);
    cameraController.Bind(&playerSpot, true);
    cameraController.Reset();
    engine.GetRenderer().SelectShader(nullptr);
    RequireStatus(LoadDay(1), "Scene_StartNewGame");
}

bool Scene::LoadSaveSlot(int slot)
{
    throw std::runtime_error("Scene_LoadSave requires the full stage-13 reader; slot " +
                             std::to_string(slot));
}

void Scene::ReturnToMainMenu()
{
    for (int action = 1; action <= 7; ++action) {
        engine.GetControlMap().SetActionEnabled(action, false);
    }
    cameraController.SetFrozen(false);
    UnloadLevel();
    engine.GetScriptEngine().Unload();
    engine.GetRenderer().SetFogEnabled(false);
    engine.GetRenderer().SelectShader(nullptr);
    inkShader.StopInk();
    inkShader.Reset();
    saveSlotPending = 0;
    showDayTitle = 0;
    dayIntroPending = 0;
    nextLevelPending = 0;
    transitionStage = 0;
    transitionTimer = 0;
    mapProgress = -1;
    day = 0;
    mapId = 0;
    gameOverStage = 0;
    specialDoorIndex = -1;
    engine.GetAudioManager().PlaySound(u"e_wind01", true);
    engine.GetInput().ClearKey();
}

soft3d::result Scene::LoadDay(int value)
{
    if (!initialized || unitManager == nullptr || player == nullptr) {
        return soft3d::S3D_STATE;
    }
    if (value < 1 || value > 7) {
        return soft3d::S3D_PARAM;
    }
    engine.GetResourceManager().ClearCache(2);
    day = value;
    dayIntroPending = 1;
    showDayTitle = 1;
    transitionTimer = 0;

    soft3d::StringManager* font = engine.GetBaseFont();
    font->Clear();
    char16_t path[32];
    soft3d::WideSprintf(path, engine.GetLanguage() == 2 ? u".\\uien\\day%d.sbn"
                                                        : u".\\ui\\day%d.sbn", day);
    font->LoadSbnFont(path, engine.GetResourceManager().GetFilePak());
    player->Reset(unitManager->GetData());
    soft3d::result status = unitManager->LoadDayContent(day, engine);
    if (status != soft3d::S3D_OK) {
        return status;
    }
    const bool extraWeapon = engine.GetConfiguration().unlocks.extraWeapon1 != 0;
    if (day == 2) player->PrepareWeapon(unitManager->GetData(), extraWeapon ? 4 : 1);
    if (day == 3) player->PrepareWeapon(unitManager->GetData(), extraWeapon ? 4 : 2);
    if (day == 5) {
        player->PrepareWeapon(unitManager->GetData(), extraWeapon ? 4 : 3);
        player->PrepareWeapon(unitManager->GetData(), 1);
    }
    if (day == 7) player->PrepareWeapon(unitManager->GetData(), 3);
    engine.GetScriptEngine().ClearSharedVariables();
    dayStartSnapshot = eventFlags;
    player->SnapshotDayState();
    if (fadeShader.IsCompleted()) {
        fadeShader.StartFade(0, PackRgb565(0, 0, 0), 8, 1);
    }
    return soft3d::S3D_OK;
}

int Scene::LoadMap(int value)
{
    if (!initialized || unitManager == nullptr || player == nullptr) {
        return soft3d::S3D_STATE;
    }
    mapId = value;
    engine.GetContent().mapId = value;
    BeginMapLoad();
    unitManager->SetMapId(value);
    UnloadLevel();
    player->SetVisible(false);
    engine.GetRenderer().Clear(0);
    engine.GetInput().ClearKey();
    return 0;
}

void Scene::LoadNextLevel(int nextMap, int nextEpisode)
{
    cameraController.ClearTriggers();
    cameraController.SetFrozen(false);
    nextLevelPending = 1;
    nextMapId = nextMap;
    nextEpisodeId = nextEpisode;
}

void Scene::SetLoadMapParams(int resourceIndex, const soft3d::Vec3& position,
                             const soft3d::Vec3& direction, uint16_t color)
{
    doorResourceIndex = resourceIndex;
    spawnPosition = position;
    spawnDirection = direction;
    transitionColor = color;
}

void Scene::BeginMapLoad()
{
    if (transitionStage != 0) {
        return;
    }
    const bool standardDoor = doorResourceIndex >= 0 &&
        (static_cast<uint32_t>(doorResourceIndex) & 0xffff0000U) != 0x10000U;
    if (standardDoor) {
        if (doorResourceIndex == 0) {
            cameraController.SetMode(CameraController::FixedMode, player->GetPosition(),
                                     {0, 0, soft3d::fixedOne}, 0, nullptr, 0, false, 0);
            cameraController.SetCurrentTarget(player->GetPosition());
            player->SetDirection({0, 0, soft3d::fixedOne});
            playerSpot.SetForward({0, 0, soft3d::fixedOne});
            transitionCameraPosition = soft3d::Subtract(player->GetPosition(),
                soft3d::Scale(playerSpot.GetForward(), 0x90000));
        } else {
            transitionCameraPosition = player->GetPosition();
        }
        const soft3d::Fixed halfHeight = soft3d::SubtractFixed(
            player->GetLocalBounds().maximum.y, player->GetLocalBounds().minimum.y) >> 1;
        transitionCameraPosition.y = soft3d::AddFixed(transitionCameraPosition.y, halfHeight);
        transitionCameraPosition = soft3d::Add(transitionCameraPosition, playerSpot.GetForward());
        transitionCameraTarget = soft3d::Add(transitionCameraPosition,
                                             soft3d::Scale(playerSpot.GetForward(), 0x20000));
        cameraController.SetCurrentTarget(transitionCameraTarget);
        transitionCameraSpeed = soft3d::Length(soft3d::Subtract(
            transitionCameraPosition, cameraController.GetPosition())) >> 3;
        cameraController.Bind(nullptr, true);
        cameraController.SetDirectFollow(false, 0);
    }
    engine.GetRenderer().SetLightingEnabled(true);
    engine.GetRenderer().SetGlobalLightDirection(soft3d::Normalize({0xa666, -0x10000, 0xcccc}));
    engine.GetRenderer().SetAmbientIntensity(0xb333);
    engine.GetRenderer().SetFogEnabled(false);
    if (engine.GetControlMap().IsEnabled()) {
        engine.GetInput().ClearKey();
    }
    engine.GetControlMap().SetEnabled(false);
    if (!fadeShader.IsActive()) {
        fadeShader.StartFade(1, transitionColor, 8, 1);
    }
    if (doorResourceIndex >= 0) {
        player->SetVisible(true);
    }
    transitionStage = 1;
    transitionTimer = 0;
    mapProgress = 0;
    useDay1Fallback = 0;
    engine.GetAudioManager().SetEffectsVolume(255);
    engine.GetAudioManager().SetMusicVolume(255);
    if ((static_cast<uint32_t>(doorResourceIndex) & 0xffff0000U) == 0x10000U) {
        specialDoorIndex = doorResourceIndex & 0xffff;
    } else {
        specialDoorIndex = -1;
    }
}

void Scene::FinishMapLoad()
{
    player->ResetForMap();
    player->SetVisible(true);
    player->EnterFPS();
    cameraController.Bind(&playerSpot, true);
    cameraController.Reset();
    cameraController.SetInstant(true);
    cameraController.Update(nullptr, true);
    unitManager->FinishMapLoad(restoreSavedState != 0);
    engine.GetControlMap().SetEnabled(true);
    engine.GetInput().ClearKey();
    cameraWarmupCounter = 0;
    engine.GetScriptEngine().StartTasks();
}

void Scene::UnloadLevel()
{
    cameraController.ClearTriggers();
    if (player != nullptr) {
        player->StopSound();
    }
    engine.GetAudioManager().StopAllSounds();
    engine.GetAudioManager().ClearSoundInstances();
    if (engine.GetGameUi() != nullptr) {
        engine.GetGameUi()->ResetLevelState();
    }
    if (bsp != nullptr) {
        bsp->FreeLastLevel();
    }
    if (unitManager != nullptr) {
        unitManager->Reset();
    }
    engine.GetRenderQueues().Clear();
    engine.GetRenderer().Clear(0);
    engine.GetResourceManager().ClearCache(3);
    ResetEnvironmentAudio();
    useCameraListener = 0;
}

void Scene::ResetEnvironmentAudio()
{
    for (EnvironmentChannel& channel : environment) {
        channel = EnvironmentChannel{};
    }
}

bool Scene::AddEnvironmentSound(int index, const char16_t* name, int32_t baseDelay,
                                int32_t randomDelay)
{
    if (index < 0 || index >= 2) {
        return false;
    }
    EnvironmentChannel& channel = environment[index];
    if (channel.resourceCount >= 4) {
        return false;
    }
    auto* data = static_cast<soft3d::AudioData*>(engine.GetResourceManager().GetResource(
        soft3d::ResourceAudio, 3, name, u".\\audio\\"));
    channel.resources[channel.resourceCount] = data;
    channel.baseDelay = baseDelay;
    channel.randomDelay = randomDelay;
    channel.nextDelay = -1;
    ++channel.resourceCount;
    channel.voice = 0;
    return true;
}

bool Scene::ClearEnvironmentChannel(int index)
{
    if (index < 0 || index >= 2) {
        return false;
    }
    EnvironmentChannel& channel = environment[index];
    channel.resources.fill(nullptr);
    channel.resourceCount = 0;
    channel.voice = 0;
    return true;
}

const Scene::EnvironmentChannel& Scene::GetEnvironmentChannel(int index) const
{
    if (index < 0 || index >= 2) {
        throw std::out_of_range("Scene environment channel " + std::to_string(index));
    }
    return environment[index];
}

void Scene::UpdateEnvironmentAudio()
{
    for (EnvironmentChannel& channel : environment) {
        if (channel.resourceCount == 0) {
            continue;
        }
        if (channel.nextDelay == -1) {
            const int32_t random = channel.randomDelay != 0
                ? static_cast<int32_t>(unitManager->NextRandom() %
                    static_cast<uint32_t>(channel.randomDelay << 7) >> 7)
                : 0;
            channel.nextDelay = channel.baseDelay + random;
            channel.elapsed = 0;
            continue;
        }
        if (channel.voice != 0) {
            if (!engine.GetAudioManager().IsVoiceFinished(channel.voice)) {
                continue;
            }
            channel.voice = 0;
        }
        ++channel.elapsed;
        if (channel.elapsed >= channel.nextDelay) {
            channel.nextDelay = -1;
            const int32_t selection = static_cast<int32_t>(unitManager->NextRandom() %
                static_cast<uint32_t>(channel.resourceCount << 7) >> 7);
            channel.voice = engine.GetAudioManager().PlaySound(channel.resources[selection], false);
        }
    }
}

void Scene::LoadNextScript()
{
    char16_t path[64];
    if (nextEpisodeId < 1000 && nextEpisodeId != 0) {
        soft3d::WideSprintf(path, u".\\day%d\\ep0_%d", day, nextEpisodeId);
    } else {
        soft3d::WideSprintf(path, u".\\day%d\\ep%d", day, nextEpisodeId);
    }
    scriptName = path;
    RequireStatus(engine.GetScriptEngine().LoadFromPak(path, engine.GetResourceManager().GetFilePak()),
                  "Scene_LoadNextScript");
    engine.GetScriptEngine().Start();
    if (nextMapId != 0) {
        LoadMap(nextMapId);
    }
    nextLevelPending = 0;
}

void Scene::UpdateMapLoad()
{
    if (mapProgress < 0) {
        return;
    }
    int delay = 1;
    if (restoreSavedState == 0) {
        if (engine.GetScriptEngine().GetState() == soft3d::ScriptState::Initialized) {
            delay = 2;
        }
    } else if (transitionTimer == 29) {
        delay = 2;
    }

    if (mapProgress < 100) {
        char16_t directory[32];
        char16_t fileName[34];
        soft3d::WideSprintf(directory, u".\\day%d\\", day);
        if (mapId < 1000) {
            soft3d::WideSprintf(fileName, u"0_%d.sbp", mapId);
        } else {
            soft3d::WideSprintf(fileName, u"%d.sbp", mapId);
        }
        soft3d::result status;
        if (useDay1Fallback == 0) {
            status = soft3d::LoadSbpMapChunks(bsp.get(), directory, fileName, &mapProgress);
            if (status != soft3d::S3D_OK) {
                mapProgress = 0;
                status = soft3d::LoadSbpMapChunks(bsp.get(), u".\\day1\\", fileName, &mapProgress);
                if (status != soft3d::S3D_OK) {
                    bsp->FreeAll();
                    RequireStatus(status, "Scene_UpdateMapLoad day1 fallback");
                }
                useDay1Fallback = 1;
            }
        } else {
            status = soft3d::LoadSbpMapChunks(bsp.get(), u".\\day1\\", fileName, &mapProgress);
            if (status != soft3d::S3D_OK) {
                bsp->FreeAll();
                RequireStatus(status, "Scene_UpdateMapLoad fallback continuation");
            }
        }
        if (mapProgress == 100) {
            player->SetVisible(false);
        }
    } else if (mapProgress != 103) {
        mapProgress = 100 + delay;
    }
    if (mapProgress == 102) {
        ++mapProgress;
    } else if (mapProgress == 103) {
        FinishMapLoad();
        mapProgress = -1;
    }
}

void Scene::UpdateTransition()
{
    const bool standardDoor = doorResourceIndex >= 0 &&
        (static_cast<uint32_t>(doorResourceIndex) & 0xffff0000U) != 0x10000U;
    if (transitionStage == 1) {
        showDayTitle = 0;
        if (standardDoor) {
            cameraController.MoveCamera(transitionCameraPosition, transitionCameraTarget,
                                        transitionCameraSpeed);
        }
        if (transitionTimer == 8) {
            if (standardDoor) {
                const DoorResourceRecord& resource = unitManager->GetData().GetDoorResource(
                    static_cast<std::size_t>(doorResourceIndex));
                RequireStatus(doorModel->LoadObjects(resource.modelName.data(), nullptr, 1),
                              "Scene transition door model");
                if (doorModel->GetObject(0) != nullptr) {
                    doorModel->GetObject(0)->SetAnimationPaused(false);
                }
                RequireStatus(doorModel->LoadAnimation(
                    unitManager->GetData().GetAnimationName(238 + doorResourceIndex), nullptr),
                    "Scene transition door animation");
                doorModel->SetAnimation(doorModel->GetAnimation(), 1, false);
                doorModel->SetPosition(soft3d::Add(transitionCameraTarget,
                    soft3d::Scale(playerSpot.GetForward(), 0x140000)));
                doorModel->SetDirection(playerSpot.GetForward());
            }
            fadeShader.StartFade(0, transitionColor, 4, 1);
            if (restoreSavedState == 0) {
                engine.GetAudioManager().StopAllSounds();
            } else {
                for (int voice = 1; voice < 16; ++voice) {
                    engine.GetAudioManager().StopVoice(voice);
                }
            }
            if (standardDoor) {
                transitionCameraPosition = soft3d::Subtract(doorModel->GetPosition(),
                    soft3d::Scale(playerSpot.GetForward(), doorResourceIndex == 0 ? 0x40000 : 0xa0000));
                transitionCameraTarget = soft3d::Add(transitionCameraPosition,
                    soft3d::Scale(playerSpot.GetForward(), 0x20000));
                cameraController.SetCurrentTarget(transitionCameraTarget);
                transitionCameraSpeed = soft3d::FixedDivide(soft3d::Length(soft3d::Subtract(
                    transitionCameraPosition, cameraController.GetPosition())), 0x140000);
                const int32_t sound = unitManager->GetData().GetDoorResource(
                    static_cast<std::size_t>(doorResourceIndex)).openSoundIndex;
                if (sound > 0) {
                    engine.GetAudioManager().PlaySound(sound, false);
                }
            }
            transitionStage = 2;
        }
    } else if (transitionStage == 2) {
        if (standardDoor) {
            cameraController.MoveCamera(transitionCameraPosition, transitionCameraTarget,
                                        transitionCameraSpeed);
        }
        if (transitionTimer == 24) {
            if (restoreSavedState == 0) {
                player->SetPosition(spawnPosition);
                player->SetDirection(spawnDirection);
                playerSpot.SetForward(spawnDirection);
                player->SetCollisionBounds(TranslateBounds(player->GetLocalBounds(), spawnPosition));
            }
            specialDoorIndex = -1;
            fadeShader.StartFade(1, transitionColor, 4, 1);
            transitionStage = 3;
        }
    } else if (transitionStage == 3) {
        if (transitionTimer == 28) {
            player->SetVisible(false);
            cameraController.Bind(&playerSpot, false);
        } else if (transitionTimer == 31) {
            fadeShader.StartFade(0, transitionColor, 8, 1);
            transitionStage = 4;
            if (standardDoor) {
                const int32_t sound = unitManager->GetData().GetDoorResource(
                    static_cast<std::size_t>(doorResourceIndex)).closeSoundIndex;
                if (sound > 0) {
                    engine.GetAudioManager().PlaySound(sound, false);
                }
            }
        }
    } else if (transitionStage == 4 && transitionTimer == 36) {
        transitionStage = 0;
        restoreSavedState = 0;
        transitionTimer = 0;
    }
    if ((transitionStage == 2 || transitionStage == 3) && standardDoor) {
        doorModel->Update();
    }
    UpdateMapLoad();
}

void Scene::UpdateWorld()
{
    UpdateEnvironmentAudio();
    if (!unitManager->IsPaused()) {
        bsp->Update();
    }
    player->Update();
    unitManager->Update();
    cameraController.Update();
    if (cameraWarmupCounter == 2) {
        cameraController.SetInstant(false);
    }
    if (cameraWarmupCounter < 2) {
        ++cameraWarmupCounter;
    }
    engine.GetAudioManager().SetListenerPosition(useCameraListener == 0
        ? player->GetPosition() : cameraController.GetPosition());
}

void Scene::Update()
{
    if (!initialized) {
        return;
    }
    if (dayIntroPending != 0) {
        engine.GetRenderer().Clear(0);
        ++transitionTimer;
        if (transitionTimer == 56) {
            SetLoadMapParams(0, {}, {0, 0, soft3d::fixedOne}, PackRgb565(0, 0, 0));
            LoadNextLevel(0, 0);
            dayIntroPending = 0;
            transitionTimer = 0;
        }
        return;
    }
    if (nextLevelPending != 0) {
        LoadNextScript();
        return;
    }
    if (saveSlotPending != 0) {
        LoadSaveSlot(saveSlotPending);
        return;
    }

    if (transitionStage != 0) {
        ++transitionTimer;
        UpdateTransition();
        if (transitionStage > 0 && transitionStage < 5) {
            engine.GetInput().ClearKey();
            if (transitionStage != 4) {
                return;
            }
        }
    }
    UpdateWorld();
}

void Scene::RestartAfterGameOver()
{
    engine.GetRenderer().SelectShader(nullptr);
    inkShader.StopInk();
    inkShader.Reset();
    engine.GetInput().ClearKey();
    cameraController.SetFrozen(false);
    cameraWarmupCounter = 0;
    cameraController.SetInstant(true);
    engine.SetPreUpdateEnabled(true);
    for (int action = 1; action <= 7; ++action) {
        engine.GetControlMap().SetActionEnabled(action, false);
    }
    engine.GetScriptEngine().Reset();
    cameraController.Reset();
    unitManager->Reset();
    player->RestoreDayState();
    restoreSavedState = 0;
    eventFlags = dayStartSnapshot;
    RequireStatus(LoadDay(day), "Scene_RestartAfterGameOver");
    gameOverStage = 0;
}

void Scene::PostUpdate()
{
    if (!initialized || dayIntroPending != 0) {
        return;
    }
    if (transitionStage == 2 || transitionStage == 3) {
        if (transitionTimer < 29 && doorResourceIndex >= 0 &&
            (static_cast<uint32_t>(doorResourceIndex) & 0xffff0000U) != 0x10000U) {
            doorModel->PostUpdate();
        }
        return;
    }
    player->PostUpdate();
    if (player->IsDeathComplete() && gameOverStage == 0) {
        gameOverStage = 2;
    }
    if (gameOverStage == 3) {
        if (engine.GetInput().IsKeyClick(soft3d::Key::Number7) ||
            engine.GetInput().IsKeyClick(soft3d::Key::Accept)) {
            RestartAfterGameOver();
        }
        if (engine.GetInput().IsKeyClick(soft3d::Key::SoftRight) ||
            engine.GetInput().IsKeyClick(soft3d::Key::Number9)) {
            engine.SetPreUpdateEnabled(true);
            engine.ReturnToMainMenu();
            gameOverStage = 0;
        }
    }
    if (gameOverStage == 2) {
        if (inkShader.IsActive()) {
            ++inkTimer;
            if (inkTimer > 16 || (inkTimer > 8 && engine.GetInput().IsAnyKeyClick())) {
                engine.SetPreUpdateEnabled(false);
                gameOverStage = 3;
                return;
            }
        }
        if (inkShader.IsCompleted()) {
            engine.SetPreUpdateEnabled(false);
            gameOverStage = 3;
        } else if (!inkShader.IsActive()) {
            engine.GetAudioManager().StopAllSounds();
            auto* audio = static_cast<soft3d::AudioData*>(engine.GetResourceManager().GetResource(
                soft3d::ResourceAudio, 3, u"m_gameover", u".\\audio\\"));
            engine.GetAudioManager().PlaySound(audio, false);
            engine.GetScriptEngine().Unload();
            unitManager->SetPaused(true);
            engine.GetRenderer().SelectShader(&inkShader);
            inkShader.StartInk(&engine, 1, engine.GetVideo().GetBackBufferPtr(), 1);
            inkTimer = 0;
        }
    }
    unitManager->PostUpdateUnits();
}

void Scene::PrepareRender()
{
    if (!initialized || showDayTitle != 0) {
        return;
    }
    if (transitionStage >= 1 && transitionStage <= 3) {
        engine.GetRenderer().Clear(transitionColor);
        if ((transitionStage == 2 || transitionStage == 3) && transitionTimer < 29 &&
            doorResourceIndex >= 0 &&
            (static_cast<uint32_t>(doorResourceIndex) & 0xffff0000U) != 0x10000U) {
            doorModel->SubmitRenderables(engine.GetRenderQueues());
        } else if (transitionStage == 1 && doorResourceIndex < 0) {
            player->SubmitRenderables(engine.GetRenderQueues());
        }
        return;
    }
    soft3d::Camera& camera = engine.GetCamera();
    soft3d::BspRenderState state;
    state.cameraPosition = camera.GetPosition();
    state.worldToCamera = camera.GetWorldToCamera();
    state.projection = camera.GetProjection();
    state.screenScaleX = camera.GetScreenScaleX();
    state.screenScaleY = camera.GetScreenScaleY();
    state.screenCenterX = camera.GetWidth() << 15;
    state.screenCenterY = camera.GetHeight() << 15;
    state.inverseScreenScaleX = camera.GetInverseScreenScaleX();
    state.inverseScreenScaleY = camera.GetInverseScreenScaleY();
    bsp->PrepareRender(state);
    bsp->Render();
    bsp->SubmitVisibleEntities(engine.GetRenderQueues());
    player->SubmitRenderables(engine.GetRenderQueues());
    unitManager->SubmitRenderables(engine.GetRenderQueues());
}

void Scene::RenderDayTitle()
{
    engine.GetRenderer().Clear(0);
    char16_t dateName[32];
    char16_t numberName[32];
    soft3d::WideSprintf(dateName, u"riqi_%d", day);
    if (day == 7 && eventFlags[699] == 0) {
        soft3d::WideSprintf(numberName, u"%dnd_a", day);
    } else {
        soft3d::WideSprintf(numberName, u"%dnd", day);
    }
    const char16_t* directory = engine.GetLanguage() == 2 ? u".\\uien\\" : u".\\ui\\";
    auto* date = static_cast<soft3d::TextureRes*>(engine.GetResourceManager().GetResource(
        soft3d::ResourceTexture, 3, dateName, directory));
    auto* number = static_cast<soft3d::TextureRes*>(engine.GetResourceManager().GetResource(
        soft3d::ResourceTexture, 3, numberName, directory));
    if (date == nullptr || number == nullptr || date->GetTexture() == nullptr ||
        number->GetTexture() == nullptr) {
        return;
    }
    const soft3d::Texture* dateTexture = date->GetTexture();
    const soft3d::Texture* numberTexture = number->GetTexture();
    const int32_t dateWidth = static_cast<int32_t>(dateTexture->GetWidth());
    const int32_t dateHeight = static_cast<int32_t>(dateTexture->GetHeight());
    const int32_t numberWidth = static_cast<int32_t>(numberTexture->GetWidth());
    const int32_t y = (engine.GetHeight() - dateHeight) >> 1;
    engine.GetScreenBlitter().BlitUnscaled(dateTexture, (engine.GetWidth() - dateWidth) >> 1, y,
        {0, 0, dateWidth, dateHeight});
    engine.GetScreenBlitter().BlitUnscaled(numberTexture, (engine.GetWidth() - numberWidth) >> 1,
        y + dateHeight, {0, 0, numberWidth,
                          static_cast<int32_t>(numberTexture->GetHeight())});
}

void Scene::SubmitRenderables()
{
    if (initialized && showDayTitle != 0) {
        RenderDayTitle();
    }
}

void Scene::SelectPresentationShader(int selection)
{
    fadeShader.Complete();
    switch (selection) {
    case 1:
        engine.GetRenderer().SelectShader(&grayShader);
        break;
    case 2:
        engine.GetRenderer().SelectShader(&motionBlurShader);
        motionBlurShader.ResetHistory();
        break;
    case 7:
        engine.GetRenderer().SelectShader(&blindShader);
        break;
    case 8:
        engine.GetRenderer().SelectShader(&twistShader);
        break;
    default:
        engine.GetRenderer().SelectShader(nullptr);
        break;
    }
}

bool Scene::FadeCommand(bool fadeOut, uint16_t color, int frameCount)
{
    if (fadeShader.IsActive()) {
        if (!fadeShader.IsCompleted()) {
            return false;
        }
        fadeShader.Complete();
        return true;
    }
    fadeShader.StartFade(fadeOut ? 1 : 0, color, frameCount, 0);
    return false;
}

void Scene::BindCameraToPlayer()
{
    cameraController.Bind(&playerSpot, true);
}

void Scene::SetPlayerDirection(const soft3d::Vec3& direction)
{
    player->SetDirection(direction);
    playerSpot.SetForward(player->GetObject(0) != nullptr
        ? player->GetObject(0)->GetAxisZ() : direction);
}

void Scene::BindCameraToEnemy()
{
    enemySpot.SetPosition(playerSpot.GetPosition());
    enemySpot.SetRight(playerSpot.GetRight());
    enemySpot.SetUp(playerSpot.GetUp());
    enemySpot.SetForward(playerSpot.GetForward());
    cameraController.Bind(&enemySpot, true);
}

void Scene::SetEventFlag(int32_t index)
{
    if (index < 0 || static_cast<std::size_t>(index) >= eventFlags.size()) {
        throw std::out_of_range("Scene event flag index " + std::to_string(index));
    }
    eventFlags[static_cast<std::size_t>(index)] = 1;
}

bool Scene::GetEventFlag(int32_t index) const
{
    if (index < 0 || static_cast<std::size_t>(index) >= eventFlags.size()) {
        throw std::out_of_range("Scene event flag index " + std::to_string(index));
    }
    return eventFlags[static_cast<std::size_t>(index)] != 0;
}

void Scene::ShowQte(int32_t key, int32_t time, int32_t mode)
{
    qteUiKey = key - 1;
    qteUiTime = time;
    qteUiMode = mode;
    qteUiFailure = 0;
}

void Scene::FailQte()
{
    qteUiTime = 0;
    qteUiMode = 0;
    qteUiFailure = 1;
}

void Scene::SetHideMask(int32_t index, bool active)
{
    hideMaskIndex = index;
    hideMaskActive = active ? 1 : 0;
}

void Scene::ShowTutorial(int32_t id, int32_t frames)
{
    tutorialId = id;
    tutorialFrames = frames;
}

soft3d::BspEntity* Scene::CreateGameEntity(soft3d::BspEntityRecord* record, void* context,
                                            soft3d::BspEntityRecord*)
{
    if (record == nullptr || context == nullptr) {
        return nullptr;
    }
    auto* scene = static_cast<Scene*>(context);
    if (record->type != -1 && record->type != 0) {
        scene->unitManager->HandleMapEntity(*record, *scene->bsp);
        return nullptr;
    }
    auto entity = std::make_unique<BspUnit>();
    entity->LoadEntity(*record, *scene->bsp);
    return entity.release();
}

} // namespace game
