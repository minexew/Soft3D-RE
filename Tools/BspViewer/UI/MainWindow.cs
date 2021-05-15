using BspViewer.Common;
using OpenTK;
using OpenTK.Graphics.OpenGL;
using System;
using System.Drawing;

namespace BspViewer.UI
{
    class MainWindow : GameWindow
    {
        public MainWindow(int width, int height) : base(width, height)
        {
            GL.Enable(EnableCap.DepthTest);
            SetViewport(width, height);
            WindowBorder = WindowBorder.Fixed;
        }

        public void SetViewport(int width, int height)
        {
            GL.Viewport(0, 0, width, height);
        }

        protected override void OnLoad(EventArgs e)
        {
            GL.ClearColor(Color.Black);

            Core.Init();
            base.OnLoad(e);
        }

        protected override void OnResize(EventArgs e)
        {
            SetViewport(Width, Height);
            base.OnResize(e);
        }

        protected override void OnUpdateFrame(FrameEventArgs e)
        {
            Globals.WinStart = PointToScreen(Point.Empty);
            Core.Update();
            base.OnUpdateFrame(e);
        }



        protected override void OnRenderFrame(FrameEventArgs e)
        {
            GL.Clear(ClearBufferMask.ColorBufferBit | ClearBufferMask.DepthBufferBit);
            GL.ClearColor(Color.Black);

            Core.Render();

            Context.SwapBuffers();
            base.OnRenderFrame(e);
        }
    }
}
