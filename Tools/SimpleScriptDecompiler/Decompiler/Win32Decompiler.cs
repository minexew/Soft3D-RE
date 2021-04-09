using SimpleScriptDecompiler.Script;
using SimpleScriptDecompiler.Utils;
using System;
using System.Collections.Generic;
using static SimpleScriptDecompiler.Script.Win32SimpleScript;

namespace SimpleScriptDecompiler.Decompiler
{
    class Win32Decompiler : IDecompilerInterface
    {
        private readonly CodeWriter writer;
        private readonly List<int> bracesQueue;


        public Win32Decompiler()
        {
            writer = new CodeWriter();
            CommandsFile.LoadFromFile("commands_win32.txt");
            bracesQueue = new List<int>();
        }

        public void Decompile(IScriptInterface script, string name)
        {
            if (script == null)
            {
                Console.WriteLine("Decompiler: script is null");
                return;
            }

            Win32SimpleScript ss = (Win32SimpleScript)script;
            //TODO: Check normal cast

            writer.Open(name);

            for (int i = 0; i < ss.functions.Length; i++)
            {
                WriteFunction(ss.functions[i], i);
                if (i < ss.functions.Length - 1) //if func != last
                    writer.WriteLine();
            }

            writer.Close();
        }

        private void WriteFunction(Function func, int index)
        {
            writer.WriteLine(DecompilerUtils.GetFuncName(index) + "()");
            writer.WriteLine("{");
            writer.AddTabLevel();
            for (int i = 0; i < func.commands.Length; i++)
            {
                WriteCommand(func.commands[i]);
            }
            writer.RemoveTabLevel();
            writer.WriteLine("}");
        }

        private void WriteCommand(Command command)
        {

            ProcessStartCommand(command.type);

            ProcessCommandName(command);

            writer.Write("(");
            ProcessCommandArgs(command);
            writer.Write(")");

            ProcessCommandCondition(command);

            if (command.unkCount1 > 0)
            {
                writer.WriteWarning(" UNK INT {0}", BitConverter.ToInt32(command.unk1, 0));
            }

            ProcessEndCommand(command.type);

            ProcessBraces(command);
        }

        private void ProcessStartCommand(byte type)
        {
            switch (type)
            {
                case 0: //Ignore
                    return;
                case 1:
                    writer.WriteLine();
                    writer.Write("if(");
                    break;
                case 3:
                    writer.Write("wait(");
                    break;
                default:
                    writer.WriteWarning("UNK TYPE {0}", type);
                    break;
            }
            writer.Write(" ");
        }

        private void ProcessCommandName(Command command)
        {
            string commandName = "Command[" + command.id.ToString() + "]";

            if (CommandsFile.IsLoaded())
            {
                var c = CommandsFile.GetCommand(command.id);
                if (c != null && c.Name != null)
                    commandName = c.Name;
            }
            writer.Write(commandName);
        }

        private void ProcessCommandArgs(Command command)
        {
            if (command.id == 1 || command.id == 2) //enableTask or disableTask
            {
                if (command.args.Length != 1)
                    throw new Exception("Illegal args count on spec functions!");
                string funcName = DecompilerUtils.GetFuncName((short)command.args[0]);
                writer.Write("\"{0}\"", funcName);
            }
            else
            {
                CheckArgsTypes(command);
                if (command.argsCount > 0)
                {
                    writer.Write(" {0} ", DecompilerUtils.SeparateArgsAsString(command.args));
                }
            }
        }

        private void ProcessCommandCondition(Command command)
        {
            if (!command.conditionExist)
                return;

            switch (command.conditionType)
            {
                case 0:
                    writer.Write(" == {0}", command.conditionValue);
                    break;
                case 4:
                    writer.Write(" >= {0}", command.conditionValue);
                    break;
                default:
                    writer.WriteWarning("UNK CONDITION TYPE BYTE {0} INT {1}", command.conditionType, command.conditionValue);
                    break;
            }
        }

        private void ProcessBraces(Command command)
        {
            if (bracesQueue.Count > 0)
            {
                for (int i = 0; i < bracesQueue.Count; i++)
                {
                    if (--bracesQueue[i] <= 0)
                    {
                        bracesQueue.RemoveAt(i--);
                        writer.RemoveTabLevel();
                        writer.WriteLine("}");
                    }
                }
            }

            if (command.braceExist > 0)
            {
                int lineCount = BitConverter.ToInt16(command.braceLineCount, 0) - 1;
                bracesQueue.Add(lineCount);
                if (command.type != 1) //else
                    writer.WriteLine("else");
                writer.WriteLine("{");
                writer.AddTabLevel();
            }
        }

        private void ProcessEndCommand(int type)
        {
            switch (type)
            {
                case 1:
                    writer.WriteLine(" )");
                    break;
                case 3:
                    writer.WriteLine(" );");
                    break;
                default:
                    writer.WriteLine(";");
                    break;
            }
        }

        //Move to Utils
        public void CheckArgsTypes(Command command)
        {
            if (CommandsFile.IsLoaded()) //check args
            {
                var c = CommandsFile.GetCommand(command.id);
                if (c != null && c.HasArgs())
                {
                    if (c.Args.Length != command.argsCount)
                        Console.WriteLine("Warning! Wrong args count in command {0}!", c.Name);
                    else
                        for (int i = 0; i < command.argsCount; i++)
                            if (command.args[i].GetType() != c.Args[i])
                                Console.WriteLine("Warning! arg {0} has incorrect type {1}! Line: {2}",
                                    command.args[i],
                                    command.args[i].GetType(),
                                    writer.GetLineIndex());
                }
            }
        }
    }
}