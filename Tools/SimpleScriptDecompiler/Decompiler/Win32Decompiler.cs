using SimpleScriptDecompiler.Script;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using static SimpleScriptDecompiler.Script.Win32SimpleScript;

namespace SimpleScriptDecompiler.Decompiler
{
    class Win32Decompiler : IDecompilerInterface
    {
        private readonly bool namesDone;
        private readonly CodeWriter writer;

        private readonly List<int> bracesQueue;

        public Win32Decompiler()
        {
            writer = new CodeWriter();
            namesDone = CommandsFile.Init("commands_win32.txt");
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
                writer.WriteLine("\n\n");
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
            StringBuilder line = new StringBuilder();
            string commandName = "Command[" + command.id.ToString() + "]";
            if (namesDone)
            {
                string temp = CommandsFile.GetCommandName(command.id);
                if (temp != null)
                    commandName = temp;
            }

            line.Append(string.Format("{0}(", commandName));

            if (command.id == 1 || command.id == 2) //enableTask or disableTask
            {
                if (command.args.Length != 1)
                    throw new Exception("Illegal args count on spec functions!");
                line.Append(DecompilerUtils.GetFuncName((short)command.args[0]));
            }
            else
            {
                if (command.argsCount > 0)
                {
                    line.Append(" ");
                    line.Append(DecompilerUtils.SeparateArgsAsString(command.args));
                    line.Append(" ");
                }
            }
            line.Append(")");

            if (command.conditionExist > 0)
            {
                switch (command.conditionType)
                {
                    case 0:
                        line.Append(" == ");
                        line.Append(command.conditionValue);
                        break;
                    case 4:
                        line.Append(" >= ");
                        line.Append(command.conditionValue);
                        break;
                    default:
                        line.Append(string.Format(" UNK CONDITION TYPE BYTE {0} INT {1}", command.conditionType, command.conditionValue));
                        break;
                }
            }

            switch (command.type)
            {
                case 0:
                    writer.WriteLine(line.ToString() + ";");
                    break;
                case 1:
                    writer.WriteLine("if( {0} )", line.ToString());
                    break;
                case 3:
                    writer.WriteLine("wait( {0} );", line.ToString());
                    break;
                default:
                    writer.WriteLine("UNK TYPE {0} {1}", command.type, line.ToString());
                    break;
            }

            if (command.unkCount1 > 0)
            {
                writer.WriteLine("UNK INT {0}", BitConverter.ToInt32(command.unk1, 0));
            }

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
                writer.WriteLine("{ //" + lineCount);
                writer.AddTabLevel();
            }

        }

    }
}
