using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace SimpleScriptDecompiler.Utils
{
    class DecompilerUtils
    {
        public static string SeparateArgsAsString(object[] args)
        {
            StringBuilder sb = new StringBuilder();
            for (int i = 0; i < args.Length; i++)
            {
                sb.Append(args[i]);
                if (args.Length > 1 && i < args.Length - 1)
                    sb.Append(", ");

            }
            return sb.ToString();
        }

        public static string GetFuncName(int index)
        {
            return "func_" + index;
        }

        public static string GetVarName(int index)
        {
            return "var_" + (index + 1);
        }

        public static float Int2Float(int i)
        {
            float result = i >> 16;
            result += (float)(i & 0xFFFF) / ushort.MaxValue;
            return (float)Math.Round(result, 4);
        }
    }
}
