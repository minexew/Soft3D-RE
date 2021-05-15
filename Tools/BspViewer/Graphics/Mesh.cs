using OpenTK;
using OpenTK.Graphics.OpenGL;
using System;

namespace BspViewer.Graphics
{
    class Mesh
    {
        public Vector3[] Positions { get; set; }
        public Vector3[] Normals { get; set; }
        public Vector2[] UVs { get; set; }

        public ushort[] Indices { get; set; }
        public string TextureName { get; set; }

        public RenderMode RenderMode { get; set; }

        public void Render()
        {
            if (RenderMode == RenderMode.None)
                return;
            if (!UsePositions())
                return;

            GL.Enable(EnableCap.VertexArray);

            if (UseNormals())
            {
                GL.Enable(EnableCap.NormalArray);
                GL.NormalPointer(NormalPointerType.Float, Vector3.SizeInBytes, Normals);
            }

            if (UseUVs())
            {
                GL.Enable(EnableCap.Blend);
                GL.BlendFunc(BlendingFactor.SrcAlpha, BlendingFactor.OneMinusSrcAlpha);

                GL.Enable(EnableCap.TextureCoordArray);
                GL.Enable(EnableCap.Texture2D);
                if(TextureName == null)
                    throw new Exception("Mesh texture not set!");
                int textureID = TexturePool.GetTextureIDByName(TextureName);

                if (textureID == -1)
                    throw new Exception("Mesh texture not loaded!");
                GL.BindTexture(TextureTarget.Texture2D, textureID);
                GL.TexCoordPointer(2, TexCoordPointerType.Float, Vector2.SizeInBytes, UVs);
            }

            GL.VertexPointer(3, VertexPointerType.Float, Vector3.SizeInBytes, Positions);
            switch(RenderMode)
            {
                case RenderMode.Triangles:
                    GL.DrawElements(PrimitiveType.Triangles, Indices.Length, DrawElementsType.UnsignedShort, Indices);
                    break;
                case RenderMode.Polygons:
                    GL.DrawElements(PrimitiveType.Polygon, Indices.Length, DrawElementsType.UnsignedShort, Indices);
                    break;
            } 

            if (UseUVs())
            {
                GL.Disable(EnableCap.Texture2D);
                GL.Disable(EnableCap.TextureCoordArray);
                GL.Disable(EnableCap.Blend);
            }

            if (UseNormals())
            {
                GL.Disable(EnableCap.NormalArray);
            }

            GL.Disable(EnableCap.VertexArray);
        }

        private bool UsePositions()
        {
            return Positions != null && Positions.Length > 0;
        }

        private bool UseUVs()
        {
            return UVs != null && UVs.Length > 0;
        }

        private bool UseNormals()
        {
            return Normals != null && Normals.Length > 0;
        }
    }
}
