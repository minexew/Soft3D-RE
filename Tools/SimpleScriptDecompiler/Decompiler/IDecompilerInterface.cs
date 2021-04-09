using SimpleScriptDecompiler.Script;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace SimpleScriptDecompiler.Decompiler
{
    interface IDecompilerInterface
    {
        void Decompile(IScriptInterface script, string name);
    }
}
