using SimpleScriptDecompiler.Decompiler;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace SimpleScriptDecompiler.Script
{

    class Win32SimpleScript : IScriptInterface
    {
        public class Function
        {
            public Command[] commands;

            public Function(int commandCount)
            {
                commands = new Command[commandCount];
            }
        }

        public struct Command
        {
            public byte type;
            public byte id;
            public byte argsCount;

            public object[] args;

            public byte unkCount1;
            public byte[] unk1;

            public byte braceExist;
            public byte[] braceLineCount;

            public byte conditionExist;
            public byte conditionType;
            public int conditionValue;
        }

        public Function[] functions;
        private BinaryReader reader;

        
        public void LoadFromFile(string path)
        {
            reader = new BinaryReader(File.OpenRead(path));
            int count = reader.ReadUInt16();
            if (count > 0)
            {
                functions = new Function[count];
                for (int i = 0; i < count; i++)
                {
                    functions[i] = ReadScriptFunction();
                }
            }
            reader.Close();
        }

        private Function ReadScriptFunction()
        {
            Function func = null;
            int count = reader.ReadUInt16();
            if (count > 0)
            {
                func = new Function(count);
                for (int i = 0; i < count; i++)
                {
                    func.commands[i] = ReadScriptFunctionCommand();
                }
            }
            return func;
        }

        private Command ReadScriptFunctionCommand()
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
                command.braceLineCount = reader.ReadBytes(2 * command.braceExist); //short?
            }

            if (command.type == 1 || command.type == 3)
            {
                command.conditionExist = reader.ReadByte();
                if (command.conditionExist > 0) //always 0 or 1?
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
                case 5: //bool
                    return reader.ReadByte() == 1;
                case 2:
                    return reader.ReadInt16();
                case 3:
                    return reader.ReadInt32();
                case 6:
                    return DecompilerUtils.Int2Float(reader.ReadInt32());
                case 4: //string
                    int len = reader.ReadInt16() * 2;
                    byte[] data = reader.ReadBytes(len);
                    return "\"" + Encoding.Unicode.GetString(data) + "\"";
                default:
                    throw new Exception("Illegal arg type! " + type);
            }
        }
    }
}
