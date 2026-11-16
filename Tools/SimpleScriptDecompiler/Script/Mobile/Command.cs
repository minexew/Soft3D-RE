using SimpleScriptDecompiler.Utils;

namespace SimpleScriptDecompiler.Script.Mobile
{
    public struct ScriptReference
    {
        public ushort Scope;
        public ushort Index;

        public ScriptReference(ushort scope, ushort index)
        {
            Scope = scope;
            Index = index;
        }

        public override string ToString()
        {
            return DecompilerUtils.GetVarName(Index);
        }
    }

    public struct Command
    {
        public ushort id;
        public ushort type;
        public ushort argsCount;

        public object[] args;

        public ushort resultType;
        public ushort resultSize;
        public ScriptReference result;

        public ushort bodyCount;
        public Command[] body;

        public ushort conditionCount;
        public Command[] conditions;
    }
}
