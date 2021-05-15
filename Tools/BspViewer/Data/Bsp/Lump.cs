using BspViewer.IO;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace BspViewer.Data.Bsp
{
    abstract class Lump
    {
        public const string PREFIX = "L";

        public void Load(byte[] data)
        {
            CustomBinaryReader reader = new CustomBinaryReader(data);
            LoadFromStream(reader, data.Length);
            reader.Close();
        }

        protected abstract void LoadFromStream(CustomBinaryReader reader, int length);
    }
}
