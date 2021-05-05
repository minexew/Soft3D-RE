using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace SimpleScriptDecompiler.IO
{
    class CustomBinaryReader : BinaryReader
    {
        public CustomBinaryReader(Stream s) : base(s) { }

        public override string ReadString()
        {
            int len = base.ReadInt16() * 2;
            byte[] data = base.ReadBytes(len);

            string s = Encoding.Unicode.GetString(data);
            //some empty string already has quotes
            if (s.Equals("\"\""))
                return s;

            return string.Format("\"{0}\"", s);
        }
    }
}
