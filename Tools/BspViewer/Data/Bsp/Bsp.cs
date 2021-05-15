using BspViewer.Data.Bsp.Lumps;
using BspViewer.Data.Model;
using BspViewer.Data.Texture;
using BspViewer.Graphics;
using BspViewer.Graphics.Camera;
using BspViewer.IO;
using BspViewer.Scene;
using BspViewer.Utils;
using OpenTK;
using System;
using System.Collections.Generic;
using System.IO;
using static BspViewer.Data.Bsp.Lumps.LFace;
using static BspViewer.Data.Bsp.Lumps.LTextInfo;

namespace BspViewer.Data.Bsp
{
    class Bsp : ILoadableResource
    {
        //https://www.flipcode.com/archives/Quake_2_BSP_File_Format.shtml

        private struct LumpEntry
        {
            public int offset;
            public int length;
        }
        //Need to setup map
        private Lump[] bspLumps;

        //Root object on scene, contains all objects
        private WorldObject rootObject;


        public void LoadFromFile(string path)
        {
            CustomBinaryReader reader = new CustomBinaryReader(path);
            int ident = reader.ReadInt32(); //ident S3D, ignored by game

            if(ident != 0x443353)
            {
                Console.WriteLine("Bad bsp ident! [{0}]", ident.ToString("X"));
                return;
            }

            float version = reader.ReadSingle();

            if (version != 7.6f)
            {
                Console.WriteLine("Bad bsp version! [{0}]", version);
                return;
            }

            //may be ignored
            reader.ReadSingle();
            reader.ReadSingle();
            reader.ReadSingle();
            reader.ReadSingle();
            reader.ReadSingle();
            reader.ReadSingle();

            int lumpsCount = (int)LumpTypes.Count;
            LumpEntry[] lumpEntries = new LumpEntry[lumpsCount];
            for (int i = 0; i < lumpsCount; i++)
            {
                LumpEntry sec = new LumpEntry();
                sec.offset = reader.ReadInt32();
                sec.length = reader.ReadInt32();
                lumpEntries[i] = sec;
            }

            bspLumps = new Lump[lumpsCount];

            for (int i = 0; i < lumpsCount; i++)
            {
                LumpEntry entry = lumpEntries[i];
                LumpTypes type = (LumpTypes)i;
                reader.BaseStream.Seek(entry.offset, SeekOrigin.Begin);

                if (entry.length == 0)
                {
                    Console.WriteLine("Lump {0} size {1}, skipped", type.ToString(), entry.length);
                    continue;
                }

                Console.WriteLine("Process lump {0}", type.ToString());
                byte[] data = reader.ReadBytes(entry.length);
                bspLumps[i] = LoadLump(type, data);
            }

        }

        public void Setup()
        {
            LoadTextures();
            LoadLightmaps();
            SetupRenderObjects();
            bspLumps = null;
        }

        private Lump LoadLump(LumpTypes type, byte[] data)
        {
            string ns = typeof(Lumps.Entry).Namespace;
            try
            {
                return (Lump)ReflectionUtils.CallToClass(string.Format("{0}.{1}{2}",
                    ns, Lump.PREFIX, type.ToString()),
                    "Load", new object[] { data });
            }
            catch(InvalidCastException)
            {
                Console.WriteLine("Lump {0} not implemented.", type.ToString());
                return null;
            }
            catch(MissingMemberException)
            {
                Console.WriteLine("Lump {0} not registered.", type.ToString());
                return null;
            }
        }

        private dynamic GetLump(LumpTypes type)
        {
            if(bspLumps == null || bspLumps.Length != (int)LumpTypes.Count)
                throw new Exception("Lumps not loaded!");
            if ((int)type < 0 || (int)type >= bspLumps.Length)
                throw new IndexOutOfRangeException("Illegal lump index!");

            return bspLumps[(int)type];
        }

        private void LoadTextures()
        {
            LTextInfo tiLump = GetLump(LumpTypes.TextInfo);
            TextInfo[] texturesInfo = tiLump.TexturesInfo;

            for (int i = 0; i < texturesInfo.Length; i++)
            {
                var tex = ResourceLoader.LoadFileFromPath(ResourceType.Texture,
                    texturesInfo[i].textureName);
                TexturePool.LoadTexture((ITexture)tex);
            }
        }

        private void LoadLightmaps()
        {
            //TODO
        }

        private void SetupRenderObjects()
        {
            rootObject = new WorldObject();
            List<WorldObject> tempRenderList = new List<WorldObject>();

            //Faces to models
            Vector3[] positions = ((LVertex)GetLump(LumpTypes.Vertex)).Positions;

            LFace faceLump = GetLump(LumpTypes.Face);
            LEdge eLump = GetLump(LumpTypes.Edge);
            var Edges = eLump.Edges;
            LFaceEdge feLump = GetLump(LumpTypes.FaceEdge);
            var FaceEdges = feLump.FaceEdges;
            LTextInfo tiLump = GetLump(LumpTypes.TextInfo);
            var texturesInfo = tiLump.TexturesInfo;

            for (int i = 0; i < faceLump.Faces.Length; i++)
            {
                SOJModel model = new SOJModel();
                Mesh mesh = new Mesh();
                Face face = faceLump.Faces[i];
                List<Vector3> meshPositions = new List<Vector3>();
                List<ushort> meshIndices = new List<ushort>();
                List<Vector2> meshUVs = new List<Vector2>();
                int index = face.firstEdgeIndex;
                bool uvsExist = face.texInfoIndex != -1;


                TextInfo texInfo = new TextInfo();
                ITexture texture = null;
                if (uvsExist)
                {
                    texInfo = texturesInfo[face.texInfoIndex];
                    texture = TexturePool.GetTextureByName(texInfo.textureName);
                }
                uvsExist &= texture != null;

                for (int j = 0; j < face.edgesCount; j++)
                {
                    ushort vertexIndex;
                    if (FaceEdges[face.firstEdgeIndex + j] < 0)
                        vertexIndex = Edges[Math.Abs(FaceEdges[index])].a;
                    else
                        vertexIndex = Edges[FaceEdges[index]].b;

                    meshPositions.Add(positions[vertexIndex]);
                    //!!!
                    meshIndices.Add((ushort)j);

                    if (uvsExist)
                    {
                        float u = (Vector3.Dot(positions[vertexIndex],texInfo.uAxis) +
                            texInfo.uOffset) / texture.GetWidth();
                        float v = (Vector3.Dot(positions[vertexIndex], texInfo.vAxis) +
                            texInfo.vOffset) / texture.GetHeight();
                        meshUVs.Add(new Vector2(u, v));
                    }

                    index++;
                }
                mesh.Positions = meshPositions.ToArray();
                mesh.Indices = meshIndices.ToArray();

                if(uvsExist)
                {
                    mesh.UVs = meshUVs.ToArray();
                    mesh.TextureName = texture.GetName();
                }
                mesh.RenderMode = RenderMode.Polygons;
                model.AddMesh(mesh);
                tempRenderList.Add(model);

            }

            LModel modelLump = GetLump(LumpTypes.Model);
            if(modelLump != null)
            {
                foreach (SOJModel model in modelLump.Models)
                    tempRenderList.Add(model);
            }

            //First render models
            tempRenderList.Reverse();

            //Link all objects to root
            foreach (WorldObject obj in tempRenderList)
            {
                if(obj != null)
                    obj.LinkTo(rootObject);
            }
        }

        private bool SetupDone()
        {
            return rootObject != null;
        }

        public void Render(Camera camera)
        {
            if (SetupDone())
                camera.Render(rootObject);
        }
    }
}
