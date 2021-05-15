using BspViewer.IO;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace BspViewer.Data.Bsp.Lumps
{
    class LPlane : Lump
    {
        public struct Plane
        {
            public float a;
            public float b;
            public float c;
            public float d;

            public int unk;
            public int unkFlag;
        }

        public Plane[] Planes { get; private set; }

        protected override void LoadFromStream(CustomBinaryReader reader, int length)
        {
            Debug.Assert(length % 20 == 0);

            int count = length / 20;

            Planes = new Plane[count];

            for (int i = 0; i < count; i++)
            {
                Plane plane = new Plane();
                plane.a = reader.ReadSingle(); // * 0x10000;
                plane.b = reader.ReadSingle(); // * 0x10000;
                plane.c = reader.ReadSingle(); // * 0x10000;
                plane.d = reader.ReadSingle(); // * 0x10000;

                plane.unk = reader.ReadInt32(); //type?

                if (plane.a < length)
                    plane.unkFlag |= 1;

                if (plane.b < length)
                    plane.unkFlag |= 2;

                if (plane.c < length)
                    plane.unkFlag |= 4;

                Planes[i] = plane;
            }
        }
    }
}
