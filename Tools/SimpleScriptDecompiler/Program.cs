using SimpleScriptDecompiler.Decompiler;
using SimpleScriptDecompiler.Script;
using System;
using System.Globalization;
using System.IO;
using System.Threading;

namespace SimpleScriptDecompiler
{
    class Program
    {
        private enum TargetPlatform
        {
            None,
            Win32,
            Mobile,
            PlatFormCount
        }

        private static TargetPlatform platform;
        private static string inputFile;
        private static string outputFile;


        static void Main(string[] args)
        {
            //args = new string[] { "mobile", "mobile_scripts/ep0104.sst" };
            
            ProcessArgs(args);

            if(platform == TargetPlatform.None || string.IsNullOrEmpty(inputFile))
            {
                PrintUsage();
                return;
            }

            //float "1,23" -> "1.23"
            ChangeFloatSeparator(".");

            IScriptInterface script;
            IDecompilerInterface decompiler;
            
            switch(platform)
            {
                case TargetPlatform.Win32:
                    script = new Win32SimpleScript();
                    decompiler = new Win32Decompiler();
                    break;
                case TargetPlatform.Mobile:
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
                platform = TargetPlatform.Win32;
            else if (args[0].ToLower().StartsWith("mobile"))
                platform = TargetPlatform.Mobile;
            else
                return;

            if (!File.Exists(args[1]))
                return;

            inputFile = args[1];

            outputFile = "out.sst";

            if (args.Length == 3 && !string.IsNullOrEmpty(args[2]))
                outputFile = args[2];
        }

        private static void PrintUsage()
        {
            Console.WriteLine("Usage: <platform> <input> or <platform> <input> <output>");
            PrintPlatforms();
        }

        private static void PrintPlatforms()
        {
            Console.WriteLine("Platforms:");
            for (int i = 0; i < (int)TargetPlatform.PlatFormCount; i++)
            {
                TargetPlatform platform = (TargetPlatform)i;
                if (platform != TargetPlatform.None)
                    Console.WriteLine(platform.ToString());
            }
        }
    }
}
