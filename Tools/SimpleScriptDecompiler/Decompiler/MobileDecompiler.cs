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
        private bool externalFunctionMode; //enum
        private bool specialCommandMode; //enum

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
            //TODO: Check normal cast

            
            writer = new CodeWriter();
            writer.Open(name);

            if(ss.varsCount > 0)
            {
                WriteVars(ss.vars);
            }

            if(ss.constructorExist)
            {
                writer.WriteLine();
                ProcessConstructor(ss.constructorFunction);
            }

            for(int i = 0; i < ss.functionCount; i++)
            {
                writer.WriteLine();
                ProcessFunction(ss.functions[i], i);
            }

            writer.Close();
        }

        private void ProcessConstructor(Function function)
        {
            ProcessFunction(function, -1);
        }

        private void ProcessFunction(Function function, int index)
        {
            ProcessStartCommand(index);

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

            if (function.varsCount > 0) //not needed to decompiler
            {
                //WriteVars(function.vars);
            }

            for (int i = 0; i < function.commandCount; i++)
            {
                ProcessCommandType(function.commands[i].type);
                ProcessCommand(function.commands[i]);
                writer.WriteLine();
            }

            CheckSerialModeClosed();

            writer.RemoveTabLevel();
            writer.WriteLine(Constants.END_FUNC);
        }

        private void ProcessStartCommand(int commandIndex)
        {
            string name;
            if (commandIndex == -1)
                name = Constants.CONSTRUCT_FUNC_NAME;
            else
                name = DecompilerUtils.GetFuncName(commandIndex);

            writer.Write(name);
            writer.Write(Constants.NO_ARGS);
            writer.Write(Constants.COLON);
        }

        private void ProcessCommandType(ushort type)
        {
            switch (type)
            {
                case 0: //Default type
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

        private void CheckSerialModeClosed()
        {
            //check if serial mode opened on end function
            ProcessCommandType(0);
        }


        private void ProcessCommand(Command command)
        {
            if (IsSpecialCommand(command) && !IsSpecialCommandArgsOnly(command))
            {
                ProcessSpecialCommand(command);
                    return;
            }



            DecompilerUtils.GetCommandName(command.id, out string name);

            writer.Write(name);

            writer.Write(Constants.START_ARGS);

            ProcessCommandArgs(command);
   
            writer.Write(Constants.END_ARGS);
            if(!specialCommandMode && !externalFunctionMode)
                writer.Write(Constants.END_STATEMENT);
        }

        private bool IsSpecialCommand(Command command)
        {
            return command.id - 36 < Constants.SPECIAL_COMMANDS_COUNT;
        }

        private bool IsSpecialCommandArgsOnly(Command command)
        {
            return IsSpecialCommand(command) && command.id >= 36;
        }

        private void ProcessSpecialCommand(Command command)
        {
            switch (command.id)
            {
                case 7: //if
                    ProcessSpecialCommandIF(command);
                    break;
                case 9: //wait
                    ProcessSpecialCommandWAIT(command);
                    break;
                case 11: //else
                    ProcessSpecialCommandELSE(command);
                    break;
                case 16: //==
                    writer.Write(" == {0}", command.args[1]);
                    ProcessExternalCommands(command, 2, false);
                    break;
                case 18: //>=
                    writer.Write(" >= {0}", command.args[1]);
                    ProcessExternalCommands(command, 2, false);
                    break;
                //case 22: //maybe &&
                    //break;
                case 25:
                    writer.Write("{0} = {1};", DecompilerUtils.GetVarName(command.unk2_data2), command.args[0]);
                    break;
                default:
                    writer.Write("Unknown special command id: " + command.id);
                    if(command.argsCount > 0)
                    {
                        writer.Write(" args: ");
                        writer.Write(DecompilerUtils.SeparateArgsAsString(command.args));
                    }
                    if (command.unk2 == 6)
                    {
                        writer.Write(" UNK2 DATA {0} {1} {2}", command.unk2_data1, command.unk2_data2, command.unk2_data3);
                    }
                    ProcessExternalCommands(command, 1, true);
                    ProcessExternalCommands(command, 2, true);
                    break;

            }
        }

        private void ProcessSpecialCommandIF(Command command)
        {
            writer.WriteLine();
            writer.Write(Constants.IF);

            writer.Write(Constants.START_ARGS);
            specialCommandMode = true;
            ProcessExternalCommands(command, 2, false);
            specialCommandMode = false;
            writer.Write(Constants.END_ARGS);

            writer.WriteLine(Constants.START_FUNC);
            ProcessExternalCommands(command, 1, true);
            writer.WriteLine(Constants.END_FUNC);
        }

        private void ProcessSpecialCommandWAIT(Command command)
        {
            //if external, command "wait" magic convert to empty
            if (externalFunctionMode)
            {
                ProcessExternalCommands(command, 2, false);
                return;
            }
            writer.Write(Constants.WAIT);
            writer.Write(Constants.START_ARGS);
            writer.Write(Constants.SPACE);

            specialCommandMode = true;
            ProcessExternalCommands(command, 2, false);
            specialCommandMode = false;

            writer.Write(Constants.SPACE);
            writer.Write(Constants.END_ARGS);
            writer.Write(Constants.END_STATEMENT);
        }

        private void ProcessSpecialCommandELSE(Command command)
        {
            writer.WriteLine(Constants.ELSE);
            writer.WriteLine(Constants.START_FUNC);
            ProcessExternalCommands(command, 1, true);
            writer.WriteLine(Constants.END_FUNC);
        }

        private void ProcessCommandArgs(Command command)
        {
            if (IsSpecialCommandArgsOnly(command))
            {
                ProcessSpecialCommandArgs(command);
                return;
            }

            CheckArgsTypes(command);

            if (command.argsCount > 0)
            {
                writer.Write(DecompilerUtils.SeparateArgsAsString(command.args));
            }
        }

        private void ProcessSpecialCommandArgs(Command command)
        {
            switch(command.id)
            {
                case 37: //enableTask
                case 38: //disableTask
                case 39: //runTask
                case 40: //intermitTask
                    writer.Write("\"{0}\"", DecompilerUtils.GetFuncName((int)command.args[0]));
                    break;
                default:
                    throw new Exception("Illegal special command ID! " + command.id);
            }
        }


        private void ProcessExternalCommands(Command command, int index, bool separatorIsNeeded)
        {
            ushort count;
            Command[] exCommands;
            switch(index)
            {
                case 1:
                    count = command.exCommand1Count;
                    exCommands = command.ex1Commands;
                    break;
                case 2:
                    count = command.exCommand2Count;
                    exCommands = command.ex2Commands;
                    break;
                default:
                    throw new ArgumentException("Illegal external command index! " + index);
            }

            if (count > 0)
            {
                writer.AddTabLevel();
                for (int i = 0; i < count; i++)
                {
                    ProcessCommand(exCommands[i]);
                    if (separatorIsNeeded)
                        writer.WriteLine();
                }
                writer.RemoveTabLevel();
            }
        }

        private void WriteVars(Type[] vars)
        {
            if (vars.Length == 0)
                return;

            writer.WriteLine(Constants.VARS);
            writer.WriteLine(Constants.START_FUNC);
            writer.AddTabLevel();
            for(int i = 0; i < vars.Length; i++)
            {
                writer.WriteLine("({0}) {1};",
                    TypeNamesUtils.ConvertCSharp2C(vars[i].Name),
                    DecompilerUtils.GetVarName(i));
            }
            writer.RemoveTabLevel();
            writer.WriteLine(Constants.END_FUNC);
        }

        public void CheckArgsTypes(Command command)
        {
            if (CommandsFile.IsLoaded) //check args
            {
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

                    if (arg.GetType() != c.Args[i] &&
                        c.Args[i] != typeof(short) && //in mobile rev "short" = "int" = "byte"
                        c.Args[i] != typeof(int)   &&
                        c.Args[i] != typeof(byte))
                        Console.WriteLine("Warning! arg {0} has incorrect type {1}! Line: {2}",
                            arg,
                            arg.GetType(),
                            writer.GetLineIndex());
                }
            }
        }
    }
}
