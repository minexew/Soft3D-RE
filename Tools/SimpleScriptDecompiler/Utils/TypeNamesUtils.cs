using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace SimpleScriptDecompiler.Utils
{
    class TypeNamesUtils
    {
        private static Dictionary<string, string> names = new Dictionary<string, string>()
        {
            { "bool",   "Boolean" },
            { "float",  "Single" },
            { "int",    "Int32" },
            { "short",  "Int16" },
            { "byte",   "Byte" }, // "SByte"
            { "string", "String" }
        };

        public static string ConvertC2CSharp(string typeName)
        {
            if(!names.TryGetValue(typeName, out string value))
                throw new Exception("Illegal arg type! Name: " + typeName);

            return value;
        }

        public static string ConvertCSharp2C(string typeName)
        {
            if(!names.ContainsValue(typeName))
                throw new Exception("Illegal arg type! Name: " + typeName);

            return names.FirstOrDefault(x => x.Value == typeName).Key;
        }
    }
}
