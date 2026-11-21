#include "game/Scene.hpp"

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

} // namespace

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

    player = std::make_unique<soft3d::Creature>(&engine.GetResourceManager(), 2,
                                                 &engine.GetRenderer(), &engine.GetAudioManager(),
                                                 bsp.get());
    player->SetUnitType(9);
    player->SetUnitManager(unitManager.get());
    status = player->LoadObjects(u"r_ken_a", nullptr, 1);
    if (status != soft3d::S3D_OK) {
        return status;
    }
    status = player->LoadAlternateObject(u"r_ken_shadow_a");
    if (status != soft3d::S3D_OK) {
        return status;
    }
    player->EnableShadow(true);
    player->SetDirection(spawnDirection);

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
    player->SetPosition({});
    player->SetDirection({0, 0, soft3d::fixedOne});
    player->SetVelocity({});
    player->SetVisible(true);
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
    UnloadLevel();
    engine.GetScriptEngine().Unload();
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
    engine.GetAudioManager().PlaySound(u"e_wind01", true);
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
    soft3d::result status = player->LoadObjects(u"r_ken_a", nullptr, 1);
    if (status != soft3d::S3D_OK) {
        return status;
    }
    status = player->LoadAlternateObject(u"r_ken_shadow_a");
    if (status != soft3d::S3D_OK) {
        return status;
    }
    status = unitManager->LoadDayContent(day, engine);
    if (status != soft3d::S3D_OK) {
        return status;
    }
    engine.GetScriptEngine().ClearSharedVariables();
    dayStartSnapshot = eventFlags;
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
    engine.GetRenderer().SetLightingEnabled(true);
    engine.GetAudioManager().SetEffectsVolume(255);
    engine.GetAudioManager().SetMusicVolume(255);
    if (!fadeShader.IsActive()) {
        fadeShader.StartFade(1, transitionColor, 8, 1);
    }
    transitionStage = 1;
    transitionTimer = 0;
    mapProgress = 0;
    useDay1Fallback = 0;
}

void Scene::FinishMapLoad()
{
    player->SnapToGround();
    player->SetVisible(true);
    unitManager->FinishMapLoad(restoreSavedState != 0);
    engine.GetInput().ClearKey();
    cameraWarmupCounter = 0;
    engine.GetScriptEngine().StartTasks();
}

void Scene::UnloadLevel()
{
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

void Scene::UpdateWorld()
{
    if (!unitManager->IsPaused()) {
        bsp->Update();
    }
    player->Update();
    unitManager->UpdateUnits();
    engine.GetAudioManager().SetListenerPosition(useCameraListener == 0
        ? player->GetPosition() : engine.GetCamera().GetPosition());
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
        if (transitionStage == 1) {
            showDayTitle = 0;
            if (transitionTimer == 8) {
                transitionStage = 2;
            }
        } else if (transitionStage == 2) {
            if (transitionTimer == 24) {
                if (restoreSavedState == 0) {
                    player->SetPosition(spawnPosition);
                    player->SetDirection(spawnDirection);
                }
                transitionStage = 3;
            }
        } else if (transitionStage == 3) {
            if (transitionTimer == 28) {
                player->SetVisible(false);
            } else if (transitionTimer == 31) {
                transitionStage = 4;
            }
        } else if (transitionStage == 4 && transitionTimer == 36) {
            transitionStage = 0;
            restoreSavedState = 0;
            transitionTimer = 0;
        }
        UpdateMapLoad();
        if (transitionStage > 0 && transitionStage < 4) {
            engine.GetInput().ClearKey();
            return;
        }
    }
    UpdateWorld();
}

void Scene::PostUpdate()
{
    if (!initialized || dayIntroPending != 0) {
        return;
    }
    if (transitionStage == 2 || transitionStage == 3) {
        return;
    }
    player->PostUpdate();
    unitManager->PostUpdateUnits();
}

void Scene::PrepareRender()
{
    if (!initialized || showDayTitle != 0) {
        return;
    }
    if (transitionStage >= 1 && transitionStage <= 3) {
        engine.GetRenderer().Clear(transitionColor);
        if (doorModel != nullptr && doorResourceIndex >= 0 &&
            (static_cast<uint32_t>(doorResourceIndex) & 0xffff0000U) != 0x10000U) {
            doorModel->SubmitRenderables(engine.GetRenderQueues());
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
    unitManager->SubmitUnits(engine.GetRenderQueues());
}

void Scene::RenderDayTitle()
{
    engine.GetRenderer().Clear(0);
    char16_t dateName[32];
    char16_t numberName[32];
    soft3d::WideSprintf(dateName, u"riqi_%d", day);
    soft3d::WideSprintf(numberName, u"%dnd", day);
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

soft3d::BspEntity* Scene::CreateGameEntity(soft3d::BspEntityRecord* record, void* context,
                                            soft3d::BspEntityRecord*)
{
    if (record == nullptr || context == nullptr) {
        return nullptr;
    }
    if (record->type != -1 && record->type != 0) {
        return nullptr;
    }
    auto* scene = static_cast<Scene*>(context);
    auto entity = std::make_unique<soft3d::BspEntity>();
    if (record->modelIndex >= 0 &&
        static_cast<std::size_t>(record->modelIndex) < scene->bsp->GetModels().size()) {
        entity->SetObject(scene->bsp->GetModels()[record->modelIndex].object.get());
    }
    return entity.release();
}

} // namespace game
