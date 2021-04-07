using SimpleScriptDecompiler.Decompiler;
using System;
using System.IO;
using System.Text;

namespace SimpleScriptDecompiler.Script
{

    class MobileSimpleScript : IScriptInterface
    {
        public struct Function
        {
            public ushort varsCount;
            public Type[] vars; //types

            public ushort externalCommandCount;
            public Command[] exCommands;

            public ushort commandCount;
            public Command[] commands;
        }

        public struct Command
        {
            public ushort id;
            public ushort type;
            public ushort argsCount;

            public object[] args;

            public ushort unk2;
            public ushort unk2_data1;
            public ushort unk2_data2;
            public ushort unk2_data3;

            public ushort exCommand1Count;
            public Command[] ex1Commands;

            public ushort exCommand2Count;
            public Command[] ex2Commands;
        }

        //SimpleScriptStruct
        public ushort varsCount;
        public Type[] vars;

        public bool constructorExist;
        public Function constructorFunction;

        public ushort functionCount;
        public Function[] functions;
        //SimpleScriptStruct end

        private BinaryReader reader;

        public void LoadFromFile(string path)
        {

            reader = new BinaryReader(File.OpenRead(path));
            varsCount = reader.ReadUInt16();
            if (varsCount > 0)
            {
                vars = new Type[varsCount];
                //read types
                for (int i = 0; i < varsCount; i++)
                    vars[i] = GetArgType(reader.ReadUInt16());
            }
            constructorExist = reader.ReadUInt16() == 1;

            if (constructorExist) //Constructor?
            {
                constructorFunction = ReadFunction();
            }
            

            functionCount = reader.ReadUInt16();

            if (functionCount > 0)
            {
                functions = new Function[functionCount];
                for (int i = 0; i < functionCount; i++)
                {
                    functions[i] = ReadFunction();
                }
            }


            reader.Close();
        }

        private Function ReadFunction()
        {
            Function func = new Function();

            func.varsCount = reader.ReadUInt16();
            if (func.varsCount > 0) //vars?
            {
                func.vars = new Type[func.varsCount];
                for (int i = 0; i < func.varsCount; i++)
                {
                    func.vars[i] = GetArgType(reader.ReadUInt16()); //type
                }
            }
            func.externalCommandCount = reader.ReadUInt16();

            if (func.externalCommandCount > 0) //External?
            {
                func.exCommands = new Command[func.externalCommandCount];
                for (int i = 0; i < func.externalCommandCount; i++)
                {
                    func.exCommands[i] = ReadCommand();
                }
            }

            func.commandCount = reader.ReadUInt16();
            if (func.commandCount > 0)
            {
                func.commands = new Command[func.commandCount];
                for (int i = 0; i < func.commandCount; i++)
                {
                    func.commands[i] = ReadCommand();
                }
            }

            return func;
        }


        private Command ReadCommand()
        {
            Command command = new Command();
            command.id = reader.ReadUInt16();
            command.type = reader.ReadUInt16();
            command.argsCount = reader.ReadUInt16();

            if (command.argsCount > 0)
            {
                command.args = new object[command.argsCount];
                for (int i = 0; i < command.argsCount; i++)
                {
                    command.args[i] = ReadCommandArg();

                }
            }
            command.unk2 = reader.ReadUInt16();

            if (command.unk2 == 6)
            {
                command.unk2_data1 = reader.ReadUInt16();
                command.unk2_data2 = reader.ReadUInt16();
                command.unk2_data3 = reader.ReadUInt16();
            }

            command.exCommand1Count = reader.ReadUInt16();

            if (command.exCommand1Count > 0)
            {
                command.ex1Commands = new Command[command.exCommand1Count];
                for (int i = 0; i < command.exCommand1Count; i++)
                {
                    command.ex1Commands[i] = ReadCommand();
                }
            }

            command.exCommand2Count = reader.ReadUInt16();

            if (command.exCommand2Count > 0)
            {
                command.ex2Commands = new Command[command.exCommand2Count];
                for (int i = 0; i < command.exCommand2Count; i++)
                {
                    command.ex2Commands[i] = ReadCommand();
                }
            }

            return command;
        }


        private object ReadCommandArg()
        {
            Type type = GetArgType(reader.ReadInt16());
            int size = reader.ReadInt16();
            byte[] data = reader.ReadBytes(size);

            //REMADE
            if (type == typeof(float))
                return DecompilerUtils.Int2Float(BitConverter.ToInt32(data, 0));
            else if (type == typeof(int))
                return BitConverter.ToInt32(data, 0);
            else if (type == typeof(string))
                return "\"" + Encoding.UTF8.GetString(data).Replace("\0", "") + "\"";
            else if (type == typeof(bool))
                return BitConverter.ToInt32(data, 0) == 1;
            else if (type == typeof(Nullable))
                return "0x" + BitConverter.ToInt32(data, 0).ToString("X");
            else
                throw new Exception("Illegal arg type! " + type);

        }

        private Type GetArgType(int type)
        {
            switch (type)
            {
                case 5: //float
                    return typeof(float);
                case 2: //int
                    return typeof(int);
                case 3: //string
                    return typeof(string);
                case 4: //bool
                    return typeof(bool);
                case 6: //Ignoring, maybe label to if, else, wait (0x2000X)
                    return typeof(Nullable);
                default:
                    throw new Exception("Illegal arg type! " + type);
            }
        }
    }
}
