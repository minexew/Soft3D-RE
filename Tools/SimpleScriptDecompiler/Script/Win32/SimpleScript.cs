using SimpleScriptDecompiler.IO;
using SimpleScriptDecompiler.Utils;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace SimpleScriptDecompiler.Script.Win32
{
    class SimpleScript : IScriptInterface
    {
        //SimpleScriptStruct
        public int functionsCount;
        public Function[] functions;
        //SimpleScriptStruct end

        private CustomBinaryReader reader;


        public void LoadFromFile(string path)
        {
            reader = new CustomBinaryReader(File.OpenRead(path));
            functionsCount = reader.ReadUInt16();
            if (functionsCount > 0)
            {
                functions = new Function[functionsCount];
                for (int i = 0; i < functionsCount; i++)
                {
                    functions[i] = ReadFunction();
                }
            }
            reader.Close();
        }

        private Function ReadFunction()
        {
            Function func = new Function();
            func.commandsCount = reader.ReadUInt16();
            if (func.commandsCount > 0)
            {
                func.commands = new Command[func.commandsCount];
                for (int i = 0; i < func.commandsCount; i++)
                {
                    func.commands[i] = ReadCommand();
                }
            }
            return func;
        }

        private Command ReadCommand()
        {
            Command command = new Command();
            command.type = reader.ReadByte();
            command.id = reader.ReadByte();
            command.argsCount = reader.ReadByte();

            if (command.argsCount > 0)
            {
                command.args = new object[command.argsCount];
                for (int i = 0; i < command.argsCount; i++)
                {
                    command.args[i] = ReadCommandArg();
                }
            }


            command.unkCount1 = reader.ReadByte();

            if (command.unkCount1 > 0) //always 0 or 1?
            {
                command.unk1 = reader.ReadBytes(4 * command.unkCount1); //int?
            }

            command.braceExist = reader.ReadByte();

            if (command.braceExist > 0) //always 0 or 1?
            {
                command.braceLineCount = reader.ReadBytes(sizeof(short) * command.braceExist); //short?
            }

            if (command.type == 1 || command.type == 3)
            {
                command.conditionExist = reader.ReadBoolean();
                if (command.conditionExist)
                {
                    command.conditionType = reader.ReadByte();
                    command.conditionValue = reader.ReadInt32();
                }

            }
            return command;
        }

        private object ReadCommandArg()
        {
            int type = reader.ReadByte();
            switch (type)
            {
                case 1:
                    return reader.ReadByte();
                case 5:
                    return reader.ReadBoolean();
                case 2:
                    return reader.ReadInt16();
                case 3:
                    return reader.ReadInt32();
                case 6:
                    return DecompilerUtils.Int2Float(reader.ReadInt32());
                case 4:
                    return reader.ReadString();
                default:
                    throw new Exception("Illegal arg type! " + type);
            }
        }
    }
}