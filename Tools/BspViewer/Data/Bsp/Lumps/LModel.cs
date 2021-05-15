using BspViewer.Common;
using BspViewer.Data.Model;
using BspViewer.IO;
using BspViewer.Utils;
using OpenTK;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace BspViewer.Data.Bsp.Lumps
{
    class LModel : Lump
    {
        public SOJModel[] Models { get; private set; }

        protected override void LoadFromStream(CustomBinaryReader reader, int length)
        {
            Debug.Assert(length % 0x58 == 0);
            int count = length / 0x58;

            Models = new SOJModel[count];
            for (int i = 0; i < count; i++)
            {
                string name = reader.ReadPackedString();
                SOJModel model = (SOJModel)ResourceLoader.LoadFileFromPath(
                    ResourceType.Model,
                    name);
                reader.ReadBytes(0x10);
                Matrix4 transform = reader.ReadMatrix();
                reader.ReadBytes(8);

                if(model == null)
                {
                    Console.WriteLine("Model {0} skipped.", name);
                    continue;
                }

                model.ModelTransform = transform;
                Models[i] = model;
            }
        }
    }
}
