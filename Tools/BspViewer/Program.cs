using BspViewer.Common;
using BspViewer.Data;
using BspViewer.UI;
using System;
using System.Collections.Generic;
using System.IO;
using System.Text;

namespace BspViewer
{
    class Program
    {
        //TODO: Add Logger
        private static List<string> maps = new List<string>();

        private const float FPS = 60.0f;
        static void Main(string[] args)
        {
            Console.WriteLine("BspViewer for 7DS v{0}", Globals.VERSION);
            ProcessArgs(args);

            if(maps.Count == 0)
            {
                PrintUsage();
                return;
            }

            MainWindow win = new MainWindow(Globals.DEFAULT_WIDTH, Globals.DEFAULT_HEIGHT);
            win.Title = "BspViewer";
            //Loading all maps
            foreach (string map in maps)
                Core.LoadMap(map);

            maps = null;

            win.Run(FPS);
        }

        private static void ProcessArgs(string[] args)
        {
            if (args == null || args.Length < 3)
                return;

            string mapsPath = args[0];
            if (!Directory.Exists(mapsPath))
                return;

            string resPath = args[1];
            if (!Directory.Exists(resPath))
                return;

            StringBuilder sb = new StringBuilder();
            for (int i = 2; i < args.Length; i++)
                sb.Append(args[i]);

            string data = sb.ToString().Replace(" ", string.Empty);

            foreach(string map in data.Split(','))
            {
                string mapPath = mapsPath + "/" + map + ".sbp";
                if (File.Exists(mapPath))
                    maps.Add(map);
                else
                    Console.WriteLine("Map {0} not found.", map);
            }

            if (maps.Count == 0)
                return;

            ResourceLoader.RESOURCES_PATH = resPath;
            ResourceLoader.MAPS_PATH = mapsPath;
        }

        private static void PrintUsage()
        {
            Console.WriteLine("Usage: <maps path> <res path> <maps names separated by commas without ext>");
            Console.WriteLine(@"Example: C:\maps C:\resources map1001, map1002");
        }
    }
}
