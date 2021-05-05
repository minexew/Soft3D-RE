using SimpleScriptDecompiler.Decompiler;
using System;
using System.Text;

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
            return Constants.FUNC_PREFIX + index;
        }

        public static string GetVarName(int index)
        {
            return Constants.VAR_PREFIX + (index + 1);
        }

        public static bool GetCommandName(int id, out string name)
        {
            name = string.Format("Command[{0}]", id);

            if (CommandsFile.IsLoaded)
            {
                var c = CommandsFile.GetCommand(id);
                if (c != null && c.Name != null)
                {
                    name = c.Name;
                    return true;
                }
            }

            return false;
        }

        public static float Int2Float(int i)
        {
            float result = (float)i / 0x10000;
            return (float)Math.Round(result, 4);
            /*
            float result = i >> 16;
            result += (float)(i & 0xFFFF) / ushort.MaxValue;
            return (float)Math.Round(result, 4);
            */
        }
    }
}
