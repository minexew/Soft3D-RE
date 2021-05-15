using BspViewer.IO;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace BspViewer.Data.Bsp.Lumps
{
    class LEdge : Lump
    {
        public struct Edge
        {
            public ushort a;
            public ushort b;
        }

        public Edge[] Edges { get; private set; }

        protected override void LoadFromStream(CustomBinaryReader reader, int length)
        {
            Debug.Assert(length % 4 == 0);

            int edgeCount = length / 4;
            Edges = new Edge[edgeCount];

            for (int i = 0; i < edgeCount; i++)
            {
                Edge edge = new Edge();
                edge.a = reader.ReadUInt16();
                edge.b = reader.ReadUInt16();
                Edges[i] = edge;
            }
        }
    }
}
