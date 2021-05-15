using BspViewer.IO;
using System;
using System.IO;

namespace BspViewer.Data.Texture
{
    class STXTexture : ITexture
    {
        private int paletteSize;
        private int type;
        private int transColor;
        private int unk;

        private int width;
        private int height;

        private string name;
        private byte[] pixelData;
        private int[] palette;

        public void LoadFromFile(string path)
        {
            name = Path.GetFileNameWithoutExtension(path);
            CustomBinaryReader reader = new CustomBinaryReader(path);
            paletteSize = reader.ReadByte();
            type = reader.ReadByte();
            transColor = reader.ReadByte();
            unk = reader.ReadByte();

            width = reader.ReadInt32();
            height = reader.ReadInt32();

            if (paletteSize == 0)
                paletteSize = 0x100;
            //if (type != 2)
                //throw new NotImplementedException();

            palette = new int[paletteSize];
            for (int i = 0; i < paletteSize; i++)
            {
                ushort rgb565 = reader.ReadUInt16();
                int b = (rgb565 & 0x1f) << 3;
                int g = ((rgb565 >> 5) & 0x3f) << 2;
                int r = ((rgb565 >> 11) & 0x1f) << 3;
                palette[i] = 0xFF << 24 | r << 16 |  g << 8 | b;
                reader.ReadBytes(31 * 2); //skip palettes
            }

            pixelData = reader.ReadBytes(width * height);
            reader.Close();
        }

        public byte[] GetPixelData()
        {
            byte[] result = new byte[width * height * sizeof(int)];
            int dataIndex = 0;
            for (int y = 0; y < height; y++)
            {
                for (int x = 0; x < width; x++)
                {
                    int colorIndex = pixelData[dataIndex];
                    int color = palette[colorIndex];
                    if (colorIndex == transColor)
                        color &= 0x00FFFFFF; //Set alpha to 0
                    //Converting color to bytes and copy to pixelData
                    Array.Copy(BitConverter.GetBytes(color), 0,
                        result, sizeof(int) * dataIndex++, 4);
                }
            }
            return result;
        }

        public string GetName()
        {
            return name;
        }

        public int GetWidth()
        {
            return width;
        }

        public int GetHeight()
        {
            return height;
        }
    }
}
