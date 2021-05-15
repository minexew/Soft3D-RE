using OpenTK;
using OpenTK.Graphics.OpenGL;
using System;

namespace BspViewer.Scene
{
    public class WorldObject
    {
        public WorldObject Parent { get;  private set; }
        public WorldObject Child { get; private set; }
        public WorldObject Next { get; private set; }
        public WorldObject Previous { get; private set; }

        public Vector3 Position
        {
            get => ModelTransform.ExtractTranslation();
            set => ModelTransform = ModelTransform.ClearTranslation() *
                Matrix4.CreateTranslation(value);
        }
        //TODO: Rotation, scale

        public string Name { get; set; }
        public Matrix4 ModelTransform { get; set; }


        public WorldObject()
        {
            Name = string.Empty;
            ModelTransform = Matrix4.Identity;
            Parent = null;
            Child = null;
        }

        public void LinkTo(WorldObject parent)
        {
            Unlink();
            Next = parent.Child;
            if (Next != null)
                Next.Previous = this;

            Parent = parent;
            Parent.Child = this;
        }

        public void Unlink()
        {
            if (Parent == null)
                return;

            //First child
            if(Parent.Child == this)
            {
                Parent.Child = Next;
                if (Next != null)
                    Next.Previous = null;
            }
            else
            {
                if (Previous != null)
                    Previous.Next = Next;
                if (Next != null)
                    Next.Previous = Previous;
            }

            Parent = null;
            Next = null;
            Previous = null;
        }

        public WorldObject NextObject(WorldObject root)
        {
            if (Child != null)
                return Child;
            else if (root == this)
                return null;
            else if (Next != null)
                return Next;
            else
            {
                for (WorldObject obj = Parent; obj != null && obj != root; obj = obj.Parent)
                    if (obj.Next != null)
                        return obj.Next;
            }

            return null;
        }

        public Matrix4 WorldTransform()
        {
            Matrix4 result = ModelTransform;
            for (WorldObject obj = Parent; obj != null; obj = obj.Parent)
                result = obj.ModelTransform * result;

            return result;
        }

        public static void PrintHierarchy(WorldObject root)
        {
            for (WorldObject obj = root; obj != null; obj = obj.NextObject(root))
            {
                int depth = 0;

                for (WorldObject parent = obj.Parent; parent != null; parent = parent.Parent)
                    depth++;
                Console.WriteLine("{0}{1}", new string('-', depth * 4), obj.Name);

            }
        }

        public virtual void Render(Matrix4 projection, Matrix4 view)
        {
            GL.MatrixMode(MatrixMode.Projection);

            //OpenTK Spec
            Matrix4 result = WorldTransform() * view * projection;
            GL.LoadMatrix(ref result);
        }

    }
}
