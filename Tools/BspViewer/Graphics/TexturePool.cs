using BspViewer.Data.Texture;
using OpenTK.Graphics.OpenGL;
using System.Collections.Generic;
using System.Linq;

namespace BspViewer.Graphics
{
    class TexturePool
    {
        private static readonly Dictionary<int, ITexture> pool = new Dictionary<int, ITexture>();


        public static int LoadTexture(ITexture texture)
        {
            if (texture == null)
                return -1;
            //Texture already loaded
            if (GetTextureIDByName(texture.GetName()) >= 0)
                return -1;

            byte[] pixelData = texture.GetPixelData();
            if(pixelData == null)
                return -1;

            GL.Hint(HintTarget.PerspectiveCorrectionHint, HintMode.Nicest);

            GL.GenTextures(1, out int tex);
            GL.BindTexture(TextureTarget.Texture2D, tex);

            GL.TexImage2D(TextureTarget.Texture2D, 0, PixelInternalFormat.Rgba, texture.GetWidth(), texture.GetHeight(), 0,
                PixelFormat.Bgra, PixelType.UnsignedByte, pixelData);

            GL.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureMinFilter, (int)TextureMinFilter.Linear);
            GL.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureMagFilter, (int)TextureMagFilter.Linear);
            pool.Add(tex, texture);
            return tex;
        }

        public static ITexture GetTextureByName(string name)
        {
            ITexture result = null;
            for (int i = 0; i < pool.Count; i++)
            {
                var element = pool.ElementAt(i);
                if (element.Value.GetName() == name)
                {
                    result = element.Value;
                    break;
                }
            }
            return result;
        }

        public static int GetTextureIDByName(string name)
        {
            int result = -1;
            for(int i = 0; i < pool.Count; i++)
            {
                var element = pool.ElementAt(i);
                if (element.Value.GetName() == name)
                {
                    result = element.Key;
                    break;
                }
            }
            return result;
        }

        public static void DeleteTextureByName(string name)
        {
            for (int i = 0; i < pool.Count; i++)
            {
                var element = pool.ElementAt(i);
                if (element.Value.GetName() == name)
                {
                    GL.DeleteTexture(element.Key);
                    break;
                }
            }
        }
    }
}
