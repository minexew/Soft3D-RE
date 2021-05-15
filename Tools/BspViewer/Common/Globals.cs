using System;
using System.Collections.Generic;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace BspViewer.Common
{
    class Globals
    {
        public const string VERSION = "0.1";

        //Original win32 7ds
        public const int DEFAULT_WIDTH = 208 * 3;
        public const int DEFAULT_HEIGHT = 176 * 3;

        public static int WIDTH = DEFAULT_WIDTH;
        public static int HEIGHT = DEFAULT_HEIGHT;

        public static Point WinStart { get; set; }
    }
}
