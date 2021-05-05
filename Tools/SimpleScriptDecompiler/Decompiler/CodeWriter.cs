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
        private uint lineIndex;

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
            lineIndex = 0;
            spaceNeeded = false;
        }

        private void WriteInfo()
        {
            WriteLine("//Decompiled by SimpleScriptDecompiler v{0}", Globals.VERSION);
            WriteLine();
            WriteLine();
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
            space = new string(Constants.SPACE, tabSize * tabLevel);
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
            lineIndex++;
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

        public void WriteLine(string format, object arg0)
        {
            if (!spaceNeeded)
            {
                writer.WriteLine(space + format, arg0);
            }
            else
            {
                spaceNeeded = false;
                writer.WriteLine(format, arg0);
            }
            lineIndex++;
        }

        public void WriteLine(string format, params object[] arg)
        {
            if (!spaceNeeded)
            {
                writer.WriteLine(space + format, arg);
            }
            else
            {
                spaceNeeded = false;
                writer.WriteLine(format, arg);
            }
            lineIndex++;
        }

        public void Write(string format, object arg0)
        {
            if (!spaceNeeded)
            {
                spaceNeeded = true;
                writer.Write(space + format, arg0);
            }
            else
            {
                writer.Write(format, arg0);
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
        /*
        public void WriteEndCommand()
        {
            WriteLine(Constants.END_STATEMENT);
        }
        */
        public void SetMode(WriterMode mode)
        {
            switch(mode)
            {
                case WriterMode.NORMAL:
                    RemoveTabLevel();
                    WriteLine(Constants.END_FUNC);
                    break;
                case WriterMode.SERIAL:
                    WriteLine(Constants.SERIAL);
                    WriteLine(Constants.START_FUNC);
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

        public void Warning(object warn)
        {
            Console.WriteLine("Warning! {0} on line: {1}", warn, GetLineIndex());
            Write(warn);
        }

        public void Warning(string format, params object[] arg)
        {
            Console.WriteLine("Warning! {0} on line: {1}",
                string.Format(format, arg),
                GetLineIndex());
            Write(format, arg);
        }

        public uint GetLineIndex()
        {
            return lineIndex + 1;
        }

        public void Close()
        {
            writer.Close();
        }
    }
}
