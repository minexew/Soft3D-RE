using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace SimpleScriptDecompiler.Script.Mobile
{
    public struct Command
    {
        public ushort id;
        public ushort type;
        public ushort argsCount;

        public object[] args;

        public ushort unk2;
        public ushort unk2_data1;
        public ushort unk2_data2;
        public ushort unk2_data3;

        public ushort exCommand1Count;
        public Command[] ex1Commands;

        public ushort exCommand2Count;
        public Command[] ex2Commands;
    }
}
