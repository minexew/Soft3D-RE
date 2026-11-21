#pragma once

#include "soft3d/Fixed.hpp"
#include "soft3d/s3dBase.hpp"
#include "soft3d/s3dCommon.hpp"
#include "soft3d/s3dFilePak.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace soft3d {

enum class ScriptValueType : int32_t {
    Byte = 0,
    Short = 1,
    Int = 2,
    String = 3,
    Bool = 4,
    Fixed = 5,
    Reference = 6,
};

class ScriptValue {
public:
    ScriptValue() = default;
    explicit ScriptValue(ScriptValueType type);

    ScriptValueType GetType() const { return type; }
    int32_t GetInt() const { return number; }
    int16_t GetShort() const { return static_cast<int16_t>(number); }
    uint8_t GetByte() const { return static_cast<uint8_t>(number); }
    Fixed GetFixed() const { return number; }
    bool GetBool() const { return number != 0; }
    const std::u16string& GetString() const { return string; }

    void SetInt(int32_t value) { number = value; }
    void SetShort(int16_t value) { number = value; }
    void SetByte(uint8_t value) { number = value; }
    void SetFixed(Fixed value) { number = value; }
    void SetBool(bool value) { number = value ? 1 : 0; }
    void SetString(std::u16string value) { string = std::move(value); }
    void Assign(const ScriptValue& source);
    void Clear();

private:
    ScriptValueType type = ScriptValueType::Byte;
    int32_t number = 0;
    std::u16string string;
};

class SimpleScript;
class ScriptFunction;

class ScriptCommand {
public:
    uint16_t GetId() const { return id; }
    uint16_t GetType() const { return type; }
    uint16_t GetArgumentCount() const { return argumentCount; }
    uint32_t GetOrdinal() const { return ordinal; }
    ScriptValue& GetArgument(uint32_t index);
    const ScriptValue& GetArgument(uint32_t index) const;
    ScriptValue* GetOutput() const;
    const std::vector<ScriptCommand*>& GetBody() const { return body; }
    const std::vector<ScriptCommand*>& GetConditions() const { return conditions; }

private:
    struct Reference {
        uint16_t scope = 0;
        uint16_t index = 0;
    };
    struct Argument {
        ScriptValue value;
        Reference reference;
    };

    uint16_t id = 0;
    uint16_t type = 0;
    uint16_t argumentCount = 0;
    std::array<Argument, 32> arguments;
    Reference outputReference{};
    bool hasOutput = false;
    ScriptFunction* function = nullptr;
    SimpleScript* script = nullptr;
    std::vector<ScriptCommand*> body;
    std::vector<ScriptCommand*> conditions;
    ScriptCommand* next = nullptr;
    ScriptCommand* continuation = nullptr;
    uint32_t ordinal = 0;

    friend class SimpleScript;
};

class ScriptInterface {
public:
    virtual ~ScriptInterface() = default;

    void SetScript(SimpleScript* script) { this->script = script; }
    void Execute(ScriptCommand& command);

protected:
    virtual void ExecuteGameCommand(ScriptCommand&) {}

    SimpleScript* script = nullptr;
};

enum class ScriptState : int32_t {
    Unloaded = 0,
    Loaded = 1,
    Initializing = 2,
    Initialized = 3,
    Running = 4,
};

enum class ScriptTaskState : int32_t {
    Disabled = 0,
    Enabled = 1,
    Running = 2,
    Suspended = 3,
};

class SimpleScript : public s3dBase {
public:
    SimpleScript();
    ~SimpleScript() override;

    result Init();
    result LoadFromPak(const char16_t* name, s3dFilePak* archive);
    result Load(const uint8_t* data, size_t size);
    size_t GetArchiveSize() const;
    result SaveArchive(const char16_t* path);
    result LoadArchive(const char16_t* path, s3dFilePak* archive);
    void Start();
    void StartTasks();
    void Update();
    void Reset();
    void ClearSharedVariables() { shared.fill(0); }
    void Unload();

    void SetScriptInterface(ScriptInterface* interface);
    void SetState(ScriptState value) { state = value; }
    ScriptState GetState() const { return state; }
    size_t GetFunctionCount() const { return functions.size(); }
    size_t GetGlobalVariableCount() const { return globals.size(); }
    size_t GetCommandCount() const { return commands.size(); }
    size_t GetVariableCount() const { return variables.size(); }
    ScriptTaskState GetTaskState(uint32_t index) const;
    const ScriptFunction& GetFunction(uint32_t index) const;
    const ScriptFunction* GetInitializer() const { return initializer.get(); }
    ScriptValue* GetGlobalVariable(uint32_t index);
    const ScriptValue* GetGlobalVariable(uint32_t index) const;

    void EnableTask(int16_t index);
    void DisableTask(int16_t index);
    int RunTask(int16_t index);
    int InterruptTask(int16_t index);
    int LoadScript(const std::u16string& name);
    int SetSharedVariable(int32_t index, uint32_t value);
    uint32_t GetSharedVariable(int32_t index) const;

private:
    class Parser;
    friend class ScriptCommand;

    ScriptValue* AllocateVariable(ScriptValueType type);
    ScriptCommand* AllocateCommand();
    ScriptValue* ResolveReference(uint16_t scope, uint16_t index, ScriptFunction* function) const;
    void ClearVariables(ScriptFunction& function);
    void ResetFunction(ScriptFunction& function, bool interrupt);
    bool StartFunction(ScriptFunction& function);
    bool Step(ScriptFunction& function, bool nested);
    ScriptCommand* ExecuteCommand(ScriptCommand* command);
    void LinkCommand(ScriptCommand* command);
    void Evaluate(ScriptValue* output, const ScriptValue& left, const ScriptValue& right,
                  uint16_t operation);
    void ProcessPendingLoad();
    static size_t GetSerializedValueSize(const ScriptValue& value);
    static void WriteSerializedValue(std::vector<uint8_t>& output, const ScriptValue& value);
    static ScriptCommand* FindCommandByOrdinal(const ScriptFunction& function, uint32_t ordinal);

    std::unique_ptr<ScriptFunction> initializer;
    std::vector<std::unique_ptr<ScriptFunction>> functions;
    std::vector<std::unique_ptr<ScriptCommand>> commands;
    std::vector<std::unique_ptr<ScriptValue>> variables;
    std::vector<ScriptValue*> scope0Variables;
    std::vector<ScriptValue*> globals;
    std::array<uint32_t, 256> shared{};
    ScriptInterface* scriptInterface = nullptr;
    s3dFilePak* archive = nullptr;
    std::u16string name;
    ScriptState state = ScriptState::Unloaded;
    ScriptState previousState = ScriptState::Unloaded;
    uint32_t currentFunction = 0;
    std::vector<ScriptValue*>* currentLocals = nullptr;
    uint32_t commandCounter = 0;
    uint32_t executionDepth = 0;
    bool pendingLoad = false;
    std::u16string pendingLoadName;
};

class ScriptFunction {
public:
    ScriptTaskState GetState() const { return state; }
    size_t GetLocalVariableCount() const { return locals.size(); }
    size_t GetEntryCommandCount() const { return entryCommands.size(); }
    size_t GetCommandCount() const { return commandList.size(); }
    const std::vector<ScriptCommand*>& GetEntryCommands() const { return entryCommands; }
    const std::vector<ScriptCommand*>& GetCommands() const { return commandList; }
    ScriptValue* GetLocalVariable(uint32_t index);
    const ScriptValue* GetLocalVariable(uint32_t index) const;
    const ScriptCommand* GetCurrentCommand() const { return currentCommand; }
    const ScriptCommand* GetCalledCommand() const { return calledCommand; }
    uint32_t GetCalledTask() const { return calledTask; }

private:
    ScriptTaskState state = ScriptTaskState::Disabled;
    std::vector<ScriptCommand*> commandList;
    ScriptCommand* currentCommand = nullptr;
    std::vector<ScriptCommand*> entryCommands;
    uint32_t calledTask = 0;
    ScriptCommand* calledCommand = nullptr;
    SimpleScript* script = nullptr;
    std::vector<ScriptValue*> locals;

    friend class SimpleScript;
};

} // namespace soft3d
