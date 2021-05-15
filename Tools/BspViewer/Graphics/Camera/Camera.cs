using BspViewer.Scene;
using OpenTK;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace BspViewer.Graphics.Camera
{
    public abstract class Camera : WorldObject
    {
        public Vector3 Front { get; set; }
        public Vector3 Up { get; set; }
        public Vector3 Right { get; set; }

        protected float _pitch;
        protected float _yaw = -MathHelper.PiOver2;
        protected float _fov = MathHelper.PiOver2;

        public float AspectRatio { protected get; set; }

        public float Pitch
        {
            get => MathHelper.RadiansToDegrees(_pitch);
            set
            {
                var angle = MathHelper.Clamp(value, -89f, 89f);
                _pitch = MathHelper.DegreesToRadians(angle);
                UpdateVectors();
            }
        }

        public float Yaw
        {
            get => MathHelper.RadiansToDegrees(_yaw);
            set
            {
                _yaw = MathHelper.DegreesToRadians(value);
                UpdateVectors();
            }
        }

        public float Fov
        {
            get => MathHelper.RadiansToDegrees(_fov);
            set
            {
                var angle = MathHelper.Clamp(value, 1f, 45f);
                _fov = MathHelper.DegreesToRadians(angle);
            }
        }

        //TODO: Projection types
        public abstract Matrix4 GetProjectionMatrix();
        public abstract Matrix4 GetViewMatrix();

        public abstract void Init(Vector3 position, float aspectRatio);
        public abstract void Render(WorldObject rootObject);

        protected abstract void UpdateVectors();
    }
}
