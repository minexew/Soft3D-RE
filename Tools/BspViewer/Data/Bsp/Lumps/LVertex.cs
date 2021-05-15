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
    class LVertex : Lump
    {
        public Vector3[] Positions { get; private set; }

        protected override void LoadFromStream(CustomBinaryReader reader, int length)
        {
            Debug.Assert(length % 12 == 0);

            int vertCount = length / 12;
            Positions = new Vector3[vertCount];

            for (int i = 0; i < vertCount; i++)
            {
                float x = reader.ReadSingle();
                float y = reader.ReadSingle();
                float z = reader.ReadSingle();
                Positions[i] = new Vector3(x, y, z);
            }
        }
    }
}
