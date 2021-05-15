using BspViewer.IO;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace BspViewer.Data.Bsp.Lumps
{
    class LLightmap : Lump
    {
        public byte[] LightmapsData { get; private set; }

        protected override void LoadFromStream(CustomBinaryReader reader, int length)
        {
            LightmapsData = reader.ReadBytes(length);
        }
    }
}
