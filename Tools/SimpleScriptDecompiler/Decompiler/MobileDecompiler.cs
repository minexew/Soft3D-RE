using SimpleScriptDecompiler.Script;
using SimpleScriptDecompiler.Script.Mobile;
using SimpleScriptDecompiler.Utils;
using System;
using static SimpleScriptDecompiler.Decompiler.CodeWriter;

namespace SimpleScriptDecompiler.Decompiler
{
    class MobileDecompiler : IDecompilerInterface
    {
        private CodeWriter writer;
        private bool externalFunctionMode;
        private bool specialCommandMode;

        public MobileDecompiler()
        {
            CommandsFile.LoadFromFile("commands_mobile.txt");
        }

        public void Decompile(IScriptInterface script, string name)
        {
            if (script == null)
            {
                Console.WriteLine("Decompiler: script is null");
                return;
            }
            SimpleScript ss = (SimpleScript)script;
            writer = new CodeWriter();
            writer.Open(name);

            if (ss.varsCount > 0)
                WriteVars(ss.vars);
            if (ss.constructorExist)
            {
                writer.WriteLine();
                ProcessFunction(ss.constructorFunction, -1);
            }
            for (int i = 0; i < ss.functionCount; i++)
            {
                writer.WriteLine();
                ProcessFunction(ss.functions[i], i);
            }
            writer.Close();
        }

        private void ProcessFunction(Function function, int index)
        {
            string name = index == -1
                ? Constants.CONSTRUCT_FUNC_NAME
                : DecompilerUtils.GetFuncName(index);
            writer.Write(name);
            writer.Write(Constants.NO_ARGS);
            writer.Write(Constants.COLON);
            writer.Write(Constants.START_ARGS);
            if (function.externalCommandCount > 0)
            {
                if (function.externalCommandCount != 1)
                    throw new Exception("ILLEGAL EXTERNAL COMMAND COUNT IN FUNCTION HEADER!");
                externalFunctionMode = true;
                ProcessCommand(function.exCommands[0]);
                externalFunctionMode = false;
            }
            writer.WriteLine(Constants.END_ARGS);
            writer.WriteLine(Constants.START_FUNC);
            writer.AddTabLevel();
            for (int i = 0; i < function.commandCount; i++)
            {
                ProcessCommandType(function.commands[i].type);
                ProcessCommand(function.commands[i]);
                writer.WriteLine();
            }
            ProcessCommandType(0);
            writer.RemoveTabLevel();
            writer.WriteLine(Constants.END_FUNC);
        }

        private void ProcessCommandType(ushort type)
        {
            switch (type)
            {
                case 0:
                    if (writer.GetMode() == WriterMode.SERIAL)
                        writer.SetMode(WriterMode.NORMAL);
                    break;
                case 1:
                    if (writer.GetMode() == WriterMode.NORMAL)
                        writer.SetMode(WriterMode.SERIAL);
                    break;
                default:
                    throw new Exception("Unknown command type! " + type);
            }
        }

        private void ProcessCommand(Command command)
        {
            if (command.id >= 7 && command.id <= 35)
            {
                ProcessInternalCommand(command);
                return;
            }
            DecompilerUtils.GetCommandName(command.id, out string name);
            writer.Write(name);
            writer.Write(Constants.START_ARGS);
            if (command.id >= 37 && command.id <= 40)
            {
                writer.Write("\"{0}\"", DecompilerUtils.GetFuncName((int)command.args[0]));
            }
            else
            {
                CheckArgsTypes(command);
                if (command.argsCount > 0)
                    writer.Write(DecompilerUtils.SeparateArgsAsString(command.args));
            }
            writer.Write(Constants.END_ARGS);
            if (!specialCommandMode && !externalFunctionMode)
                writer.Write(Constants.END_STATEMENT);
        }

        private void ProcessInternalCommand(Command command)
        {
            switch (command.id)
            {
                case 7:
                    ProcessConditional(command, Constants.IF);
                    break;
                case 8:
                    ProcessSwitch(command);
                    break;
                case 9:
                    ProcessWait(command);
                    break;
                case 10:
                    writer.Write("return();");
                    break;
                case 11:
                    writer.WriteLine(Constants.ELSE);
                    WriteBody(command.body, command.bodyCount);
                    break;
                case 12:
                    writer.WriteLine("case {0}:", command.args[0]);
                    WriteBody(command.body, command.bodyCount);
                    break;
                case 13:
                    ProcessConditional(command, "while");
                    break;
                case 14:
                    writer.Write("end();");
                    break;
                case 16:
                case 17:
                case 18:
                case 19:
                case 20:
                case 21:
                case 22:
                case 23:
                    writer.Write("{0} {1} {2}", command.args[0], Operator(command.id), command.args[1]);
                    break;
                case 24:
                    writer.Write("!{0}", command.args[0]);
                    break;
                case 25:
                    writer.Write("{0} = {1};", command.result, command.args[0]);
                    break;
                case 26:
                case 27:
                case 28:
                case 29:
                case 30:
                    writer.Write("{0} {1}= {2};", command.result,
                        Operator((ushort)(command.id + 5)), command.args[0]);
                    break;
                case 31:
                case 32:
                case 33:
                case 34:
                case 35:
                    writer.Write("{0} = {1} {2} {3};", command.result, command.args[0],
                        Operator(command.id), command.args[1]);
                    break;
                default:
                    writer.Write("Unknown internal command {0};", command.id);
                    break;
            }
        }

        private static string Operator(ushort id)
        {
            switch (id)
            {
                case 16: return "==";
                case 17: return "<=";
                case 18: return ">=";
                case 19: return "<";
                case 20: return ">";
                case 21: return "!=";
                case 22: return "&&";
                case 23: return "||";
                case 31: return "+";
                case 32: return "-";
                case 33: return "*";
                case 34: return "/";
                case 35: return "%";
                default: throw new ArgumentException("Illegal operator ID! " + id);
            }
        }

        private void ProcessConditional(Command command, string name)
        {
            writer.WriteLine();
            writer.Write(name);
            writer.Write(Constants.START_ARGS);
            ProcessConditions(command);
            writer.Write(Constants.END_ARGS);
            WriteBody(command.body, command.bodyCount);
        }

        private void ProcessWait(Command command)
        {
            if (externalFunctionMode)
            {
                ProcessConditions(command);
                return;
            }
            writer.Write(Constants.WAIT);
            writer.Write(Constants.START_ARGS);
            writer.Write(Constants.SPACE);
            ProcessConditions(command);
            writer.Write(Constants.SPACE);
            writer.Write(Constants.END_ARGS);
            writer.Write(Constants.END_STATEMENT);
        }

        private void ProcessSwitch(Command command)
        {
            writer.WriteLine();
            writer.Write("switch({0})", command.args[0]);
            WriteBody(command.body, command.bodyCount);
        }

        private void ProcessConditions(Command command)
        {
            specialCommandMode = true;
            ProcessCommands(command.conditions, command.conditionCount, false);
            specialCommandMode = false;
        }

        private void WriteBody(Command[] commands, ushort count)
        {
            writer.WriteLine(Constants.START_FUNC);
            writer.AddTabLevel();
            ProcessCommands(commands, count, true);
            writer.RemoveTabLevel();
            writer.WriteLine(Constants.END_FUNC);
        }

        private void ProcessCommands(Command[] commands, ushort count, bool lines)
        {
            for (int i = 0; i < count; i++)
            {
                ProcessCommand(commands[i]);
                if (lines)
                    writer.WriteLine();
            }
        }

        private void WriteVars(Type[] vars)
        {
            if (vars.Length == 0)
                return;
            writer.WriteLine(Constants.VARS);
            writer.WriteLine(Constants.START_FUNC);
            writer.AddTabLevel();
            for (int i = 0; i < vars.Length; i++)
                writer.WriteLine("({0}) {1};", TypeNamesUtils.ConvertCSharp2C(vars[i].Name),
                    DecompilerUtils.GetVarName(i));
            writer.RemoveTabLevel();
            writer.WriteLine(Constants.END_FUNC);
        }

        public void CheckArgsTypes(Command command)
        {
            if (!CommandsFile.IsLoaded)
                return;
            var c = CommandsFile.GetCommand(command.id);
            if (c == null || !c.HasArgs())
                return;
            if (c.Args.Length != command.argsCount)
            {
                Console.WriteLine("Warning! Wrong args count in command {0}!", c.Name);
                return;
            }
            for (int i = 0; i < command.argsCount; i++)
            {
                object arg = command.args[i];
                if (arg is ScriptReference)
                    continue;
                if (arg.GetType() != c.Args[i] && c.Args[i] != typeof(short) &&
                    c.Args[i] != typeof(int) && c.Args[i] != typeof(byte))
                    Console.WriteLine("Warning! arg {0} has incorrect type {1}! Line: {2}",
                        arg, arg.GetType(), writer.GetLineIndex());
            }
        }
    }
}
