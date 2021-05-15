using BspViewer.IO;
using OpenTK;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace BspViewer.Data.Bsp.Lumps
{
    class LTextInfo : Lump
    {
        public struct TextInfo
        {
            public Vector3 uAxis;
            public float uOffset;
            public Vector3 vAxis;
            public float vOffset;
            public string textureName;
        }

        public TextInfo[] TexturesInfo { get; private set; }

        protected override void LoadFromStream(CustomBinaryReader reader, int length)
        {
            Debug.Assert(length % 48 == 0);
            int count = length / 48;

            TexturesInfo = new TextInfo[count];
            for (int i = 0; i < count; i++)
            {
                TextInfo textInfo = new TextInfo();
                textInfo.uAxis = reader.ReadVec3();
                textInfo.uOffset = reader.ReadSingle();
                textInfo.vAxis = reader.ReadVec3();
                textInfo.vOffset = reader.ReadSingle();
                textInfo.textureName = reader.ReadPackedString();
                TexturesInfo[i] = textInfo;
            }
           
        }
    }
}
