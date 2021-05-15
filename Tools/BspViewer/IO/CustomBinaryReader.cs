using BspViewer.Utils;
using OpenTK;
using System.IO;

namespace BspViewer.IO
{
    class CustomBinaryReader : BinaryReader
    {
        public CustomBinaryReader(Stream s) : base(s) { }

        public CustomBinaryReader(byte[] data) : base(new MemoryStream(data)) { }

        public CustomBinaryReader(string path) : base(File.OpenRead(path)) { }

        public Vector3 ReadVec3()
        {
            float x = base.ReadSingle();
            float y = base.ReadSingle();
            float z = base.ReadSingle();
            return new Vector3(x, y, z);
        }

        public Vector2 ReadVec2()
        {
            float x = base.ReadSingle();
            float y = base.ReadSingle();
            return new Vector2(x, y);
        }

        public Vector3 ReadPackedVec3()
        {
            float x = MathUtils.Int2Float(base.ReadInt32());
            float y = MathUtils.Int2Float(base.ReadInt32());
            float z = MathUtils.Int2Float(base.ReadInt32());
            return new Vector3(x, y, z);
        }

        public Vector2 ReadPackedVec2()
        {
            float x = MathUtils.Int2Float(base.ReadInt32());
            float y = MathUtils.Int2Float(base.ReadInt32());
            return new Vector2(x, y);
        }

        public string ReadPackedString()
        {
            int[] nameData = new int[4]; //0x10
            for (int a = 0; a < nameData.Length; a++)
                nameData[a] = base.ReadInt32();
            return CompressUtils.UnpackString(nameData);
        }

        public float ReadPackedSingle()
        {
            return MathUtils.Int2Float(base.ReadInt32());
        }

        public Matrix4 ReadMatrix()
        {
            Matrix4 result = Matrix4.Identity;
            result.Column3 = new Vector4(ReadVec3(), 0);
            result.Column2 = new Vector4(ReadVec3(), 0);
            result.Column0 = new Vector4(ReadVec3(), 0);
            result.Column1 = new Vector4(ReadVec3(), 0);

            Matrix4 temp = result;
            for (int i = 0; i < 4; i++)
            {
                for (int j = 0; j < 4; j++)
                    result[i, j] = temp[j, i];
            }
            result[3, 3] = 1;
            return result;
        }
    }
}
