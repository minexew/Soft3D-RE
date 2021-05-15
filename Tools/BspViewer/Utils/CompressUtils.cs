using System;

namespace BspViewer.Utils
{
    class CompressUtils
    {
        private const string alph = "abcdefghijklmnopqrstuvwxyz_1234";

        public static string UnpackString(int[] data) //input 6 * 4 output 36?
        {
            int v4;
            int v3;
            char[] result = new char[6 * 4];
            for (int i = 0; i < 4; ++i)
            {
                v4 = 0;
                for (int j = 0; j < 6; ++j)
                {
                    v3 = ((31 << v4) & data[i]) >> v4;
                    if (v3 == 31)
                        return new string(result).Replace("\0", "");
                    result[j + 2 * i + 4 * i] = alph[v3];

                    v4 += 5;
                }

            }

            return new string(result).Replace("\0", ""); ;
        }
    }
}
