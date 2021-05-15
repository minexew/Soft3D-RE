using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace BspViewer.Data.Bsp
{
    public enum LumpTypes : byte
    {
        Vertex,
        Edge,
        FaceEdge,
        Plane,
        TextInfo,
        Face,
        Brush,
        BrushSide,
        VisBrush,
        VisFace,
        VertLights,
        Model,
        EPair,
        EBrush,
        Ent,
        ELink,
        VisEntIdx,
        UNUSED,
        EDictIdx,
        LeafBrush,
        LeafVBrush,
        PathPairs,
        PathNodes,
        Leaf,
        Node,
        PVS,
        //LightmapStyles, Not include?
        Lightmap,
        Count
    }
}
