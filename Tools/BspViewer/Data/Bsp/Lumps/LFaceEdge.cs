using BspViewer.IO;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace BspViewer.Data.Bsp.Lumps
{
    class LFaceEdge : Lump
    {
        public int[] FaceEdges { get; private set; }

        protected override void LoadFromStream(CustomBinaryReader reader, int length)
        {
            Debug.Assert(length % 4 == 0);

            int count = length / 4;

            FaceEdges = new int[count];

            for (int i = 0; i < count; i++)
                FaceEdges[i] = reader.ReadInt32();
        }
    }
}
