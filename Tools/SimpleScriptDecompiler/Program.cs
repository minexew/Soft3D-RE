using SimpleScriptDecompiler.Decompiler;
using SimpleScriptDecompiler.Script;
using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace SimpleScriptDecompiler
{
    class Program
    {
        private enum DecompilePlatform
        {
            None,
            Win32,
            Mobile
        }

        private static DecompilePlatform platform;
        private static string inputFile;
        private static string outputFile = "out.sst";


        static void Main(string[] args)
        {

            ProcessArgs(args);

            if(platform == DecompilePlatform.None || string.IsNullOrEmpty(inputFile))
            {
                PrintUsage();
                return;
            }

            //float "1,23 -> 1.23"
            ChangeFloatSeparator(".");

            IScriptInterface script;
            IDecompilerInterface decompiler;
            
            switch(platform)
            {
                case DecompilePlatform.Win32:
                    script = new Win32SimpleScript();
                    decompiler = new Win32Decompiler();
                    break;
                case DecompilePlatform.Mobile:
                    script = new MobileSimpleScript();
                    decompiler = new MobileDecompiler();
                    break;
                default:
                    PrintUsage();
                    return;
            }

            script.LoadFromFile(inputFile);
            decompiler.Decompile(script, outputFile);
            Console.WriteLine("Done.");
        }

        private static void ChangeFloatSeparator(string sep)
        {
            CultureInfo cultureInfo = (CultureInfo)Thread.CurrentThread.CurrentCulture.Clone();
            cultureInfo.NumberFormat.NumberDecimalSeparator = sep;
            Thread.CurrentThread.CurrentCulture = cultureInfo;
        }

        private static void ProcessArgs(string[] args)
        {
            if (args == null || args.Length <= 1)
                return;

            if (args[0].ToLower().StartsWith("win"))
                platform = DecompilePlatform.Win32;
            else if (args[0].ToLower().StartsWith("mobile"))
                platform = DecompilePlatform.Mobile;
            else
                return;

            if (!File.Exists(args[1]))
                return;

            inputFile = args[1];

            if (args.Length == 3 && !string.IsNullOrEmpty(args[2]))
                outputFile = args[2];
        }

        private static void PrintUsage()
        {
            Console.WriteLine("Usage: <platform> <input>\nor <version> <input> <output>");
            Console.WriteLine("Platform: Win or Mobile");
        }
    }
}
