using BspViewer.Data.Texture;
using BspViewer.Graphics;
using BspViewer.IO;
using BspViewer.Scene;
using OpenTK;
using System;
using System.Collections.Generic;
using System.IO;

namespace BspViewer.Data.Model
{
    class SOJModel : WorldObject, ILoadableResource
    {
        private List<Mesh> meshes;
        private int faceCount;
        private int unkCount;
        private int vertexSize;
        private int vertexCount;
        private int vertexSectionSize;

        private int indicesCount;
        private int indicesSectionSize;

        public SOJModel()
        {
            meshes = new List<Mesh>();
        }

        public void AddMesh(Mesh mesh)
        {
            meshes.Add(mesh);
        }

        public void LoadFromFile(string path)
        {
            CustomBinaryReader reader = new CustomBinaryReader(path);
            if(reader.ReadInt32() != 1)
                throw new NotImplementedException();
            reader.BaseStream.Seek(0x14, SeekOrigin.Begin);
            bool textureExist = reader.ReadInt16() == 1;
            bool alphaTexExist = reader.ReadInt16() == 1; //its not true
            if (!textureExist)
                throw new NotImplementedException();
            if (alphaTexExist)
                throw new NotImplementedException();

            string texName = reader.ReadPackedString();

            ITexture texture = (ITexture)ResourceLoader.LoadFileFromPath(
                ResourceType.Texture,
                texName);
            TexturePool.LoadTexture(texture);

            reader.BaseStream.Seek(0x4C, SeekOrigin.Begin);
            Mesh mesh = new Mesh();
            faceCount = reader.ReadInt32();
            unkCount = reader.ReadInt32();
            vertexSize = reader.ReadInt32();
            vertexCount = reader.ReadInt32();
            vertexSectionSize = reader.ReadInt32();

            mesh.Positions = new Vector3[vertexCount];
            mesh.Normals = new Vector3[vertexCount];
            mesh.UVs = new Vector2[vertexCount];
            for (int i = 0; i < vertexCount; i++)
            {
                switch (vertexSize)
                {
                    case 24:
                        mesh.Positions[i] = reader.ReadPackedVec3();
                        mesh.UVs[i] = reader.ReadPackedVec2();
                        float unk_24 = reader.ReadSingle();
                        break;
                    case 32:
                        mesh.Positions[i] = reader.ReadPackedVec3();
                        mesh.Normals[i] = reader.ReadPackedVec3();
                        mesh.UVs[i] = reader.ReadPackedVec2();
                        break;
                    case 36:
                        reader.ReadInt32();
                        mesh.Positions[i] = reader.ReadPackedVec3();
                        mesh.Normals[i] = reader.ReadPackedVec3();
                        mesh.UVs[i] = reader.ReadPackedVec2();
                        break;
                    default:
                        throw new Exception("Unknown vertex size! " + vertexSize);
                }

                mesh.UVs[i].X /= texture.GetWidth();
                mesh.UVs[i].Y /= texture.GetHeight();
            }

            indicesCount = reader.ReadInt32();
            indicesSectionSize = reader.ReadInt32();
            mesh.Indices = new ushort[indicesCount];

            for (int i = 0; i < indicesCount; i++)
                mesh.Indices[i] = reader.ReadUInt16();

            Vector3 unk1 = reader.ReadPackedVec3();

            reader.Close();

            mesh.TextureName = texName;
            mesh.RenderMode = RenderMode.Triangles;
            meshes.Add(mesh);

        }


        public override void Render(Matrix4 projection, Matrix4 view)
        {
            base.Render(projection, view);

            foreach (Mesh mesh in meshes)
            {
                if (mesh.RenderMode == RenderMode.None)
                    continue;

                mesh.Render();
            }
        }
    }
}
