using BspViewer.Common;
using BspViewer.Graphics.Camera;
using OpenTK;
using OpenTK.Input;
using System;

namespace BspViewer.Input
{
    class InputSystem
    {
        private const float DEFAULT_CAMERA_SPEED = 0.5f;

        private static float cameraSpeed = DEFAULT_CAMERA_SPEED;
        private static bool firstMove = true;
        private static Vector2 lastPos;
        private static float sensitivity = 0.2f;

        public static void Update(Camera camera)
        {
            KeyboardState input = Keyboard.GetState();
            MouseState mouse = Mouse.GetState();
            
            ProcessCameraInput(camera, input, mouse);

        }

        private static void ProcessCameraInput(Camera camera, KeyboardState input, MouseState mouse)
        {
            //Camera speed
            if (input.IsKeyDown(Key.Number1))
            {
                cameraSpeed = 0.1f;
            }

            if (input.IsKeyDown(Key.Number2))
            {
                cameraSpeed = DEFAULT_CAMERA_SPEED;
            }

            if (input.IsKeyDown(Key.Number3))
            {
                cameraSpeed = 2.0f;
            }

            if (input.IsKeyDown(Key.Number4))
            {
                cameraSpeed = 5.0f;
            }

            //Movement
            if (input.IsKeyDown(Key.W))
            {
                camera.Position += camera.Front * cameraSpeed; // Forward
            }

            if (input.IsKeyDown(Key.S))
            {
                camera.Position -= camera.Front * cameraSpeed; // Backwards
            }

            if (input.IsKeyDown(Key.A))
            {
                camera.Position += camera.Right * cameraSpeed; // Left
            }

            if (input.IsKeyDown(Key.D))
            {
                camera.Position -= camera.Right * cameraSpeed; // Right
            }

            if (input.IsKeyDown(Key.Space))
            {
                camera.Position += camera.Up * cameraSpeed; // Up
            }

            if (input.IsKeyDown(Key.C))
            {
                camera.Position -= camera.Up * cameraSpeed; // Down
            }


            if (mouse.RightButton == ButtonState.Pressed)
            {
                if (firstMove)
                {

                    lastPos = new Vector2(mouse.X, mouse.Y);
                    firstMove = false;
                }
                else
                {
                    var deltaX = mouse.X - lastPos.X;
                    var deltaY = mouse.Y - lastPos.Y;
                    lastPos = new Vector2(mouse.X, mouse.Y);

                    camera.Yaw -= deltaX * sensitivity;
                    camera.Pitch -= deltaY * sensitivity;
                }
                
                //Set cursor on center
                var loc = Globals.WinStart;
                Mouse.SetPosition(loc.X + Globals.WIDTH / 2, loc.Y + Globals.HEIGHT / 2);
            }

            if (input.IsKeyDown(Key.F))
            {
                Console.WriteLine("Cam pos {0}", camera.Position.ToString());
            }
        }
    }
}
