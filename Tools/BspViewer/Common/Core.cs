using BspViewer.Data;
using BspViewer.Data.Bsp;
using BspViewer.Graphics.Camera;
using BspViewer.Input;
using OpenTK;
using System;
using System.Collections.Generic;

namespace BspViewer.Common
{
    class Core
    {
        private static readonly List<Bsp> maps = new List<Bsp>();
        private static FreeCamera camera;

        public static void Init()
        {
            camera = new FreeCamera();
            camera.Init(Vector3.UnitZ * 3, Globals.DEFAULT_WIDTH / Globals.DEFAULT_HEIGHT);
            //camera.Position = new Vector3(-197, -26, 156);
        }

        public static void Update()
        {
            InputSystem.Update(camera);
        }

        public static void Render()
        {
            if (maps == null)
                return;

            for (int i = 0; i < maps.Count; i++)
            {
                maps[i].Render(camera);
            }
        }

        public static void LoadMap(string name)
        {
            Console.WriteLine("Loading map {0}...", name);
            Bsp map = (Bsp)ResourceLoader.LoadFileFromPath(ResourceType.Map, name);
            if (map == null)
                return;
            Console.WriteLine("Done.\n");
            maps.Add(map);
        }
    }
}
