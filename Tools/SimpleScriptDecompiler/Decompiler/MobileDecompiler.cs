using SimpleScriptDecompiler.Script;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using static SimpleScriptDecompiler.Decompiler.CodeWriter;
using static SimpleScriptDecompiler.Script.MobileSimpleScript;

namespace SimpleScriptDecompiler.Decompiler
{
    class MobileDecompiler : IDecompilerInterface
    {
        private CodeWriter writer;
        private bool namesDone;
        private bool externalFunctionMode;

        public MobileDecompiler()
        {
            namesDone = CommandsFile.Init("commands_mobile.txt");
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
                switch (function.commands[i].type)
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
                        throw new Exception("Unknown command type! " + function.commands[i].type);
                }
                WriteCommand(function.commands[i]);
                writer.WriteLine();
            }

            //check if serial mode opened on end function
            if(writer.GetMode() == WriterMode.SERIAL)
            {
                writer.SetMode(WriterMode.NORMAL);
            }

            writer.RemoveTabLevel();
            writer.WriteLine("}");
        }


        private void WriteCommand(Command command)
        {
            if (command.id < 36)
            {
                WriteSpecialsCommand(command);
                return;
            }
            string name = string.Format("Command[{0}]", command.id);
            if (namesDone)
            {
                string temp = CommandsFile.GetCommandName(command.id);
                if (temp != null)
                    name = temp;
            }

            writer.Write(name);
            writer.Write("(");
            if (command.argsCount > 0)
            {
                //writer.Write(" ");
                writer.Write(DecompilerUtils.SeparateArgsAsString(command.args));
            }
            
            writer.Write(")");
        }



        private void WriteSpecialsCommand(Command command)
        {
            switch (command.id)
            {
                case 7: //if
                    writer.WriteLine();
                    writer.Write("if(");
                    WriteExCommands2(command, false);
                    writer.WriteLine(")");
                    writer.WriteLine("{");
                    WriteExCommands1(command, true);
                    writer.WriteLine("}");
                    break;
                case 9: //wait

                    if(externalFunctionMode) //if external, command "wait" magic convert to empty
                    {
                        WriteExCommands2(command, false);
                        return;
                    }
                    writer.Write("wait( ");
                    WriteExCommands2(command, false);
                    writer.WriteLine(" )");

                    break;
                case 11: //else
                    writer.WriteLine("else");
                    writer.WriteLine("{");
                    WriteExCommands1(command, true);
                    writer.WriteLine("}");
                    break;
                case 16: //==
                    writer.Write(" == " + command.args[1]);
                    WriteExCommands2(command, false);
                    break;
                case 18: //>=
                    writer.Write(" >= " + command.args[1]);
                    WriteExCommands2(command, false);
                    break;
                //case 22: //maybe &&
                    //break;
                case 25: //This command is not special, but it needs refactoring
                    writer.Write("{0} = {1}", DecompilerUtils.GetVarName(command.unk2_data2), command.args[0]);
                    break;
                default: //??
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
                    WriteExCommands1(command, true);
                    WriteExCommands2(command, true);
                    break;

            }
        }

        private void WriteExCommands1(Command command, bool separatorIsNeeded)
        {
            if (command.exCommand1Count > 0)
            {
                //writer.WriteLine();
                //writer.WriteLine("EXTERNAL 1 COMMANDS COUNT: {0}", command.exCommand1Count);
                writer.AddTabLevel();
                for (int i = 0; i < command.exCommand1Count; i++)
                {
                    WriteCommand(command.ex1Commands[i]);
                    if (separatorIsNeeded)
                        writer.WriteLine();
                }
                writer.RemoveTabLevel();
            }
        }

        private void WriteExCommands2(Command command, bool separatorIsNeeded)
        {
            if (command.exCommand2Count > 0)
            {
                //writer.WriteLine();
                //writer.WriteLine("EXTERNAL 2 COMMANDS COUNT: {0}", command.exCommand2Count);
                writer.AddTabLevel();
                for (int i = 0; i < command.exCommand2Count; i++)
                {
                    WriteCommand(command.ex2Commands[i]);
                    if (separatorIsNeeded)
                        writer.WriteLine();
                }
                //writer.WriteLine();
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
                writer.WriteLine("({0}) {1};", vars[i].Name, DecompilerUtils.GetVarName(i)); //remade
            }
            writer.RemoveTabLevel();
            writer.WriteLine("}");
        }
    }
}
