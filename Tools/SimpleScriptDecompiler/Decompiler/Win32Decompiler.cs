using SimpleScriptDecompiler.Script;
using SimpleScriptDecompiler.Script.Win32;
using SimpleScriptDecompiler.Utils;
using System;
using System.Collections.Generic;

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

        private void Reset()
        {
            if (bracesQueue != null)
                bracesQueue.Clear();
        }

        public void Decompile(IScriptInterface script, string name)
        {
            if (script == null)
            {
                Console.WriteLine("Decompiler: script is null");
                return;
            }

            Reset();

            SimpleScript ss = (SimpleScript)script;
            //TODO: Check normal cast

            writer.Open(name);

            for (int i = 0; i < ss.functionsCount; i++)
            {
                ProcessFunction(ss.functions[i], i);
                if (i < ss.functionsCount - 1) //if func != last
                    writer.WriteLine(); //add new line
            }

            writer.Close();
        }

        private void ProcessFunction(Function func, int index)
        {
            string name = DecompilerUtils.GetFuncName(index);
            writer.WriteLine("{0}{1}", name, Constants.NO_ARGS);

            writer.WriteLine(Constants.START_FUNC);
            writer.AddTabLevel();
            for (int i = 0; i < func.commands.Length; i++)
            {
                ProcessCommand(func.commands[i]);
            }
            writer.RemoveTabLevel();
            writer.WriteLine(Constants.END_FUNC);
        }

        private void ProcessCommand(Command command)
        {
            ProcessStartCommand(command.type);
            ProcessCommandName(command);

            writer.Write(Constants.START_ARGS);
            ProcessCommandArgs(command);
            writer.Write(Constants.END_ARGS);

            ProcessCommandCondition(command);

            if (command.unkCount1 > 0)
            {
                writer.Warning(" UNK INT {0}",
                    BitConverter.ToInt32(command.unk1, 0));
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
                    writer.Write(Constants.IF);
                    writer.Write(Constants.START_ARGS);
                    break;
                case 3:
                    writer.Write(Constants.WAIT);
                    writer.Write(Constants.START_ARGS);
                    break;
                default:
                    writer.Warning("UNK TYPE {0}", type);
                    break;
            }
            writer.Write(Constants.SPACE);
        }

        private void ProcessCommandName(Command command)
        {
            //if command not exist in names file
            if (!DecompilerUtils.GetCommandName(command.id, out string name))
            {
                Console.WriteLine("Unknown command [{0}] on line: {1}",
                        command.id,
                        writer.GetLineIndex());
            }
            writer.Write(name);
        }

        private void ProcessCommandArgs(Command command)
        {
            if(IsSpecialCommand(command))
            {
                ProcessSpecialCommandArgs(command);
            }
            else
            {
                CheckArgsTypes(command);
                ProcessStandardCommandArgs(command.args, command.argsCount);
            }
        }

        private bool IsSpecialCommand(Command command)
        {
            return command.id < Constants.SPECIAL_COMMANDS_COUNT;
        }

        private void ProcessStandardCommandArgs(object[] args, int count)
        {
            if (args == null)
                return;
            if (count != args.Length)
                throw new ArgumentException("Illegal args count!");

            if (count > 0)
            {
                writer.Write(" {0} ", DecompilerUtils.SeparateArgsAsString(args));
            }
        }

        private void ProcessSpecialCommandArgs(Command command)
        {
            switch(command.id)
            {
                case 1: //enableTask
                case 2: //disableTask
                    if (command.args.Length != 1)
                        throw new ArgumentException("Illegal args count on spec functions!");
                    string taskName = DecompilerUtils.GetFuncName((short)command.args[0]);

                    writer.Write("\"{0}\"", taskName);
                    break;
                default:
                    ProcessStandardCommandArgs(command.args, command.argsCount);
                    break;

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
                case 1:
                    writer.Write(" <= {0}", command.conditionValue);
                    break;
                case 2: //check this shit
                    writer.Write(" == {0} || {0} == 0", command.conditionValue);
                    break;
                case 3: //check this
                    writer.Write(" < {0}", command.conditionValue);
                    break;
                case 4:
                    writer.Write(" >= {0}", command.conditionValue);
                    break;
                case 5:
                    writer.Write(" != {0}", command.conditionValue);
                    break;
                default:
                    writer.Warning("UNK CONDITION TYPE BYTE {0} INT {1}",
                        command.conditionType,
                        command.conditionValue);
                    return;
            }

            //command.conditionValue
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
                        writer.WriteLine(Constants.END_FUNC);
                    }
                }
            }

            if (command.braceExist > 0)
            {
                int lineCount = BitConverter.ToInt16(command.braceLineCount, 0) - 1;
                bracesQueue.Add(lineCount);
                if (command.type != 1) //else
                    writer.WriteLine(Constants.ELSE);
                writer.WriteLine(Constants.START_FUNC);
                writer.AddTabLevel();
            }
        }

        private void ProcessEndCommand(int type)
        {
            switch (type)
            {
                case 1:
                    writer.Write(Constants.SPACE);
                    writer.Write(Constants.END_ARGS);
                    break;
                case 3:
                    writer.Write(Constants.SPACE);
                    writer.Write(Constants.END_ARGS);
                    writer.Write(Constants.END_STATEMENT);
                    break;
                default:
                    writer.Write(Constants.END_STATEMENT);
                    break;
            }
            writer.WriteLine();
        }

        //Move to Utils
        public void CheckArgsTypes(Command command)
        {
            if (!CommandsFile.IsLoaded)
                return;
            var c = CommandsFile.GetCommand(command.id);

            if (c == null || !c.HasArgs())
                return;

            if (c.Args.Length != command.argsCount)
            {
                Console.WriteLine("Warning! Wrong args count in command {0}! Line: {1}",
                    c.Name,
                    writer.GetLineIndex());
                return;
            }

            for (int i = 0; i < command.argsCount; i++)
            {
                if (command.args[i].GetType() != c.Args[i])
                    Console.WriteLine("Warning! arg {0} has incorrect type {1}! Line: {2}",
                        command.args[i],
                        command.args[i].GetType(),
                        writer.GetLineIndex());
            }
        }
    }
}