#include "soft3d/SimpleScript.hpp"
#include "soft3d/s3dResManager.hpp"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using namespace soft3d;

void Check(bool condition, int line)
{
    if (!condition) {
        throw std::runtime_error("check failed at line " + std::to_string(line));
    }
}

#define CHECK(condition) Check((condition), __LINE__)

struct Argument {
    ScriptValueType type = ScriptValueType::Int;
    int32_t number = 0;
    std::string string;
    uint16_t scope = 0;
    uint16_t index = 0;

    static Argument Int(int32_t value) { return {ScriptValueType::Int, value, {}, 0, 0}; }
    static Argument Fixed(int32_t value) { return {ScriptValueType::Fixed, value, {}, 0, 0}; }
    static Argument Bool(bool value)
    {
        return {ScriptValueType::Bool, value ? 1 : 0, {}, 0, 0};
    }
    static Argument String(const char* value) { return {ScriptValueType::String, 0, value}; }
    static Argument Ref(uint16_t scope, uint16_t index)
    {
        return {ScriptValueType::Reference, 0, {}, scope, index};
    }
};

struct Reference {
    uint16_t scope;
    uint16_t index;
};

struct Command {
    uint16_t id;
    uint16_t type = 1;
    std::vector<Argument> arguments;
    bool hasOutput = false;
    Reference output{0, 0};
    std::vector<Command> body;
    std::vector<Command> conditions;
};

Command Op(uint16_t id, std::vector<Argument> arguments = {}, Reference output = {0, 0})
{
    Command command{id, 1, std::move(arguments), false, {0, 0}, {}, {}};
    command.hasOutput = true;
    command.output = output;
    return command;
}

Command Host(uint16_t id, std::vector<Argument> arguments = {})
{
    return {id, 1, std::move(arguments), false, {0, 0}, {}, {}};
}

struct Function {
    std::vector<ScriptValueType> locals;
    std::vector<Command> entry;
    std::vector<Command> commands;
};

void Append16(std::vector<uint8_t>& bytes, uint16_t value)
{
    bytes.push_back(static_cast<uint8_t>(value));
    bytes.push_back(static_cast<uint8_t>(value >> 8));
}

void Append32(std::vector<uint8_t>& bytes, uint32_t value)
{
    Append16(bytes, static_cast<uint16_t>(value));
    Append16(bytes, static_cast<uint16_t>(value >> 16));
}

void AppendArgument(std::vector<uint8_t>& bytes, const Argument& argument)
{
    Append16(bytes, static_cast<uint16_t>(argument.type));
    if (argument.type == ScriptValueType::String) {
        const uint16_t size = static_cast<uint16_t>((argument.string.size() + 2) & ~size_t(1));
        Append16(bytes, size);
        for (char c : argument.string) {
            bytes.push_back(static_cast<uint8_t>(c));
        }
        bytes.push_back(0);
        while ((bytes.size() & 1) != 0) {
            bytes.push_back(0);
        }
    } else {
        Append16(bytes, 4);
        Append32(bytes, argument.type == ScriptValueType::Reference
            ? static_cast<uint32_t>(argument.index) |
                  static_cast<uint32_t>(argument.scope) << 16
            : static_cast<uint32_t>(argument.number));
    }
}

void AppendCommand(std::vector<uint8_t>& bytes, const Command& command)
{
    Append16(bytes, command.id);
    Append16(bytes, command.type);
    Append16(bytes, static_cast<uint16_t>(command.arguments.size()));
    for (const Argument& argument : command.arguments) {
        AppendArgument(bytes, argument);
    }
    if (command.hasOutput) {
        Append16(bytes, 6);
        Append16(bytes, 4);
        Append16(bytes, command.output.index);
        Append16(bytes, command.output.scope);
    } else {
        Append16(bytes, 0);
    }
    Append16(bytes, static_cast<uint16_t>(command.body.size()));
    for (const Command& child : command.body) {
        AppendCommand(bytes, child);
    }
    Append16(bytes, static_cast<uint16_t>(command.conditions.size()));
    for (const Command& condition : command.conditions) {
        AppendCommand(bytes, condition);
    }
}

void AppendFunction(std::vector<uint8_t>& bytes, const Function& function)
{
    Append16(bytes, static_cast<uint16_t>(function.locals.size()));
    for (ScriptValueType type : function.locals) {
        Append16(bytes, static_cast<uint16_t>(type));
    }
    Append16(bytes, static_cast<uint16_t>(function.entry.size()));
    for (const Command& command : function.entry) {
        AppendCommand(bytes, command);
    }
    Append16(bytes, static_cast<uint16_t>(function.commands.size()));
    for (const Command& command : function.commands) {
        AppendCommand(bytes, command);
    }
}

std::vector<uint8_t> MakeScript(const std::vector<ScriptValueType>& globals,
                                const std::vector<Function>& functions,
                                const Function* initializer = nullptr)
{
    std::vector<uint8_t> bytes;
    Append16(bytes, static_cast<uint16_t>(globals.size()));
    for (ScriptValueType type : globals) {
        Append16(bytes, static_cast<uint16_t>(type));
    }
    Append16(bytes, initializer == nullptr ? 0 : 1);
    if (initializer != nullptr) {
        AppendFunction(bytes, *initializer);
    }
    Append16(bytes, static_cast<uint16_t>(functions.size()));
    for (const Function& function : functions) {
        AppendFunction(bytes, function);
    }
    return bytes;
}

class TestInterface : public ScriptInterface {
public:
    bool condition = false;
    int calls = 0;
    std::vector<uint16_t> ids;
    std::vector<int32_t> observations;

protected:
    void ExecuteGameCommand(ScriptCommand& command) override
    {
        ids.push_back(command.GetId());
        ++calls;
        if (command.GetId() == 50 && command.GetOutput() != nullptr) {
            command.GetOutput()->SetBool(condition);
        } else if (command.GetId() == 52) {
            observations.push_back(command.GetArgument(0).GetInt());
        }
    }
};

void LoadAndRun(SimpleScript& script, TestInterface& interface,
                const std::vector<uint8_t>& bytes)
{
    CHECK(script.Load(bytes.data(), bytes.size()) == S3D_OK);
    script.SetScriptInterface(&interface);
    script.Start();
    script.SetState(ScriptState::Running);
    script.EnableTask(0);
    script.Update();
}

void TestRealScripts()
{
#ifndef SOFT3D_SPK_ENGLISH_LAYOUT
    s3dResManager manager;
    CHECK(OpenGameResourceArchive(&manager, SOFT3D_TEST_ARCHIVE) == S3D_OK);
    struct Expected {
        const char16_t* name;
        size_t globals;
        size_t functions;
        size_t commands;
        size_t variables;
    };
    const Expected cases[] = {
        {u".\\day1\\ep0", 0, 6, 135, 30},
        {u".\\day1\\ep1010", 0, 9, 198, 56},
        {u".\\day5\\ep2050", 13, 16, 829, 244},
        {u".\\day7\\ep7009", 0, 1, 93, 29},
    };
    for (const Expected& expected : cases) {
        SimpleScript script;
        CHECK(script.Init() == S3D_OK);
        CHECK(script.LoadFromPak(expected.name, manager.GetFilePak()) == S3D_OK);
        CHECK(script.GetState() == ScriptState::Loaded);
        CHECK(script.GetGlobalVariableCount() == expected.globals);
        CHECK(script.GetFunctionCount() == expected.functions);
        CHECK(script.GetCommandCount() == expected.commands);
        CHECK(script.GetVariableCount() == expected.variables);
        CHECK(script.GetInitializer() != nullptr);
        script.Unload();
        CHECK(script.GetState() == ScriptState::Unloaded);
        CHECK(script.GetFunctionCount() == 0 && script.GetCommandCount() == 0);
        CHECK(script.LoadFromPak(expected.name, manager.GetFilePak()) == S3D_OK);
    }
    SimpleScript missing;
    CHECK(missing.LoadFromPak(u".\\day1\\not-present", manager.GetFilePak()) == S3D_FILE);
#endif
}

void TestValuesAndMalformedInput()
{
    const std::vector<ScriptValueType> types = {
        ScriptValueType::Byte, ScriptValueType::Short, ScriptValueType::Int,
        ScriptValueType::String, ScriptValueType::Bool, ScriptValueType::Fixed,
    };
    std::vector<Command> commands;
    for (uint32_t index = 0; index < types.size(); ++index) {
        commands.push_back(Op(25, {Argument::Int(7)}, {1, static_cast<uint16_t>(index)}));
    }
    commands.push_back(Op(25, {Argument::String("hello")}, {1, 3}));
    SimpleScript script;
    const auto bytes = MakeScript(types, {Function{{}, {}, commands}});
    CHECK(script.Load(bytes.data(), bytes.size()) == S3D_OK);
    CHECK(script.GetGlobalVariableCount() == 6);
    CHECK(script.GetFunction(0).GetCommandCount() == 7);
    for (uint32_t index = 0; index < types.size(); ++index) {
        CHECK(script.GetGlobalVariable(index)->GetType() == types[index]);
        CHECK(script.GetGlobalVariable(index)->GetInt() == 0);
    }
    TestInterface interface;
    script.SetScriptInterface(&interface);
    script.Start();
    script.SetState(ScriptState::Running);
    script.EnableTask(0);
    script.Update();
    for (uint32_t index = 0; index < types.size(); ++index) {
        if (index != 3) {
            CHECK(script.GetGlobalVariable(index)->GetInt() == 7);
        }
    }
    CHECK(script.GetGlobalVariable(3)->GetString() == u"hello");
    CHECK(script.GetGlobalVariable(0)->GetType() == ScriptValueType::Byte);

    SimpleScript malformed;
    CHECK(malformed.Load(nullptr, 0) == S3D_FORMAT);
    CHECK(malformed.Load(bytes.data(), bytes.size() - 1) == S3D_FORMAT);
    auto trailing = bytes;
    trailing.push_back(0);
    CHECK(malformed.Load(trailing.data(), trailing.size()) == S3D_FORMAT);
    auto badType = MakeScript({ScriptValueType::Int}, {});
    badType[2] = 7;
    CHECK(malformed.Load(badType.data(), badType.size()) == S3D_FORMAT);
    auto badReference = MakeScript({ScriptValueType::Int},
        {Function{{}, {}, {Host(52, {Argument::Ref(3, 0)})}}});
    CHECK(malformed.Load(badReference.data(), badReference.size()) == S3D_FORMAT);
    auto badIndex = MakeScript({ScriptValueType::Int},
        {Function{{}, {}, {Host(52, {Argument::Ref(1, 1)})}}});
    CHECK(malformed.Load(badIndex.data(), badIndex.size()) == S3D_FORMAT);
    auto badOutput = MakeScript({ScriptValueType::Int},
        {Function{{}, {}, {Op(25, {Argument::Int(1)}, {1, 1})}}});
    CHECK(malformed.Load(badOutput.data(), badOutput.size()) == S3D_FORMAT);
    auto tooManyArguments = MakeScript({ScriptValueType::Int},
        {Function{{}, {}, {Host(52, std::vector<Argument>(33))}}});
    CHECK(malformed.Load(tooManyArguments.data(), tooManyArguments.size()) == S3D_FORMAT);
}

void TestIntegerAndFixedArithmetic()
{
    struct Case {
        uint16_t id;
        int32_t left;
        int32_t right;
        int32_t expected;
    };
    const Case binary[] = {
        {31, 7, 3, 10}, {32, 7, 3, 4}, {33, 7, 3, 21},
        {34, 7, 3, 2}, {35, 7, 3, 1},
        {31, 0x7fffffff, 1, static_cast<int32_t>(0x80000000U)},
        {34, static_cast<int32_t>(0x80000000U), -1, static_cast<int32_t>(0x80000000U)},
        {35, static_cast<int32_t>(0x80000000U), -1, 0},
    };
    for (const Case& test : binary) {
        SimpleScript script;
        TestInterface interface;
        LoadAndRun(script, interface, MakeScript({ScriptValueType::Int},
            {Function{{}, {}, {Op(test.id, {Argument::Int(test.left), Argument::Int(test.right)},
                                      {1, 0})}}}));
        CHECK(script.GetGlobalVariable(0)->GetInt() == test.expected);
    }
    const Case inplace[] = {
        {26, 10, 5, 15}, {27, 10, 5, 5}, {28, 10, 5, 50},
        {29, 10, 5, 2}, {30, 10, 5, 0},
    };
    for (const Case& test : inplace) {
        SimpleScript script;
        TestInterface interface;
        LoadAndRun(script, interface, MakeScript({ScriptValueType::Int},
            {Function{{}, {}, {
                Op(25, {Argument::Int(test.left)}, {1, 0}),
                Op(test.id, {Argument::Int(test.right)}, {1, 0}),
            }}}));
        CHECK(script.GetGlobalVariable(0)->GetInt() == test.expected);
    }
    const Case fixed[] = {
        {31, 0x20000, 0x18000, 0x38000}, {32, 0x20000, 0x18000, 0x8000},
        {33, 0x20000, 0x18000, 0x30000}, {34, 0x20000, 0x18000, 0x15555},
        {35, 0x20000, 0x18000, 0},
    };
    for (const Case& test : fixed) {
        SimpleScript script;
        TestInterface interface;
        LoadAndRun(script, interface, MakeScript({ScriptValueType::Fixed},
            {Function{{}, {}, {Op(test.id, {Argument::Fixed(test.left), Argument::Fixed(test.right)},
                                      {1, 0})}}}));
        CHECK(script.GetGlobalVariable(0)->GetFixed() == test.expected);
    }
}

void TestComparisonsAndLogic()
{
    struct Case {
        uint16_t id;
        int32_t left;
        int32_t right;
        bool expected;
    };
    const Case cases[] = {
        {16, 4, 4, true}, {17, 4, 4, true}, {18, 4, 4, true},
        {19, 4, 5, true}, {20, 5, 4, true}, {21, 4, 5, true},
        {22, 1, 1, true}, {22, 1, 0, false}, {23, 0, 1, true},
        {23, 0, 0, false}, {24, 0, 0, true}, {24, 1, 1, false},
    };
    for (const Case& test : cases) {
        SimpleScript script;
        TestInterface interface;
        LoadAndRun(script, interface, MakeScript({ScriptValueType::Int},
            {Function{{}, {}, {Op(test.id, {Argument::Int(test.left), Argument::Int(test.right)},
                                      {1, 0})}}}));
        CHECK(script.GetGlobalVariable(0)->GetBool() == test.expected);
        CHECK(script.GetGlobalVariable(0)->GetType() == ScriptValueType::Int);
    }
    for (uint16_t operation = 16; operation <= 21; ++operation) {
        SimpleScript script;
        TestInterface interface;
        LoadAndRun(script, interface, MakeScript({ScriptValueType::Fixed},
            {Function{{}, {}, {Op(operation, {Argument::Fixed(0x20000), Argument::Fixed(0x10000)},
                                      {1, 0})}}}));
        const bool expected = operation == 18 || operation == 20 || operation == 21;
        CHECK(script.GetGlobalVariable(0)->GetBool() == expected);
    }
    for (uint16_t operation = 16; operation <= 21; ++operation) {
        SimpleScript script;
        TestInterface interface;
        LoadAndRun(script, interface, MakeScript({ScriptValueType::String},
            {Function{{}, {}, {Op(operation, {Argument::String("a"), Argument::String("b")},
                                      {1, 0})}}}));
        const bool expected = operation == 17 || operation == 19 || operation == 21;
        CHECK(script.GetGlobalVariable(0)->GetBool() == expected);
        CHECK(script.GetGlobalVariable(0)->GetType() == ScriptValueType::String);
    }
    for (uint16_t operation : {uint16_t(22), uint16_t(23)}) {
        SimpleScript script;
        TestInterface interface;
        LoadAndRun(script, interface, MakeScript({ScriptValueType::String},
            {Function{{}, {}, {
                Op(25, {Argument::Int(99)}, {1, 0}),
                Op(operation, {Argument::String("a"), Argument::String("b")}, {1, 0}),
            }}}));
        CHECK(script.GetGlobalVariable(0)->GetInt() == 99);
    }
}

void TestStructuredControlAndSuspension()
{
    {
        SimpleScript script;
        TestInterface interface;
        Command conditional = Op(7, {Argument::Ref(2, 0)});
        conditional.body = {Host(51)};
        conditional.conditions = {Op(50, {}, {2, 0})};
        Command alternative = Host(11);
        alternative.body = {Host(52, {Argument::Int(99)})};
        LoadAndRun(script, interface, MakeScript({}, {
            Function{{ScriptValueType::Bool}, {}, {conditional, alternative}}}));
        CHECK(interface.calls == 2 && interface.ids[0] == 50 && interface.ids[1] == 52);
        CHECK(interface.observations.size() == 1 && interface.observations[0] == 99);
        interface.condition = true;
        script.EnableTask(0);
        script.Update();
        CHECK(interface.ids.size() == 4 && interface.ids[2] == 50 && interface.ids[3] == 51);
    }
    {
        SimpleScript script;
        TestInterface interface;
        Command wait = Op(9, {Argument::Ref(2, 0)});
        wait.type = 0;
        wait.conditions = {Op(50, {}, {2, 0})};
        LoadAndRun(script, interface, MakeScript({}, {
            Function{{ScriptValueType::Bool}, {}, {wait, Host(51)}}}));
        CHECK(script.GetTaskState(0) == ScriptTaskState::Running);
        CHECK(interface.calls == 1);
        script.Update();
        CHECK(interface.calls == 2 && interface.ids.back() == 50);
        interface.condition = true;
        script.Update();
        CHECK(interface.ids.back() == 50);
        CHECK(script.GetTaskState(0) == ScriptTaskState::Running);
        script.Update();
        CHECK(interface.ids.back() == 51);
        CHECK(script.GetTaskState(0) == ScriptTaskState::Enabled);
    }
    {
        SimpleScript script;
        TestInterface interface;
        Command loop = Op(13, {Argument::Ref(2, 0)});
        loop.conditions = {Op(19, {Argument::Ref(1, 0), Argument::Int(3)}, {2, 0})};
        loop.body = {Op(26, {Argument::Int(1)}, {1, 0})};
        LoadAndRun(script, interface, MakeScript({ScriptValueType::Int}, {
            Function{{ScriptValueType::Bool}, {}, {loop, Host(51)}}}));
        CHECK(script.GetGlobalVariable(0)->GetInt() == 3);
        CHECK(interface.ids.back() == 51);
    }
    {
        SimpleScript script;
        TestInterface interface;
        Command select = Host(8, {Argument::Int(2)});
        Command first = Host(12, {Argument::Int(1)});
        first.body = {Host(52, {Argument::Int(1)})};
        Command second = Host(12, {Argument::Int(2)});
        second.body = {Host(52, {Argument::Int(2)})};
        select.body = {first, second};
        LoadAndRun(script, interface, MakeScript({}, {Function{{}, {}, {select}}}));
        CHECK(interface.observations.size() == 1 && interface.observations[0] == 2);
    }
    for (uint16_t termination : {uint16_t(10), uint16_t(14)}) {
        SimpleScript script;
        TestInterface interface;
        LoadAndRun(script, interface, MakeScript({}, {
            Function{{}, {}, {Host(51), Host(termination), Host(51)}}}));
        CHECK(interface.calls == 1);
        CHECK(script.GetTaskState(0) == ScriptTaskState::Enabled);
    }
}

void TestTasksCallsAndGenericCommands()
{
    {
        SimpleScript script;
        TestInterface interface;
        std::vector<Function> functions(3);
        functions[0].commands = {Host(37, {Argument::Int(1)})};
        functions[1].commands = {Host(51)};
        LoadAndRun(script, interface, MakeScript({}, functions));
        CHECK(script.GetTaskState(1) == ScriptTaskState::Enabled);
        CHECK(interface.calls == 1);
        script.DisableTask(0);
        script.DisableTask(1);
        CHECK(script.GetTaskState(1) == ScriptTaskState::Disabled);
        script.EnableTask(1);
        CHECK(script.GetTaskState(1) == ScriptTaskState::Enabled);
        script.InterruptTask(1);
        CHECK(script.GetTaskState(1) == ScriptTaskState::Disabled);
    }
    {
        SimpleScript script;
        TestInterface interface;
        std::vector<Function> functions(2);
        functions[0].locals = {ScriptValueType::Int};
        functions[0].commands = {
            Op(25, {Argument::Int(5)}, {2, 0}),
            Host(39, {Argument::Int(1)}),
        };
        functions[1].locals = {ScriptValueType::Int};
        functions[1].commands = {
            Op(26, {Argument::Int(2)}, {2, 0}),
            Host(52, {Argument::Ref(2, 0)}),
        };
        LoadAndRun(script, interface, MakeScript({}, functions));
        CHECK(interface.observations.empty());
        CHECK(script.GetFunction(0).GetCalledTask() == 1);
        script.Update();
        CHECK(interface.observations.size() == 1 && interface.observations[0] == 7);
        CHECK(script.GetFunction(0).GetLocalVariable(0)->GetInt() == 0);
        CHECK(script.GetFunction(1).GetLocalVariable(0)->GetInt() == 0);
        CHECK(script.GetFunction(0).GetCalledTask() == 0);
    }
    {
        SimpleScript script;
        TestInterface interface;
        LoadAndRun(script, interface, MakeScript({ScriptValueType::Int}, {
            Function{{}, {}, {
                Host(41, {Argument::Int(7), Argument::Int(99)}),
                Op(42, {Argument::Int(7)}, {1, 0}),
                Host(52, {Argument::Ref(1, 0)}),
            }}}));
        CHECK(script.GetSharedVariable(7) == 99);
        CHECK(interface.observations.size() == 1 && interface.observations[0] == 99);
    }
    for (uint16_t operation : {uint16_t(38), uint16_t(40)}) {
        SimpleScript script;
        TestInterface interface;
        LoadAndRun(script, interface, MakeScript({}, {
            Function{{}, {}, {Host(operation, {Argument::Int(0)})}}}));
        CHECK(script.GetTaskState(0) == ScriptTaskState::Disabled);
    }
}

void TestInitializerAndReset()
{
    SimpleScript script;
    TestInterface interface;
    Function initializer{{}, {}, {Host(51)}};
    LoadAndRun(script, interface, MakeScript({}, {Function{{}, {}, {Host(51)}}}, &initializer));
    CHECK(script.GetState() == ScriptState::Running);
    CHECK(interface.calls == 1);

    SimpleScript initialized;
    const auto empty = MakeScript({}, {Function{{}, {}, {Host(51)}}});
    CHECK(initialized.Load(empty.data(), empty.size()) == S3D_OK);
    initialized.SetScriptInterface(&interface);
    initialized.Start();
    initialized.Update();
    CHECK(initialized.GetState() == ScriptState::Initialized);

    SimpleScript waiting;
    TestInterface waitInterface;
    Function waitingInitializer{{ScriptValueType::Bool}, {}, {Op(9, {Argument::Ref(2, 0)})}};
    waitingInitializer.commands[0].type = 0;
    waitingInitializer.commands[0].conditions = {Op(50, {}, {2, 0})};
    const auto waitingBytes = MakeScript({}, {}, &waitingInitializer);
    CHECK(waiting.Load(waitingBytes.data(), waitingBytes.size()) == S3D_OK);
    waiting.SetScriptInterface(&waitInterface);
    waiting.Start();
    waiting.Update();
    CHECK(waiting.GetState() == ScriptState::Initializing);
    waitInterface.condition = true;
    waiting.Update();
    CHECK(waiting.GetState() == ScriptState::Initialized);

    initialized.SetState(ScriptState::Running);
    initialized.SetSharedVariable(3, 10);
    initialized.EnableTask(0);
    initialized.Reset();
    CHECK(initialized.GetSharedVariable(3) == 0);
    CHECK(initialized.GetTaskState(0) == ScriptTaskState::Enabled);
    initialized.Unload();
    CHECK(initialized.GetFunctionCount() == 0 && initialized.GetVariableCount() == 0);
}

void TestRunScriptCommand()
{
#ifndef SOFT3D_SPK_ENGLISH_LAYOUT
    s3dResManager manager;
    CHECK(OpenGameResourceArchive(&manager, SOFT3D_TEST_ARCHIVE) == S3D_OK);
    SimpleScript script;
    TestInterface interface;
    CHECK(script.LoadFromPak(u".\\day1\\ep0", manager.GetFilePak()) == S3D_OK);
    const auto bytes = MakeScript({}, {
        Function{{}, {}, {Host(36, {Argument::String(".\\day1\\ep1010")})}}});
    LoadAndRun(script, interface, bytes);
    CHECK(script.GetState() == ScriptState::Initializing);
    CHECK(script.GetFunctionCount() == 9);
    CHECK(script.GetCommandCount() == 198);
#endif
}

} // namespace

int main()
{
    try {
        TestRealScripts();
        TestValuesAndMalformedInput();
        TestIntegerAndFixedArithmetic();
        TestComparisonsAndLogic();
        TestStructuredControlAndSuspension();
        TestTasksCallsAndGenericCommands();
        TestInitializerAndReset();
        TestRunScriptCommand();
        std::cout << "script tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
