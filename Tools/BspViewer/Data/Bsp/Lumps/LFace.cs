using BspViewer.IO;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace BspViewer.Data.Bsp.Lumps
{
    class LFace : Lump
    {
        public struct Face
        {
            public ushort planeIndex;
            public ushort firstEdgeIndex;
            public ushort unused04;
            public ushort edgesCount;
            public short texInfoIndex;
            public short unknown0a;
            public short textureMinU;
            public short textureMinV;
            public int lightmapOffset;
            public byte[] styles;
        }

        public Face[] Faces { get; private set; }

        protected override void LoadFromStream(CustomBinaryReader reader, int length)
        {
            Debug.Assert(length % 24 == 0);

            int count = length / 24;
            Faces = new Face[count];
            for (int i = 0; i < count; i++)
            {
                Face face = new Face();
                face.planeIndex = reader.ReadUInt16();
                face.firstEdgeIndex = reader.ReadUInt16();
                face.unused04 = reader.ReadUInt16();
                face.edgesCount = reader.ReadUInt16();
                face.texInfoIndex = reader.ReadInt16();
                face.unknown0a = reader.ReadInt16();
                face.textureMinU = reader.ReadInt16();
                face.textureMinV = reader.ReadInt16();
                face.lightmapOffset = reader.ReadInt32();
                face.styles = reader.ReadBytes(4);

                Faces[i] = face;
            }
        }
    }
}
