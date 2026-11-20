#include "soft3d/SimpleScript.hpp"

#include "soft3d/s3dIO.hpp"

#include <algorithm>
#include <cstdio>

namespace soft3d {
namespace {

constexpr size_t maxScriptFunctions = 256;
constexpr size_t maxScriptCommands = 1600;
constexpr size_t maxScriptVariables = 3000;
constexpr uint32_t referenceResultMarker = 6;

int32_t WrapAdd(int32_t left, int32_t right)
{
    return static_cast<int32_t>(static_cast<uint32_t>(left) + static_cast<uint32_t>(right));
}

int32_t WrapSubtract(int32_t left, int32_t right)
{
    return static_cast<int32_t>(static_cast<uint32_t>(left) - static_cast<uint32_t>(right));
}

int32_t WrapMultiply(int32_t left, int32_t right)
{
    const uint64_t product = static_cast<uint64_t>(static_cast<int64_t>(left) *
                                                  static_cast<int64_t>(right));
    return static_cast<int32_t>(static_cast<uint32_t>(product));
}

int32_t WrapDivide(int32_t left, int32_t right)
{
    if (right == 0) {
        return 0;
    }
    return static_cast<int32_t>(static_cast<uint32_t>(
        static_cast<int32_t>(static_cast<int64_t>(left) / right)));
}

int32_t WrapModulo(int32_t left, int32_t right)
{
    if (right == 0) {
        return 0;
    }
    return static_cast<int32_t>(static_cast<uint32_t>(
        static_cast<int32_t>(static_cast<int64_t>(left) % right)));
}

int CompareStrings(const std::u16string& left, const std::u16string& right)
{
    const size_t count = std::min(left.size(), right.size());
    for (size_t index = 0; index < count; ++index) {
        if (left[index] != right[index]) {
            return left[index] < right[index] ? -1 : 1;
        }
    }
    if (left.size() == right.size()) {
        return 0;
    }
    return left.size() < right.size() ? -1 : 1;
}

void Append16(std::vector<uint8_t>& output, uint16_t value)
{
    output.push_back(static_cast<uint8_t>(value));
    output.push_back(static_cast<uint8_t>(value >> 8));
}

void Append32(std::vector<uint8_t>& output, uint32_t value)
{
    Append16(output, static_cast<uint16_t>(value));
    Append16(output, static_cast<uint16_t>(value >> 16));
}

class ArchiveReader {
public:
    ArchiveReader(const uint8_t* data, size_t size)
        : data(data), size(size)
    {
    }

    bool Read16(uint16_t& value)
    {
        if (position + 2 > size) {
            return false;
        }
        value = static_cast<uint16_t>(data[position]) |
                static_cast<uint16_t>(data[position + 1]) << 8;
        position += 2;
        return true;
    }

    bool Read32(uint32_t& value)
    {
        uint16_t low = 0;
        uint16_t high = 0;
        if (!Read16(low) || !Read16(high)) {
            return false;
        }
        value = low | static_cast<uint32_t>(high) << 16;
        return true;
    }

    bool Skip(size_t bytes)
    {
        if (bytes > size - position) {
            return false;
        }
        position += bytes;
        return true;
    }

    const uint8_t* data;
    size_t size;
    size_t position = 0;
};

ScriptCommand* FindOrdinalInCommand(ScriptCommand* command, uint32_t ordinal)
{
    if (command == nullptr) {
        return nullptr;
    }
    if (command->GetOrdinal() == ordinal) {
        return command;
    }
    for (ScriptCommand* child : command->GetBody()) {
        if (ScriptCommand* found = FindOrdinalInCommand(child, ordinal)) {
            return found;
        }
    }
    return nullptr;
}

} // namespace

ScriptValue::ScriptValue(ScriptValueType valueType)
    : type(valueType)
{
}

void ScriptValue::Assign(const ScriptValue& source)
{
    number = source.number;
    string = source.string;
}

void ScriptValue::Clear()
{
    number = 0;
    string.clear();
}

ScriptValue& ScriptCommand::GetArgument(uint32_t index)
{
    Argument& argument = arguments[index];
    if (argument.value.GetType() == ScriptValueType::Reference) {
        ScriptValue* resolved = script->ResolveReference(argument.reference.scope,
                                                         argument.reference.index, function);
        if (resolved != nullptr) {
            return *resolved;
        }
    }
    return argument.value;
}

const ScriptValue& ScriptCommand::GetArgument(uint32_t index) const
{
    return const_cast<ScriptCommand*>(this)->GetArgument(index);
}

ScriptValue* ScriptCommand::GetOutput() const
{
    if (!hasOutput) {
        return nullptr;
    }
    return script->ResolveReference(outputReference.scope, outputReference.index, function);
}

void ScriptInterface::Execute(ScriptCommand& command)
{
    if (script == nullptr) {
        return;
    }
    switch (command.GetId()) {
    case 36:
        script->LoadScript(command.GetArgument(0).GetString());
        break;
    case 37:
        script->EnableTask(command.GetArgument(0).GetShort());
        break;
    case 38:
        script->DisableTask(command.GetArgument(0).GetShort());
        break;
    case 39:
        script->RunTask(command.GetArgument(0).GetShort());
        break;
    case 40:
        script->InterruptTask(command.GetArgument(0).GetShort());
        break;
    case 41:
        script->SetSharedVariable(command.GetArgument(0).GetShort(),
                                  static_cast<uint32_t>(command.GetArgument(1).GetInt()));
        break;
    case 42:
        if (command.GetOutput() != nullptr) {
            command.GetOutput()->SetInt(static_cast<int32_t>(
                script->GetSharedVariable(command.GetArgument(0).GetShort())));
        }
        break;
    default:
        ExecuteGameCommand(command);
        break;
    }
}

class SimpleScript::Parser {
public:
    Parser(SimpleScript& script, const uint8_t* data, size_t size)
        : script(script), data(data), size(size)
    {
    }

    result Run()
    {
        const uint16_t globalCount = Read16();
        for (uint16_t index = 0; index < globalCount; ++index) {
            const uint16_t type = Read16();
            if (!ValidVariableType(type)) {
                return S3D_FORMAT;
            }
            ScriptValue* value = script.AllocateVariable(static_cast<ScriptValueType>(type));
            if (value == nullptr) {
                return S3D_MEMORY;
            }
            script.globals.push_back(value);
        }
        hasInitializer = Read16() != 0;
        if (hasInitializer) {
            script.initializer = ReadFunction();
            if (failed) {
                return S3D_FORMAT;
            }
        }
        const uint16_t functionCount = Read16();
        if (functionCount > maxScriptFunctions) {
            return S3D_FORMAT;
        }
        for (uint16_t index = 0; index < functionCount; ++index) {
            std::unique_ptr<ScriptFunction> function = ReadFunction();
            if (failed) {
                return S3D_FORMAT;
            }
            LinkFunctions(*function);
            script.functions.push_back(std::move(function));
        }
        if (hasInitializer) {
            LinkFunctions(*script.initializer);
        }
        return failed || position != size ? S3D_FORMAT : S3D_OK;
    }

private:
    uint16_t Read16()
    {
        if (position + 2 > size) {
            failed = true;
            return 0;
        }
        const uint16_t value = static_cast<uint16_t>(data[position]) |
                               static_cast<uint16_t>(data[position + 1]) << 8;
        position += 2;
        return value;
    }

    int16_t ReadSigned16()
    {
        return static_cast<int16_t>(Read16());
    }

    static bool ValidVariableType(uint16_t type)
    {
        return type <= static_cast<uint16_t>(ScriptValueType::Fixed);
    }

    std::unique_ptr<ScriptFunction> ReadFunction()
    {
        auto function = std::make_unique<ScriptFunction>();
        function->script = &script;
        currentFunction = function.get();
        script.commandCounter = 0;

        const uint16_t localCount = Read16();
        for (uint16_t index = 0; index < localCount && !failed; ++index) {
            const uint16_t type = Read16();
            if (!ValidVariableType(type)) {
                failed = true;
                break;
            }
            ScriptValue* value = script.AllocateVariable(static_cast<ScriptValueType>(type));
            if (value == nullptr) {
                failed = true;
                break;
            }
            function->locals.push_back(value);
        }
        script.currentLocals = &function->locals;
        ReadCommandList(function->entryCommands, true);
        ReadCommandList(function->commandList, true);
        return function;
    }

    void ReadCommandList(std::vector<ScriptCommand*>& list, bool linkSiblings)
    {
        const uint16_t count = Read16();
        for (uint16_t index = 0; index < count && !failed; ++index) {
            list.push_back(ReadCommand());
        }
        if (linkSiblings) {
            for (size_t index = 0; index + 1 < list.size(); ++index) {
                list[index]->next = list[index + 1];
            }
        }
    }

    ScriptCommand* ReadCommand()
    {
        ScriptCommand* command = script.AllocateCommand();
        if (failed || command == nullptr) {
            failed = true;
            return command;
        }
        command->id = Read16();
        command->type = Read16();
        command->argumentCount = Read16();
        command->function = currentFunction;
        command->script = &script;
        if (command->argumentCount > command->arguments.size()) {
            failed = true;
            return command;
        }
        for (uint16_t index = 0; index < command->argumentCount && !failed; ++index) {
            ReadArgument(command->arguments[index]);
        }
        if (Read16() == referenceResultMarker) {
            Read16();
            command->outputReference.index = Read16();
            command->outputReference.scope = Read16();
            command->hasOutput = true;
            if (script.ResolveReference(command->outputReference.scope,
                                        command->outputReference.index, currentFunction) == nullptr) {
                failed = true;
                return command;
            }
        }
        ReadCommandList(command->body, true);
        ReadCommandList(command->conditions, false);
        command->ordinal = script.commandCounter++;
        return command;
    }

    void ReadArgument(ScriptCommand::Argument& argument)
    {
        const int16_t rawType = ReadSigned16();
        const int16_t rawSize = ReadSigned16();
        if (rawType < 0 || rawType > static_cast<int16_t>(ScriptValueType::Reference) ||
            rawSize < 0) {
            failed = true;
            return;
        }
        const ScriptValueType type = static_cast<ScriptValueType>(rawType);
        const size_t bytes = static_cast<size_t>(rawSize);
        if (position + bytes > size ||
            (type != ScriptValueType::String && bytes != sizeof(uint32_t))) {
            failed = true;
            return;
        }
        argument.value = ScriptValue(type);
        if (type == ScriptValueType::String) {
            std::u16string string;
            for (size_t index = 0; index < bytes && data[position + index] != 0; ++index) {
                string.push_back(data[position + index]);
            }
            argument.value.SetString(std::move(string));
        } else if (type == ScriptValueType::Reference) {
            const uint32_t packed = Read32At(position);
            argument.reference.scope = static_cast<uint16_t>(packed >> 16);
            argument.reference.index = static_cast<uint16_t>(packed);
            if (argument.reference.scope > 2 ||
                script.ResolveReference(argument.reference.scope, argument.reference.index,
                                        currentFunction) == nullptr) {
                failed = true;
                return;
            }
        } else {
            argument.value.SetInt(static_cast<int32_t>(Read32At(position)));
        }
        position += bytes & ~size_t(1);
    }

    uint32_t Read32At(size_t offset) const
    {
        return static_cast<uint32_t>(data[offset]) |
               static_cast<uint32_t>(data[offset + 1]) << 8 |
               static_cast<uint32_t>(data[offset + 2]) << 16 |
               static_cast<uint32_t>(data[offset + 3]) << 24;
    }

    void LinkFunctions(ScriptFunction& function)
    {
        for (ScriptCommand* command : function.entryCommands) {
            script.LinkCommand(command);
        }
        for (ScriptCommand* command : function.commandList) {
            script.LinkCommand(command);
        }
    }

    SimpleScript& script;
    const uint8_t* data;
    size_t size;
    size_t position = 0;
    ScriptFunction* currentFunction = nullptr;
    bool hasInitializer = false;
    bool failed = false;
};

SimpleScript::SimpleScript() = default;
SimpleScript::~SimpleScript() = default;

result SimpleScript::Init()
{
    return S3D_OK;
}

result SimpleScript::LoadFromPak(const char16_t* baseName, s3dFilePak* filePak)
{
    Unload();
    archive = filePak;
    name = baseName == nullptr ? std::u16string() : std::u16string(baseName);
    if (archive == nullptr) {
        return S3D_PARAM;
    }
    const std::u16string path = name + u".sst";
    if (archive->Select(path.c_str()) != S3D_OK) {
        return S3D_FILE;
    }
    if (archive->Seek(0, SEEK_END) != S3D_OK) {
        return S3D_FILE;
    }
    const int length = archive->Tell();
    if (length == S3D_EOF) {
        return S3D_UNKNOWN;
    }
    std::vector<uint8_t> bytes(static_cast<size_t>(length));
    archive->Seek(0, SEEK_SET);
    if (!bytes.empty() && archive->Read(bytes.data(), length) != S3D_OK) {
        return S3D_FILE;
    }
    Parser parser(*this, bytes.data(), bytes.size());
    const result status = parser.Run();
    if (status != S3D_OK) {
        Unload();
        return status;
    }
    state = ScriptState::Loaded;
    previousState = ScriptState::Loaded;
    return S3D_OK;
}

result SimpleScript::Load(const uint8_t* data, size_t size)
{
    Unload();
    if (data == nullptr && size != 0) {
        return S3D_PARAM;
    }
    Parser parser(*this, data, size);
    const result status = parser.Run();
    if (status != S3D_OK) {
        Unload();
        return status;
    }
    state = ScriptState::Loaded;
    previousState = ScriptState::Loaded;
    return S3D_OK;
}

size_t SimpleScript::GetSerializedValueSize(const ScriptValue& value)
{
    const size_t payload = value.GetType() == ScriptValueType::String
        ? (value.GetString().size() + 1) * sizeof(char16_t) : sizeof(uint32_t);
    return (payload + 7) & ~size_t(3);
}

void SimpleScript::WriteSerializedValue(std::vector<uint8_t>& output, const ScriptValue& value)
{
    const size_t recordStart = output.size();
    const size_t recordSize = GetSerializedValueSize(value);
    if (value.GetType() == ScriptValueType::String) {
        const std::u16string& string = value.GetString();
        Append32(output, static_cast<uint32_t>((string.size() + 1) * sizeof(char16_t)));
        for (char16_t character : string) {
            Append16(output, character);
        }
        Append16(output, 0);
    } else {
        Append32(output, sizeof(uint32_t));
        Append32(output, static_cast<uint32_t>(value.GetInt()));
    }
    output.resize(recordStart + recordSize, 0);
}

size_t SimpleScript::GetArchiveSize() const
{
    size_t result = 0x440;
    const auto addValue = [&result](const ScriptValue* value) {
        result += GetSerializedValueSize(*value);
    };
    for (const ScriptValue* value : scope0Variables) {
        addValue(value);
    }
    for (const ScriptValue* value : globals) {
        addValue(value);
    }
    for (const auto& function : functions) {
        result += 0x10;
        for (const ScriptValue* value : function->locals) {
            addValue(value);
        }
    }
    return result;
}

ScriptCommand* SimpleScript::FindCommandByOrdinal(const ScriptFunction& function, uint32_t ordinal)
{
    for (ScriptCommand* command : function.commandList) {
        if (ScriptCommand* found = FindOrdinalInCommand(command, ordinal)) {
            return found;
        }
    }
    return nullptr;
}

result SimpleScript::SaveArchive(const char16_t* path)
{
    if (path == nullptr) {
        return S3D_PARAM;
    }
    std::vector<uint8_t> output;
    output.reserve(GetArchiveSize());
    for (size_t index = 0; index < 32; ++index) {
        Append16(output, index < name.size() ? name[index] : 0);
    }
    for (uint32_t value : shared) {
        Append32(output, value);
    }
    const auto writeValue = [&output](const ScriptValue* value) {
        WriteSerializedValue(output, *value);
    };
    for (const ScriptValue* value : scope0Variables) {
        writeValue(value);
    }
    for (const ScriptValue* value : globals) {
        writeValue(value);
    }
    for (const auto& function : functions) {
        Append32(output, static_cast<uint32_t>(function->state));
        Append32(output, function->currentCommand != nullptr
            ? function->currentCommand->GetOrdinal() : 0);
        Append32(output, function->calledTask);
        Append32(output, function->calledCommand != nullptr
            ? function->calledCommand->GetOrdinal() : 0);
        for (const ScriptValue* value : function->locals) {
            writeValue(value);
        }
    }

    s3dIO file;
    if (!file.Open(path, u"wb")) {
        return S3D_FILE;
    }
    const int status = output.empty() ? 0 : file.Write(output.data(),
        static_cast<int>(output.size()), 1);
    file.Close();
    return status == 0 ? S3D_OK : S3D_FILE;
}

result SimpleScript::LoadArchive(const char16_t* path, s3dFilePak* resourceArchive)
{
    if (path == nullptr || resourceArchive == nullptr) {
        return S3D_PARAM;
    }
    s3dIO file;
    if (!file.Open(path, u"rb")) {
        return S3D_FILE;
    }
    file.Seek(0, SEEK_END);
    const uint32_t byteCount = file.Tell();
    file.Seek(0, SEEK_SET);
    std::vector<uint8_t> data(byteCount);
    const uint32_t read = data.empty() ? 0 : file.Read(data.data(), byteCount);
    file.Close();
    if (read != data.size()) {
        return S3D_FILE;
    }

    ArchiveReader reader(data.data(), data.size());
    std::u16string archiveName;
    for (size_t index = 0; index < 32; ++index) {
        uint16_t character = 0;
        if (!reader.Read16(character)) {
            return S3D_FORMAT;
        }
        if (character != 0) {
            archiveName.push_back(character);
        }
    }
    std::array<uint32_t, 256> loadedShared{};
    for (uint32_t& value : loadedShared) {
        if (!reader.Read32(value)) {
            return S3D_FORMAT;
        }
    }
    const result loadStatus = LoadFromPak(archiveName.c_str(), resourceArchive);
    if (loadStatus != S3D_OK) {
        return loadStatus;
    }
    shared = loadedShared;

    const auto readValue = [&reader](ScriptValue& value) {
        uint32_t payloadSize = 0;
        if (!reader.Read32(payloadSize)) {
            return false;
        }
        const size_t payloadStart = reader.position;
        const size_t paddedSize = (static_cast<size_t>(payloadSize) + 3) & ~size_t(3);
        if (paddedSize > reader.size - payloadStart) {
            return false;
        }
        if (value.GetType() == ScriptValueType::String) {
            if (payloadSize < 2 || (payloadSize & 1) != 0) {
                return false;
            }
            std::u16string string;
            for (size_t offset = 0; offset < payloadSize; offset += 2) {
                uint16_t character = 0;
                if (!reader.Read16(character)) {
                    return false;
                }
                if (character != 0) {
                    string.push_back(character);
                }
            }
            value.SetString(std::move(string));
        } else {
            uint32_t number = 0;
            if (payloadSize != sizeof(uint32_t) || !reader.Read32(number)) {
                return false;
            }
            value.SetInt(static_cast<int32_t>(number));
        }
        reader.position = payloadStart + paddedSize;
        return true;
    };

    for (ScriptValue* value : scope0Variables) {
        if (!readValue(*value)) {
            return S3D_FORMAT;
        }
    }
    for (ScriptValue* value : globals) {
        if (!readValue(*value)) {
            return S3D_FORMAT;
        }
    }
    for (const auto& function : functions) {
        uint32_t taskState = 0;
        uint32_t currentOrdinal = 0;
        uint32_t calledOrdinal = 0;
        if (!reader.Read32(taskState) || !reader.Read32(currentOrdinal) ||
            !reader.Read32(function->calledTask) || !reader.Read32(calledOrdinal)) {
            return S3D_FORMAT;
        }
        function->state = static_cast<ScriptTaskState>(taskState);
        function->currentCommand = currentOrdinal == 0 ? nullptr
            : FindCommandByOrdinal(*function, currentOrdinal);
        function->calledCommand = nullptr;
        if (calledOrdinal != 0 && function->calledTask < functions.size()) {
            function->calledCommand = FindCommandByOrdinal(
                *functions[function->calledTask], calledOrdinal);
        }
        for (ScriptValue* value : function->locals) {
            if (!readValue(*value)) {
                return S3D_FORMAT;
            }
        }
    }
    if (reader.position != reader.size) {
        return S3D_FORMAT;
    }
    state = ScriptState::Running;
    previousState = ScriptState::Running;
    return S3D_OK;
}

void SimpleScript::Start()
{
    state = ScriptState::Initializing;
    previousState = ScriptState::Initializing;
    if (initializer != nullptr) {
        ResetFunction(*initializer, false);
    }
    for (const auto& function : functions) {
        ResetFunction(*function, false);
    }
}

void SimpleScript::Update()
{
    if (scriptInterface == nullptr) {
        return;
    }
    ++executionDepth;
    if (state == ScriptState::Initializing) {
        if (initializer == nullptr) {
            state = ScriptState::Initialized;
            previousState = state;
        } else {
            initializer->state = ScriptTaskState::Running;
            currentLocals = &initializer->locals;
            if (initializer->currentCommand == nullptr) {
                state = ScriptState::Initialized;
                previousState = state;
            } else if (initializer->currentCommand->ordinal <= 2) {
                if (!Step(*initializer, false)) {
                    state = ScriptState::Initialized;
                    previousState = state;
                }
            } else {
                while (Step(*initializer, false)) {
                }
                state = ScriptState::Initialized;
                previousState = state;
            }
        }
    } else if (state == ScriptState::Running) {
        for (uint32_t index = 0; index < functions.size(); ++index) {
            ScriptFunction& function = *functions[index];
            currentLocals = &function.locals;
            currentFunction = index;
            if (function.state == ScriptTaskState::Running ||
                function.state == ScriptTaskState::Suspended) {
                Step(function, false);
            } else if (function.state == ScriptTaskState::Enabled && StartFunction(function)) {
                Step(function, false);
            }
        }
    }
    --executionDepth;
    ProcessPendingLoad();
}

void SimpleScript::Reset()
{
    for (const auto& function : functions) {
        ResetFunction(*function, false);
    }
    currentFunction = 0;
    shared.fill(0);
    for (ScriptValue* value : scope0Variables) {
        value->Clear();
    }
    for (ScriptValue* global : globals) {
        global->Clear();
    }
    currentLocals = nullptr;
}

void SimpleScript::Unload()
{
    initializer.reset();
    functions.clear();
    commands.clear();
    variables.clear();
    scope0Variables.clear();
    globals.clear();
    state = ScriptState::Unloaded;
    previousState = ScriptState::Unloaded;
    currentFunction = 0;
    currentLocals = nullptr;
    commandCounter = 0;
    pendingLoad = false;
    pendingLoadName.clear();
}

void SimpleScript::SetScriptInterface(ScriptInterface* interface)
{
    scriptInterface = interface;
    if (scriptInterface != nullptr) {
        scriptInterface->SetScript(this);
    }
}

ScriptTaskState SimpleScript::GetTaskState(uint32_t index) const
{
    return index < functions.size() ? functions[index]->state : ScriptTaskState::Disabled;
}

const ScriptFunction& SimpleScript::GetFunction(uint32_t index) const
{
    return *functions.at(index);
}

ScriptValue* SimpleScript::GetGlobalVariable(uint32_t index)
{
    return index < globals.size() ? globals[index] : nullptr;
}

const ScriptValue* SimpleScript::GetGlobalVariable(uint32_t index) const
{
    return index < globals.size() ? globals[index] : nullptr;
}

ScriptValue* ScriptFunction::GetLocalVariable(uint32_t index)
{
    return index < locals.size() ? locals[index] : nullptr;
}

const ScriptValue* ScriptFunction::GetLocalVariable(uint32_t index) const
{
    return index < locals.size() ? locals[index] : nullptr;
}

void SimpleScript::EnableTask(int16_t index)
{
    if (index < 0 || static_cast<size_t>(index) >= functions.size()) {
        return;
    }
    ScriptFunction& function = *functions[index];
    if (function.state == ScriptTaskState::Disabled) {
        function.state = ScriptTaskState::Enabled;
    } else if (function.state == ScriptTaskState::Suspended) {
        function.state = ScriptTaskState::Running;
    }
}

void SimpleScript::DisableTask(int16_t index)
{
    if (index < 0 || static_cast<size_t>(index) >= functions.size()) {
        return;
    }
    ScriptFunction& function = *functions[index];
    if (function.state == ScriptTaskState::Running) {
        function.state = ScriptTaskState::Suspended;
    } else if (function.state == ScriptTaskState::Enabled) {
        function.state = ScriptTaskState::Disabled;
    }
}

int SimpleScript::RunTask(int16_t index)
{
    if (index < 0 || static_cast<size_t>(index) >= functions.size() || currentFunction >= functions.size()) {
        return 1;
    }
    ScriptFunction& caller = *functions[currentFunction];
    ScriptFunction& target = *functions[index];
    caller.calledTask = static_cast<uint32_t>(index);
    caller.calledCommand = target.commandList.empty() ? nullptr : target.commandList[0];
    return 1;
}

int SimpleScript::InterruptTask(int16_t index)
{
    if (index < 0 || static_cast<size_t>(index) >= functions.size()) {
        return 1;
    }
    ResetFunction(*functions[index], true);
    return 1;
}

int SimpleScript::LoadScript(const std::u16string& scriptName)
{
    if (executionDepth != 0) {
        pendingLoad = true;
        pendingLoadName = scriptName;
        return 1;
    }
    currentFunction = 0;
    shared.fill(0);
    LoadFromPak(scriptName.c_str(), archive);
    Start();
    return 1;
}

int SimpleScript::SetSharedVariable(int32_t index, uint32_t value)
{
    if (index < 0 || static_cast<size_t>(index) >= shared.size()) {
        return 1;
    }
    shared[static_cast<size_t>(index)] = value;
    return 1;
}

uint32_t SimpleScript::GetSharedVariable(int32_t index) const
{
    if (index < 0 || static_cast<size_t>(index) >= shared.size()) {
        return 0;
    }
    return shared[static_cast<size_t>(index)];
}

ScriptValue* SimpleScript::AllocateVariable(ScriptValueType type)
{
    if (variables.size() >= maxScriptVariables) {
        return nullptr;
    }
    variables.push_back(std::make_unique<ScriptValue>(type));
    return variables.back().get();
}

ScriptCommand* SimpleScript::AllocateCommand()
{
    if (commands.size() >= maxScriptCommands) {
        return nullptr;
    }
    commands.push_back(std::make_unique<ScriptCommand>());
    return commands.back().get();
}

ScriptValue* SimpleScript::ResolveReference(uint16_t scope, uint16_t index,
                                            ScriptFunction*) const
{
    const std::vector<ScriptValue*>* values = nullptr;
    if (scope == 0) {
        values = &scope0Variables;
    } else if (scope == 1) {
        values = &globals;
    } else if (scope == 2) {
        values = currentLocals;
    }
    if (values == nullptr || index >= values->size()) {
        return nullptr;
    }
    return (*values)[index];
}

void SimpleScript::ClearVariables(ScriptFunction& function)
{
    for (ScriptValue* value : function.locals) {
        value->Clear();
    }
}

void SimpleScript::ResetFunction(ScriptFunction& function, bool interrupt)
{
    function.calledCommand = nullptr;
    function.calledTask = 0;
    if (function.state == ScriptTaskState::Running) {
        function.state = ScriptTaskState::Enabled;
    }
    if (function.state == ScriptTaskState::Suspended || interrupt) {
        function.state = ScriptTaskState::Disabled;
    }
    function.currentCommand = function.commandList.empty() ? nullptr : function.commandList[0];
    ClearVariables(function);
}

bool SimpleScript::StartFunction(ScriptFunction& function)
{
    if (function.entryCommands.empty()) {
        function.state = ScriptTaskState::Running;
        return true;
    }
    if (ExecuteCommand(function.entryCommands[0]) == nullptr) {
        function.state = ScriptTaskState::Running;
        return true;
    }
    return false;
}

bool SimpleScript::Step(ScriptFunction& function, bool nested)
{
    ++executionDepth;
    if (function.calledCommand != nullptr) {
        function.calledCommand = ExecuteCommand(function.calledCommand);
        while (function.calledCommand != nullptr) {
            ScriptCommand* executed = function.calledCommand;
            function.calledCommand = ExecuteCommand(function.calledCommand);
            if ((executed->type == 0 && !nested) || executed->id == 9) {
                break;
            }
            if (executed->id == 39 && Step(function, true)) {
                --executionDepth;
                return true;
            }
        }
        if (function.calledCommand != nullptr) {
            --executionDepth;
            return true;
        }
        function.calledTask = 0;
        ClearVariables(function);
        --executionDepth;
        return false;
    }

    if (function.currentCommand == nullptr) {
        ResetFunction(function, false);
        --executionDepth;
        return false;
    }
    while (function.currentCommand != nullptr) {
        ScriptCommand* executed = function.currentCommand;
        function.currentCommand = ExecuteCommand(function.currentCommand);
        if (function.currentCommand == nullptr || executed->type == 0) {
            break;
        }
        if (executed->id == 39 && Step(function, true)) {
            --executionDepth;
            return true;
        }
    }
    if (function.currentCommand != nullptr) {
        --executionDepth;
        return true;
    }
    if (function.calledCommand == nullptr) {
        ResetFunction(function, false);
    }
    --executionDepth;
    return false;
}

ScriptCommand* SimpleScript::ExecuteCommand(ScriptCommand* command)
{
    if (command == nullptr) {
        return nullptr;
    }
    ScriptCommand* next = command->next;
    const auto evaluateConditions = [this, command]() {
        for (ScriptCommand* condition : command->conditions) {
            ExecuteCommand(condition);
        }
    };

    switch (command->id) {
    case 7:
        evaluateConditions();
        next = command->GetArgument(0).GetBool() && !command->body.empty()
            ? command->body[0] : command->next;
        break;
    case 8: {
        evaluateConditions();
        for (ScriptCommand* child : command->body) {
            if (command->GetArgument(0).GetInt() == child->GetArgument(0).GetInt()) {
                next = child;
                break;
            }
        }
        break;
    }
    case 9:
        evaluateConditions();
        next = command->GetArgument(0).GetBool() ? command->next : command;
        break;
    case 10:
        ResetFunction(*command->function, false);
        return nullptr;
    case 11:
        next = command->body.empty() ? command->next : command->body[0];
        break;
    case 12:
        next = command->GetArgument(0).GetBool() && !command->body.empty()
            ? command->body[0] : command->next;
        break;
    case 13:
        evaluateConditions();
        next = command->GetArgument(0).GetBool() && !command->body.empty()
            ? command->body[0] : command->next;
        break;
    case 14:
        return nullptr;
    case 16:
    case 17:
    case 18:
    case 19:
    case 20:
    case 21:
    case 22:
    case 23:
        Evaluate(command->GetOutput(), command->GetArgument(0), command->GetArgument(1), command->id);
        break;
    case 24:
        Evaluate(command->GetOutput(), command->GetArgument(0), command->GetArgument(0), command->id);
        break;
    case 25:
        if (command->GetOutput() != nullptr) {
            command->GetOutput()->Assign(command->GetArgument(0));
        }
        break;
    case 26:
    case 27:
    case 28:
    case 29:
    case 30:
        if (command->GetOutput() != nullptr) {
            Evaluate(command->GetOutput(), *command->GetOutput(), command->GetArgument(0),
                     command->id + 5);
        }
        break;
    case 31:
    case 32:
    case 33:
    case 34:
    case 35:
        Evaluate(command->GetOutput(), command->GetArgument(0), command->GetArgument(1), command->id);
        break;
    default:
        if (command->id >= 36 && scriptInterface != nullptr) {
            scriptInterface->Execute(*command);
        }
        break;
    }
    return next == nullptr ? command->continuation : next;
}

void SimpleScript::LinkCommand(ScriptCommand* command)
{
    if (command == nullptr) {
        return;
    }
    if (command->id == 7) {
        ScriptCommand* continuation = command->next;
        if (continuation != nullptr && continuation->id == 11) {
            continuation = continuation->next;
        }
        if (continuation == nullptr) {
            continuation = command->continuation;
        }
        for (ScriptCommand* child : command->body) {
            child->continuation = continuation;
            LinkCommand(child);
        }
    } else if (command->id == 11) {
        ScriptCommand* continuation = command->next == nullptr ? command->continuation : command->next;
        for (ScriptCommand* child : command->body) {
            child->continuation = continuation;
            LinkCommand(child);
        }
    } else if (command->id == 13) {
        for (ScriptCommand* child : command->body) {
            child->continuation = command;
            LinkCommand(child);
        }
    } else if (command->id == 12) {
        ScriptCommand* continuation = command->next;
        while (continuation != nullptr && continuation->id == 12) {
            continuation = continuation->next;
        }
        if (continuation == nullptr) {
            continuation = command->continuation;
        }
        for (ScriptCommand* child : command->body) {
            child->continuation = continuation;
            LinkCommand(child);
        }
    }
}

void SimpleScript::Evaluate(ScriptValue* output, const ScriptValue& left, const ScriptValue& right,
                            uint16_t operation)
{
    if (output == nullptr) {
        return;
    }
    const ScriptValueType type = left.GetType();
    if (operation >= 16 && operation <= 23) {
        if ((type > ScriptValueType::Int && type != ScriptValueType::String &&
             type != ScriptValueType::Bool && type != ScriptValueType::Fixed) ||
            (type == ScriptValueType::String && operation > 21)) {
            return;
        }
        bool result = false;
        if (type == ScriptValueType::String) {
            const int comparison = CompareStrings(left.GetString(), right.GetString());
            switch (operation) {
            case 16: result = comparison == 0; break;
            case 17: result = comparison <= 0; break;
            case 18: result = comparison >= 0; break;
            case 19: result = comparison < 0; break;
            case 20: result = comparison > 0; break;
            case 21: result = comparison != 0; break;
            default: break;
            }
        } else if (type == ScriptValueType::Fixed) {
            const Fixed a = left.GetFixed();
            const Fixed b = right.GetFixed();
            switch (operation) {
            case 16: result = a == b; break;
            case 17: result = a <= b; break;
            case 18: result = a >= b; break;
            case 19: result = a < b; break;
            case 20: result = a > b; break;
            case 21: result = a != b; break;
            case 22: result = left.GetBool() && right.GetBool(); break;
            case 23: result = left.GetBool() || right.GetBool(); break;
            default: break;
            }
        } else if (type == ScriptValueType::Bool) {
            const bool a = left.GetBool();
            const bool b = right.GetBool();
            switch (operation) {
            case 16: result = a == b; break;
            case 17: result = a <= b; break;
            case 18: result = a >= b; break;
            case 19: result = a < b; break;
            case 20: result = a > b; break;
            case 21: result = a != b; break;
            case 22: result = a && b; break;
            case 23: result = a || b; break;
            default: break;
            }
        } else if (type <= ScriptValueType::Int) {
            const int32_t a = left.GetInt();
            const int32_t b = right.GetInt();
            switch (operation) {
            case 16: result = a == b; break;
            case 17: result = a <= b; break;
            case 18: result = a >= b; break;
            case 19: result = a < b; break;
            case 20: result = a > b; break;
            case 21: result = a != b; break;
            case 22: result = left.GetBool() && right.GetBool(); break;
            case 23: result = left.GetBool() || right.GetBool(); break;
            default: break;
            }
        }
        output->SetBool(result);
        return;
    }
    if (operation == 24) {
        if (type <= ScriptValueType::Int || type == ScriptValueType::Bool) {
            output->SetBool(!left.GetBool());
        }
        return;
    }
    if (type <= ScriptValueType::Int) {
        const int32_t a = left.GetInt();
        const int32_t b = right.GetInt();
        switch (operation) {
        case 31: output->SetInt(WrapAdd(a, b)); break;
        case 32: output->SetInt(WrapSubtract(a, b)); break;
        case 33: output->SetInt(WrapMultiply(a, b)); break;
        case 34: output->SetInt(WrapDivide(a, b)); break;
        case 35: output->SetInt(WrapModulo(a, b)); break;
        default: break;
        }
    } else if (type == ScriptValueType::Fixed) {
        const Fixed a = left.GetFixed();
        const Fixed b = right.GetFixed();
        switch (operation) {
        case 31: output->SetFixed(AddFixed(a, b)); break;
        case 32: output->SetFixed(SubtractFixed(a, b)); break;
        case 33: output->SetFixed(FixedMultiply(a, b)); break;
        case 34: output->SetFixed(FixedDivide(a, b)); break;
        default: break;
        }
    }
}

void SimpleScript::ProcessPendingLoad()
{
    if (!pendingLoad || executionDepth != 0) {
        return;
    }
    const std::u16string scriptName = pendingLoadName;
    pendingLoad = false;
    pendingLoadName.clear();
    LoadScript(scriptName);
}

} // namespace soft3d
