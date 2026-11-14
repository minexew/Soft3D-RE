#include "soft3d/MenuBackdrop.hpp"

#include "soft3d/Engine.hpp"

namespace soft3d {

MenuBackdrop::MenuBackdrop(Engine* value)
    : engine(value)
    , bsp(value)
    , paths(64)
{
    bsp.SetEntityFactory(CreateEntity, this);
}

result MenuBackdrop::Load()
{
    loaded = false;
    s3dFilePak* filePak = engine->GetResourceManager().GetFilePak();
    result status = LoadS3dStyleResource(&bsp, u".\\common\\default.s3dsty", filePak);
    if (status != S3D_OK) {
        return status;
    }
    status = LoadSplCameraResource(paths, u".\\common\\1000.spl", filePak);
    if (status != S3D_OK) {
        return status;
    }
    int progress = 0;
    while (progress != 100) {
        status = LoadSbpMapChunks(&bsp, u".\\common\\", u"menu.sbp", &progress);
        if (status != S3D_OK) {
            return status;
        }
    }
    cameraSpot.SetPosition(paths.EvaluatePosition(0));
    cameraSpot.SetForward(Normalize(paths.EvaluateDerivative(0)));
    cameraSpot.SetCameraController(&cameraController);
    cameraController.Start(paths, &cameraSpot, 0x1e0000, false, false);
    loaded = true;
    return S3D_OK;
}

void MenuBackdrop::Update()
{
    if (loaded) {
        cameraController.Update(engine->GetCamera());
    }
}

void MenuBackdrop::PrepareRender()
{
    if (!loaded) {
        return;
    }
    Camera& camera = engine->GetCamera();
    BspRenderState state;
    state.cameraPosition = camera.GetPosition();
    state.worldToCamera = camera.GetWorldToCamera();
    state.screenScaleX = camera.GetScreenScaleX();
    state.screenScaleY = camera.GetScreenScaleY();
    state.screenCenterX = camera.GetWidth() << 15;
    state.screenCenterY = camera.GetHeight() << 15;
    state.inverseScreenScaleX = camera.GetInverseScreenScaleX();
    state.inverseScreenScaleY = camera.GetInverseScreenScaleY();
    bsp.PrepareRender(state);
    bsp.Render();
    bsp.SubmitVisibleEntities(engine->GetRenderQueues());
}

bool MenuBackdrop::IsLoaded() const { return loaded; }
Bsp& MenuBackdrop::GetBsp() { return bsp; }
CameraPathCollection& MenuBackdrop::GetPaths() { return paths; }
CameraPathController& MenuBackdrop::GetCameraController() { return cameraController; }
DummySpot& MenuBackdrop::GetCameraSpot() { return cameraSpot; }

BspEntity* MenuBackdrop::CreateEntity(BspEntityRecord* record, void* context, BspEntityRecord*)
{
    auto* backdrop = static_cast<MenuBackdrop*>(context);
    auto* entity = new BspEntity;
    if (record->modelIndex >= 0) {
        entity->SetObject(backdrop->bsp.GetModels()[record->modelIndex].object.get());
    }
    return entity;
}

} // namespace soft3d
