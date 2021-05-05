using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace SimpleScriptDecompiler.Decompiler
{
    class Constants
    {
        public const int SPECIAL_COMMANDS_COUNT = 7;

        public const char SPACE = ' ';
        public const char START_FUNC = '{';
        public const char END_FUNC = '}';
        public const char END_STATEMENT = ';';
        public const char COLON = ':';
        public const char START_ARGS = '(';
        public const char END_ARGS = ')';
        public const string NO_ARGS = "()";

        public const string IF = "if";
        public const string ELSE = "else";
        public const string WAIT = "wait";
        public const string SERIAL = "serial";

        public const string CONSTRUCT_FUNC_NAME = "init";
        public const string VARS = "VarName";
        public const string FUNC_PREFIX = "func_";
        public const string VAR_PREFIX = "var_";
    }
}
