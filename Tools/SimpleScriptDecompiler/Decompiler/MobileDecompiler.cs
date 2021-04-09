using SimpleScriptDecompiler.Script;
using SimpleScriptDecompiler.Utils;
using System;
using static SimpleScriptDecompiler.Decompiler.CodeWriter;
using static SimpleScriptDecompiler.Script.MobileSimpleScript;

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
            MobileSimpleScript ss = (MobileSimpleScript)script;
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
                WriteFunction(ss.constructorFunction, -1);
            }

            for(int i = 0; i < ss.functionCount; i++)
            {
                writer.WriteLine();
                WriteFunction(ss.functions[i], i);
            }

            writer.Close();
        }

        private void WriteFunction(Function function, int index)
        {
            string name;
            if (index == -1)
                name = "init";
            else
                name = DecompilerUtils.GetFuncName(index);

            writer.Write(name + "():(");
            if(function.externalCommandCount > 0)
            {
                if (function.externalCommandCount != 1)
                    throw new Exception("ILLEGAL EXTERNAL COMMAND COUNT IN FUNCTION HEADER!");
                externalFunctionMode = true;
                WriteCommand(function.exCommands[0]);
                externalFunctionMode = false;
            }
            writer.WriteLine(")");


            writer.WriteLine("{");
            writer.AddTabLevel();

            if(function.varsCount > 0) //not needed to decompiler
            {
                //WriteVars(function.vars);
            }

            for(int i = 0; i < function.commandCount; i++)
            {
                ProcessCommandType(function.commands[i].type);
                WriteCommand(function.commands[i]);
                writer.WriteLine();
            }

            CheckSeialModeClosed();

            writer.RemoveTabLevel();
            writer.WriteLine("}");
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

        private void CheckSeialModeClosed()
        {
            //check if serial mode opened on end function
            ProcessCommandType(0);
        }


        private void WriteCommand(Command command)
        {
            if (command.id < 39)
            {
                ProcessSpecialCommand(command);
                return;
            }

            string name = string.Format("Command[{0}]", command.id);

            if (CommandsFile.IsLoaded())
            {
                var c  = CommandsFile.GetCommand(command.id);
                if (c != null && c.Name != null)
                    name = c.Name;
            }

            writer.Write(name);

            writer.Write("(");
            ProcessCommandArgs(command);
            writer.Write(")");
            if(!specialCommandMode && !externalFunctionMode)
                writer.Write(";");
        }



        private void ProcessSpecialCommand(Command command)
        {
            switch (command.id)
            {
                case 7: //if
                    writer.WriteLine();
                    writer.Write("if(");
                    specialCommandMode = true;
                    ProcessExternalCommands(command, 2, false);
                    specialCommandMode = false;
                    writer.WriteLine(")");
                    writer.WriteLine("{");
                    ProcessExternalCommands(command, 1, true);
                    writer.WriteLine("}");
                    break;
                case 9: //wait

                    if(externalFunctionMode) //if external, command "wait" magic convert to empty
                    {
                        ProcessExternalCommands(command, 2, false);
                        return;
                    }
                    writer.Write("wait( ");
                    specialCommandMode = true;
                    ProcessExternalCommands(command, 2, false);
                    specialCommandMode = false;
                    writer.WriteLine(" );");

                    break;
                case 11: //else
                    writer.WriteLine("else");
                    writer.WriteLine("{");
                    ProcessExternalCommands(command, 1, true);
                    writer.WriteLine("}");
                    break;
                case 16: //==
                    writer.Write(" == " + command.args[1]);
                    ProcessExternalCommands(command, 2, false);
                    break;
                case 18: //>=
                    writer.Write(" >= " + command.args[1]);
                    ProcessExternalCommands(command, 2, false);
                    break;
                //case 22: //maybe &&
                    //break;

                //Next command is not special, but it needs refactoring
                case 25:
                    writer.Write("{0} = {1};", DecompilerUtils.GetVarName(command.unk2_data2), command.args[0]);
                    break;
                case 37:
                    writer.Write("enableTask(\"{0}\");", DecompilerUtils.GetFuncName((int)command.args[0]));
                    break;
                case 38:
                    writer.Write("disableTask(\"{0}\");", DecompilerUtils.GetFuncName((int)command.args[0]));
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

        private void ProcessCommandArgs(Command command)
        {
            CheckArgsTypes(command);

            if (command.argsCount > 0)
            {
                writer.Write(DecompilerUtils.SeparateArgsAsString(command.args));
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
                    WriteCommand(exCommands[i]);
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

            writer.WriteLine("VarName");
            writer.WriteLine("{");
            writer.AddTabLevel();
            for(int i = 0; i < vars.Length; i++)
            {
                writer.WriteLine("({0}) {1};",
                    TypeNamesUtils.ConvertCSharp2C(vars[i].Name),
                    DecompilerUtils.GetVarName(i));
            }
            writer.RemoveTabLevel();
            writer.WriteLine("}");
        }

        //Move to Utils
        public void CheckArgsTypes(Command command)
        {
            if (CommandsFile.IsLoaded()) //check args
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
