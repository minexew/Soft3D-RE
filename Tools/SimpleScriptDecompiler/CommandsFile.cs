using System;
using System.Collections.Generic;
using System.IO;


namespace SimpleScriptDecompiler
{
    class CommandsFile
    {
        private const string dir = "names";

        private static Dictionary<int, string> commands;

        public static bool Init(string filename)
        {
            string path = dir + "/" + filename;
            if(!File.Exists(path))
            {
                Console.WriteLine("{0} not found!", path);
                return false;
            }

            commands = new Dictionary<int, string>();

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
                commands.Add(index, data[1]);
            }
            reader.Close();
            return true;
        }

        public static string GetCommandName(int index)
        {
            commands.TryGetValue(index, out string commandName);

            return commandName;
        }
    }
}
