using System;


namespace BspViewer.Data.Texture
{
    public interface ITexture : ILoadableResource
    {
        byte[] GetPixelData();
        string GetName();
        int GetWidth();
        int GetHeight();
    }
}
