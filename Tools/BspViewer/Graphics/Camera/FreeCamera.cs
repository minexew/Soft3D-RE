using BspViewer.Scene;
using OpenTK;
using System;

namespace BspViewer.Graphics.Camera
{
    public class FreeCamera : Camera
    {
        public FreeCamera()
        {
            Front = -Vector3.UnitZ;
            Up = Vector3.UnitY;
            Right = Vector3.UnitX;
        }

        public override void Init(Vector3 position, float aspectRatio)
        {
            Position = position;
            AspectRatio = aspectRatio;
        }

        public override Matrix4 GetProjectionMatrix()
        {
            return Matrix4.CreatePerspectiveFieldOfView(_fov, AspectRatio, 0.01f, 2000f);
        }

        public override Matrix4 GetViewMatrix()
        {
            Matrix4 result = Matrix4.LookAt(Position, Position + Front, Up);
            //!!! Mirror x axis
            result.Column0 = -result.Column0;
            return result;
        }

        protected override void UpdateVectors()
        {
            Front = new Vector3(
                (float)Math.Cos(_pitch) * (float)Math.Cos(_yaw),
                (float)Math.Sin(_pitch),
                (float)Math.Cos(_pitch) * (float)Math.Sin(_yaw));

            Front = Vector3.Normalize(Front);
            Right = Vector3.Normalize(Vector3.Cross(Front, Vector3.UnitY));
            Up = Vector3.Normalize(Vector3.Cross(Right, Front));
        }

        public override void Render(WorldObject rootObject)
        {
            Matrix4 proj = GetProjectionMatrix();
            Matrix4 view = GetViewMatrix();

            for (WorldObject obj = rootObject; obj != null; obj = obj.NextObject(rootObject))
            {
                obj.Render(proj, view);

                for (WorldObject parent = obj.Parent; parent != null; parent = parent.Parent)
                    parent.Render(proj, view);
            }
        }

    }
}
