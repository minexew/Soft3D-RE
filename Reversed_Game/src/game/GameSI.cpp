#include "game/GameSI.hpp"

#include <sstream>

namespace game {
namespace {

constexpr std::array<uint32_t, gameSICallbackCount> armHandlers = {
    0x000653c0, 0x00065428, 0x00065490, 0x000654f8, 0x00065560, 0x000655c8,
    0x00065614, 0x00065660, 0x000656dc, 0x00065820, 0x00065964, 0x000659ac,
    0x00065b18, 0x00065b78, 0x00065bc0, 0x00065c10, 0x00065c64, 0x00065cd0,
    0x00065d08, 0x00065d3c, 0x00065d84, 0x00065e3c, 0x00065f94, 0x000660b4,
    0x00066444, 0x00066488, 0x00066504, 0x0006652c, 0x00066664, 0x000666c8,
    0x0006672c, 0x00066774, 0x000667d4, 0x00066860, 0x000668a8, 0x00066908,
    0x0006694c, 0x00066980, 0x000669c8, 0x000669fc, 0x00066a24, 0x00066a4c,
    0x00066ab0, 0x00066ae8, 0x00066b80, 0x00066bc8, 0x00066c14, 0x00066c48,
    0x00066d1c, 0x00066d50, 0x00066d94, 0x00066ddc, 0x00066e34, 0x00066e7c,
    0x00066edc, 0x00066f10, 0x00066f70, 0x00066fc8, 0x0006714c, 0x000671ac,
    0x000672e4, 0x00067404, 0x0006749c, 0x000674e4, 0x00067524, 0x00067560,
    0x0006759c, 0x000675d8, 0x00067614, 0x000676ac, 0x00067760, 0x000678ac,
    0x000678f8, 0x00067948, 0x000679a4, 0x00067a60, 0x00067ab8, 0x00067b60,
    0x00067ba4, 0x00067cb0, 0x00067da8, 0x00067e94, 0x00067fa8, 0x000680c0,
    0x00068174, 0x00068228, 0x00068280, 0x000682d0, 0x0006830c, 0x000683c4,
    0x000684ac, 0x000684f8, 0x00068540, 0x00068584, 0x000687d0, 0x00068814,
    0x00068894, 0x000688d0, 0x0006890c, 0x00068944, 0x00068978, 0x000689bc,
    0x00068a10, 0x00068a68, 0x00068af4, 0x00068b44, 0x00068b78, 0x00068cbc,
    0x00068d24, 0x00068d8c, 0x00068df0, 0x00068f34, 0x00068fa4, 0x00069200,
    0x00069268, 0x000692d0, 0x0006934c, 0x00069450, 0x000694b8, 0x00069578,
    0x000695f8, 0x00069660, 0x0006979c, 0x0006980c, 0x000699f8, 0x00069a9c,
    0x00069b0c, 0x00069c30, 0x00069ca0, 0x00069d10, 0x00069e34, 0x00069eec,
    0x0006a020, 0x0006a0cc, 0x0006a14c, 0x0006a1d8, 0x0006a248, 0x0006a298,
    0x0006a408, 0x0006a468, 0x0006a4f4, 0x0006a5cc, 0x0006a64c, 0x0006a6f8,
    0x0006a76c, 0x0006a834, 0x0006a8a8, 0x0006a920, 0x0006aa14, 0x0006ab0c,
    0x0006ac1c, 0x0006ad44, 0x0006ae9c, 0x0006af04, 0x0006afa0, 0x0006b00c,
    0x0006b0b4, 0x0006b138, 0x0006b1c8, 0x0006b298, 0x0006b33c, 0x0006b3bc,
    0x0006b4c0, 0x0006b5e8, 0x0006b710, 0x0006b7bc, 0x0006b884, 0x0006b9c8,
    0x0006ba48, 0x0006baac, 0x0006bb34, 0x0006bbb4, 0x0006bc50, 0x0006bcec,
    0x0006bd78, 0x0006be7c, 0x0006becc, 0x0006c03c, 0x0006c08c, 0x0006c31c,
    0x0006c3e8, 0x0006c458, 0x0006c4c0, 0x0006c564, 0x0006c5cc, 0x0006c63c,
    0x0006c718, 0x0006c788, 0x0006c7ec, 0x0006c85c, 0x0006c8c0, 0x0006c924,
    0x0006c9e0, 0x0006ca54, 0x0006cac4, 0x0006cb34, 0x0006cbec, 0x0006cca4,
    0x0006cd5c, 0x0006ce18, 0x0006cf08, 0x0006cfec, 0x0006d06c, 0x0006d0dc,
    0x0006d12c, 0x0006d198, 0x0006d208, 0x0006d350, 0x0006d498, 0x0006d508,
    0x0006d574, 0x0006d600, 0x0006d988, 0x0006d9d4, 0x0006da2c, 0x00064fd0,
    0x00064ff8, 0x00065034,
};

uint16_t PackRgb565(int16_t red, int16_t green, int16_t blue)
{
    const uint32_t r = static_cast<uint16_t>(red);
    const uint32_t g = static_cast<uint16_t>(green);
    const uint32_t b = static_cast<uint16_t>(blue);
    return static_cast<uint16_t>(((r & 0xff) >> 3) << 11 |
                                 ((g & 0xff) >> 2) << 5 | ((b & 0xff) >> 3));
}

} // namespace

GameSIError::GameSIError(int id, uint32_t address, const std::string& reason)
    : std::runtime_error("GameSI command " + std::to_string(id) + " at ARM 0x" +
                         [&address] {
                             std::ostringstream stream;
                             stream << std::hex << address;
                             return stream.str();
                         }() + ": " + reason)
    , commandId(id)
    , armAddress(address)
{
}

GameSI::GameSI()
{
    for (std::size_t index = 0; index < callbacks.size(); ++index) {
        callbacks[index] = {armHandlers[index], &GameSI::Unimplemented, 0};
    }
    callbacks[53 - gameSIFirstCommand].handler = &GameSI::LoadNextDay;
    callbacks[54 - gameSIFirstCommand].handler = &GameSI::SetLoadMapParams;
    callbacks[55 - gameSIFirstCommand].handler = &GameSI::LoadNextLevel;
    callbacks[56 - gameSIFirstCommand].handler = &GameSI::LoadMap;
    callbacks[57 - gameSIFirstCommand].handler = &GameSI::IsMapLoaded;
    callbacks[58 - gameSIFirstCommand].handler = &GameSI::SetEventFlag;
    callbacks[59 - gameSIFirstCommand].handler = &GameSI::GetEventFlag;
    callbacks[69 - gameSIFirstCommand].handler = &GameSI::NullCallback;
    callbacks[258 - gameSIFirstCommand].handler = &GameSI::NullCallback;
    callbacks[259 - gameSIFirstCommand].handler = &GameSI::SetSharedWord255;
    callbacks[260 - gameSIFirstCommand].handler = &GameSI::NullCallback;
}

void GameSI::Bind(const GameSIContext& value)
{
    if (value.scene == nullptr || value.unitManager == nullptr || value.cameraController == nullptr ||
        value.player == nullptr || value.gameUi == nullptr || value.audio == nullptr ||
        value.resources == nullptr || value.bsp == nullptr || value.controlMap == nullptr) {
        throw std::invalid_argument("GameSI requires the complete scene context");
    }
    context = value;
}

const GameSI::Callback& GameSI::GetCallback(int commandId) const
{
    if (commandId < gameSIFirstCommand || commandId > gameSILastCommand) {
        throw std::out_of_range("GameSI callback ID " + std::to_string(commandId));
    }
    return callbacks[static_cast<std::size_t>(commandId - gameSIFirstCommand)];
}

uint32_t GameSI::GetArmHandlerAddress(int commandId)
{
    if (commandId < gameSIFirstCommand || commandId > gameSILastCommand) {
        throw std::out_of_range("GameSI ARM handler ID " + std::to_string(commandId));
    }
    return armHandlers[static_cast<std::size_t>(commandId - gameSIFirstCommand)];
}

void GameSI::RequireArguments(const soft3d::ScriptCommand& command, std::size_t count) const
{
    if (command.GetArgumentCount() < count) {
        throw GameSIError(command.GetId(), GetArmHandlerAddress(command.GetId()),
                          "expected at least " + std::to_string(count) + " arguments, got " +
                              std::to_string(command.GetArgumentCount()));
    }
}

void GameSI::RequireContext() const
{
    if (context.scene == nullptr || context.unitManager == nullptr ||
        context.cameraController == nullptr || context.player == nullptr || context.gameUi == nullptr ||
        context.audio == nullptr || context.resources == nullptr || context.bsp == nullptr ||
        context.controlMap == nullptr) {
        throw std::runtime_error("GameSI context is not bound");
    }
}

void GameSI::ExecuteGameCommand(soft3d::ScriptCommand& command)
{
    RequireContext();
    const Callback& callback = GetCallback(command.GetId());
    (this->*callback.handler)(command);
}

void GameSI::Unimplemented(soft3d::ScriptCommand& command)
{
    throw GameSIError(command.GetId(), GetArmHandlerAddress(command.GetId()),
                      "ARM handler is not implemented in the portable reconstruction yet");
}

void GameSI::LoadNextDay(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 0);
    const soft3d::result status = context.scene->LoadDay(context.scene->GetDay() + 1);
    if (status != soft3d::S3D_OK) {
        throw GameSIError(command.GetId(), GetArmHandlerAddress(command.GetId()),
                          "LoadDay failed with Soft3D result " + std::to_string(status));
    }
}

void GameSI::SetLoadMapParams(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 10);
    const soft3d::Vec3 position{command.GetArgument(1).GetFixed(),
                                command.GetArgument(2).GetFixed(),
                                command.GetArgument(3).GetFixed()};
    const soft3d::Vec3 direction{command.GetArgument(4).GetFixed(),
                                 command.GetArgument(5).GetFixed(),
                                 command.GetArgument(6).GetFixed()};
    const uint16_t color = PackRgb565(command.GetArgument(7).GetShort(),
                                      command.GetArgument(8).GetShort(),
                                      command.GetArgument(9).GetShort());
    context.scene->SetLoadMapParams(command.GetArgument(0).GetInt(), position, direction, color);
}

void GameSI::LoadNextLevel(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 2);
    context.scene->LoadNextLevel(command.GetArgument(0).GetShort(),
                                 command.GetArgument(1).GetShort());
}

void GameSI::LoadMap(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 1);
    context.scene->LoadMap(command.GetArgument(0).GetShort());
}

void GameSI::IsMapLoaded(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 0);
    if (command.GetOutput() != nullptr) {
        command.GetOutput()->SetBool(context.scene->IsMapLoaded());
    }
}

void GameSI::SetEventFlag(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 1);
    context.scene->SetEventFlag(command.GetArgument(0).GetShort());
}

void GameSI::GetEventFlag(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 1);
    if (command.GetOutput() != nullptr) {
        command.GetOutput()->SetBool(context.scene->GetEventFlag(command.GetArgument(0).GetShort()));
    }
}

void GameSI::SetSharedWord255(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 0);
    if (script == nullptr) {
        throw GameSIError(command.GetId(), GetArmHandlerAddress(command.GetId()),
                          "script back-pointer is not bound");
    }
    script->SetSharedVariable(255, 1);
}

void GameSI::NullCallback(soft3d::ScriptCommand&)
{
}

} // namespace game
