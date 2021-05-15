using BspViewer.Data.Model;
using BspViewer.Data.Texture;
using System;
using System.IO;

namespace BspViewer.Data
{
    class ResourceLoader
    {
        public static string RESOURCES_PATH { get; set; }
        public static string MAPS_PATH { get; set; }

        public static void LoadPak(string path)
        {
            //TODO
            throw new NotImplementedException();
        }

        public static ILoadableResource LoadFileFromPak(ResourceType type, string name)
        {
            //TODO
            throw new NotImplementedException();
        }

        public static ILoadableResource LoadFileFromPath(ResourceType type, string path)
        {
            if (string.IsNullOrEmpty(RESOURCES_PATH) || !Directory.Exists(RESOURCES_PATH))
                throw new DirectoryNotFoundException("Uncorrect resource directory!");

            try
            {
                switch(type)
                {
                    case ResourceType.Model:
                        SOJModel model = new SOJModel();
                        model.LoadFromFile(RESOURCES_PATH + "/" + path + ".soj");
                        return model;
                    case ResourceType.Texture:
                        //TODO: More texture types
                        ITexture texture = new STXTexture();
                        texture.LoadFromFile(RESOURCES_PATH + "/" + path + ".stx");
                        return texture;
                    case ResourceType.Map:
                        //TODO: Change this shit
                        Bsp.Bsp map = new Bsp.Bsp();
                        map.LoadFromFile(MAPS_PATH + "/" + path + ".sbp");
                        map.Setup();
                        return map;
                    default:
                        throw new NotImplementedException();
                }
            }
            catch
            {
                Console.WriteLine("Error loading resource: {0} Type: {1}",
                    path, type.ToString());
            }
            return null;
        }
    }
}
