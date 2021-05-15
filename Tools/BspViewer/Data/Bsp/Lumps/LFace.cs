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
            public ushort edgesCount;
            public short texInfoIndex;
            public int lightmapOffset;
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
                int b = reader.ReadUInt16(); //unused?
                face.edgesCount = reader.ReadUInt16();
                face.texInfoIndex = reader.ReadInt16();
                /*
                int texInfoIndexInFace = 0;
                if (texInfoIndex == -1)
                    texInfoIndexInFace = 0;
                else
                    texInfoIndexInFace = texInfo[texInfoIndex];
                    sub_402B44
                */
                int d = reader.ReadInt16();
                int e = reader.ReadUInt16();
                int f = reader.ReadUInt16();
                int g = reader.ReadInt32();
                face.lightmapOffset = reader.ReadInt32();

                Faces[i] = face;
            }
        }
    }
}
