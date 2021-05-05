using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace SimpleScriptDecompiler.Script.Win32
{
    public struct Command
    {
        public byte type;
        public byte id;
        public byte argsCount;

        public object[] args;

        public byte unkCount1;
        public byte[] unk1;

        public byte braceExist;
        public byte[] braceLineCount;

        public bool conditionExist;
        public byte conditionType;
        public int conditionValue;
    }
}
