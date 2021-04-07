using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace SimpleScriptDecompiler.Decompiler
{
    class CodeWriter
    {
        public enum WriterMode
        {
            NORMAL,
            SERIAL
        }

        private const int tabSize = 4;

        private StreamWriter writer;
        private string space;
        private int tabLevel;
        private WriterMode mode;

        private bool spaceNeeded;

        public void Open(string path)
        {
            Reset();
            writer = new StreamWriter(File.Open(path, FileMode.Create));
            WriteInfo();
        }

        private void Reset()
        {
            space = string.Empty;
            tabLevel = 0;
            mode = WriterMode.NORMAL;
            spaceNeeded = false;
        }

        private void WriteInfo()
        {
            writer.WriteLine("//Decompiled by SimpleScriptDecompiler v{0}\n\n", Globals.VERSION);
        }

        public void AddTabLevel()
        {
            SetTabLevel(++tabLevel);
        }

        public void RemoveTabLevel()
        {
            SetTabLevel(--tabLevel);
        }

        public void SetTabLevel(int level)
        {
            tabLevel = Math.Max(level, 0);
            space = new string(' ', tabSize * tabLevel);
        }

        public void WriteLine()
        {
            WriteLine(string.Empty);
        }

        public void WriteLine(object value)
        {
            if (!spaceNeeded)
            {
                writer.WriteLine(space + value);
            }
            else
            {
                spaceNeeded = false;
                writer.WriteLine(value);
            }
        }

        public void Write(object value)
        {
            if(!spaceNeeded)
            {
                spaceNeeded = true;
                writer.Write(space + value);
            }
            else
            {
                writer.Write(value);
            }
            
        }

        public void WriteLine(string format, params object[] arg)
        {
            if(arg == null || arg.Length <= 0)
            {
                WriteLine((object)format);
                return;
            }
            if (!spaceNeeded)
            {
                writer.WriteLine(space + format, arg);
            }
            else
            {
                spaceNeeded = false;
                writer.WriteLine(format, arg);
            }
        }

        public void Write(string format, params object[] arg)
        {
            if (!spaceNeeded)
            {
                spaceNeeded = true;
                writer.Write(space + format, arg);
            }
            else
            {
                writer.Write(format, arg);
            }
        }

        public void WriteEndCommand()
        {
            WriteLine(";");
        }

        public void SetMode(WriterMode mode)
        {
            switch(mode)
            {
                case WriterMode.NORMAL:
                    RemoveTabLevel();
                    WriteLine("}");
                    break;
                case WriterMode.SERIAL:
                    WriteLine("serial");
                    WriteLine("{");
                    AddTabLevel();
                    break;
                default:
                    throw new ArgumentException("Illegal writer mode!");
            }
            this.mode = mode;
        }

        public WriterMode GetMode()
        {
            return mode;
        }

        public void Close()
        {
            writer.Close();
        }
    }
}
