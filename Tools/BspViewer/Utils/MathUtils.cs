using System;

namespace BspViewer.Utils
{
    class MathUtils
    {
        public static float Int2Float(int i)
        {
            return (float)i / 0x10000;
        }

        //TODO: Check this
        public static int Float2Int(float f)
        {
            return (int)(f * 0x10000);
        }
    }
}
