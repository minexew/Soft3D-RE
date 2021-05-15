using System;

namespace BspViewer.Data
{
    public interface ILoadableResource
    {
        void LoadFromFile(string path);
        //void LoadFromPak(string name);
    }
}
