using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace SimpleScriptDecompiler.Script.Win32
{
    public struct Function
    {
        public int commandsCount;
        public Command[] commands;
    }
}
