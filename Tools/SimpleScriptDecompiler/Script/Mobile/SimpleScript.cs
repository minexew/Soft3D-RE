using SimpleScriptDecompiler.Utils;
using System;
using System.IO;
using System.Text;

namespace SimpleScriptDecompiler.Script.Mobile
{
    class SimpleScript : IScriptInterface
    {
        public ushort varsCount;
        public Type[] vars;

        public bool constructorExist;
        public Function constructorFunction;

        public ushort functionCount;
        public Function[] functions;

        private BinaryReader reader;

        public void LoadFromFile(string path)
        {
            reader = new BinaryReader(File.OpenRead(path));
            varsCount = reader.ReadUInt16();
            if (varsCount > 0)
            {
                vars = new Type[varsCount];
                for (int i = 0; i < varsCount; i++)
                    vars[i] = GetArgType(reader.ReadUInt16());
            }
            constructorExist = reader.ReadUInt16() == 1;

            if (constructorExist)
                constructorFunction = ReadFunction();

            functionCount = reader.ReadUInt16();
            if (functionCount > 0)
            {
                functions = new Function[functionCount];
                for (int i = 0; i < functionCount; i++)
                    functions[i] = ReadFunction();
            }
            if (reader.BaseStream.Position != reader.BaseStream.Length)
                throw new Exception("Trailing data after simple script");
            reader.Close();
        }

        private Function ReadFunction()
        {
            Function func = new Function();

            func.varsCount = reader.ReadUInt16();
            if (func.varsCount > 0)
            {
                func.vars = new Type[func.varsCount];
                for (int i = 0; i < func.varsCount; i++)
                    func.vars[i] = GetArgType(reader.ReadUInt16());
            }
            func.externalCommandCount = reader.ReadUInt16();
            if (func.externalCommandCount > 0)
            {
                func.exCommands = new Command[func.externalCommandCount];
                for (int i = 0; i < func.externalCommandCount; i++)
                    func.exCommands[i] = ReadCommand();
            }

            func.commandCount = reader.ReadUInt16();
            if (func.commandCount > 0)
            {
                func.commands = new Command[func.commandCount];
                for (int i = 0; i < func.commandCount; i++)
                    func.commands[i] = ReadCommand();
            }
            return func;
        }

        private Command ReadCommand()
        {
            Command command = new Command();
            command.id = reader.ReadUInt16();
            command.type = reader.ReadUInt16();
            command.argsCount = reader.ReadUInt16();
            if (command.argsCount > 32)
                throw new Exception("Illegal command arg count! " + command.argsCount);

            if (command.argsCount > 0)
            {
                command.args = new object[command.argsCount];
                for (int i = 0; i < command.argsCount; i++)
                    command.args[i] = ReadCommandArg();
            }
            command.resultType = reader.ReadUInt16();
            if (command.resultType == 6)
            {
                command.resultSize = reader.ReadUInt16();
                ushort index = reader.ReadUInt16();
                ushort scope = reader.ReadUInt16();
                command.result = new ScriptReference(scope, index);
            }

            command.bodyCount = reader.ReadUInt16();
            if (command.bodyCount > 0)
            {
                command.body = new Command[command.bodyCount];
                for (int i = 0; i < command.bodyCount; i++)
                    command.body[i] = ReadCommand();
            }

            command.conditionCount = reader.ReadUInt16();
            if (command.conditionCount > 0)
            {
                command.conditions = new Command[command.conditionCount];
                for (int i = 0; i < command.conditionCount; i++)
                    command.conditions[i] = ReadCommand();
            }
            return command;
        }

        private object ReadCommandArg()
        {
            Type type = GetArgType(reader.ReadInt16());
            int size = reader.ReadInt16();
            if (size != sizeof(int) && type != typeof(string))
                throw new Exception(string.Format("Illegal command arg size! Pos: {0} Size: {1}",
                    reader.BaseStream.Position - 2, size));

            byte[] data = reader.ReadBytes(size);
            if (data.Length != size)
                throw new EndOfStreamException();
            if (type == typeof(float))
                return DecompilerUtils.Int2Float(BitConverter.ToInt32(data, 0));
            if (type == typeof(byte))
                return data[0];
            if (type == typeof(short))
                return BitConverter.ToInt16(data, 0);
            if (type == typeof(int))
                return BitConverter.ToInt32(data, 0);
            if (type == typeof(string))
                return "\"" + Encoding.ASCII.GetString(data).Replace("\0", "") + "\"";
            if (type == typeof(bool))
                return BitConverter.ToInt32(data, 0) != 0;
            if (type == typeof(ScriptReference))
            {
                uint packed = BitConverter.ToUInt32(data, 0);
                return new ScriptReference((ushort)(packed >> 16), (ushort)packed);
            }
            throw new Exception("Illegal arg type! " + type);
        }

        private Type GetArgType(int type)
        {
            switch (type)
            {
                case 0:
                    return typeof(byte);
                case 1:
                    return typeof(short);
                case 2:
                    return typeof(int);
                case 3:
                    return typeof(string);
                case 4:
                    return typeof(bool);
                case 5:
                    return typeof(float);
                case 6:
                    return typeof(ScriptReference);
                default:
                    throw new Exception("Illegal arg type! " + type);
            }
        }
    }
}
