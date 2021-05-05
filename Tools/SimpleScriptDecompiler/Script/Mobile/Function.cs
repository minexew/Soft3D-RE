using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace SimpleScriptDecompiler.Script.Mobile
{
    public struct Function
    {
        public ushort varsCount;
        public Type[] vars; //types

        public ushort externalCommandCount;
        public Command[] exCommands;

        public ushort commandCount;
        public Command[] commands;
    }
}
