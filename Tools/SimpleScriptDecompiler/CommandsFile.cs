using SimpleScriptDecompiler.Utils;
using System;
using System.Collections.Generic;
using System.IO;


namespace SimpleScriptDecompiler
{
    class CommandsFile
    {
        public class Command
        {
            public string Name;
            public Type[] Args;

            public bool HasArgs()
            {
                return Args != null;
            }
        }

        private const string dir = "names";

        private static Dictionary<int, Command> commands;
        private static bool isLoaded;


        public static void LoadFromFile(string filename)
        {
            string path = dir + "/" + filename;
            if(!File.Exists(path))
            {
                Console.WriteLine("{0} not found!", path);
                return;
            }

            commands = new Dictionary<int, Command>();

            StreamReader reader = new StreamReader(File.OpenRead(path));
            string line;
            while((line = reader.ReadLine()) != null)
            {
                if (string.IsNullOrEmpty(line))
                    continue;
                if (line.StartsWith("#"))
                    continue;

                string[] data = line.Split(':');
                if(data == null || data.Length != 2)
                    continue;

                if (!int.TryParse(data[0], out int index))
                    continue;

                Command command = GetCommand(data[1].Replace(" ", string.Empty));
                if(command != null)
                    commands.Add(index, command);
            }
            reader.Close();


            isLoaded = true;
        }

        public static bool IsLoaded()
        {
            return isLoaded;
        }

        private static Command GetCommand(string line)
        {
            string[] data = line.Split('(');
            if (data == null || data.Length != 2)
            {
                Console.WriteLine("Uncorrect command {0} on command file! Skipped.", line);
                return null;
            }

            Command command = new Command();
            command.Name = data[0];
            command.Args = GetCommandAgrs(data[1].Substring(0, data[1].IndexOf(')')));
            return command;
        }

        private static Type[] GetCommandAgrs(string s)
        {
            string[] data = s.Split(',');
            List<Type> args = new List<Type>();
            for(int i = 0; i < data.Length; i++)
            {
                if (string.IsNullOrEmpty(data[i]))
                    continue;

                string normalTypeName = TypeNamesUtils.ConvertC2CSharp(data[i]);
                string fullTypeName = "System." + normalTypeName;
                Type t = Type.GetType(fullTypeName);

                if (t == null)
                    throw new Exception("Illegal arg type in command file! Str: " + fullTypeName);
                args.Add(t);
            }
            if (args.Count == 0)
                return null;

            return args.ToArray();
        }



        public static Command GetCommand(int index)
        {
            commands.TryGetValue(index, out Command command);
            return command;
        }
    }
}
