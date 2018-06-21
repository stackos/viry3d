/*
* Viry3D
* Copyright 2014-2018 by Stack - stackos@qq.com
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include "Application.h"
#include "graphics/Display.h"
#include "graphics/Camera.h"
#include "graphics/Shader.h"
#include "graphics/Material.h"
#include "graphics/MeshRenderer.h"

using namespace Viry3D;

class App
{
public:
    Camera* m_camera;
    Ref<Material> m_material;

    App()
    {
        m_camera = Display::GetDisplay()->CreateCamera();

        String vs = R"(
UniformBuffer(0, 0) uniform UniformBuffer00
{
	mat4 mvp;
} u_buf_0_0;

Input(0) vec4 a_pos;
Input(1) vec2 a_uv;

Output(0) vec2 v_uv;

void main()
{
	gl_Position = a_pos * u_buf_0_0.mvp;
	v_uv = a_uv;

	vulkan_convert();
}
)";
        String fs = R"(
precision mediump float;
      
UniformTexture(0, 1) uniform sampler2D u_texture;

Input(0) vec2 v_uv;

Output(0) vec4 o_frag;

void main()
{
    //o_frag = texture(u_texture, v_uv);
    o_frag = vec4(1, 1, 1, 1);
}
)";
        RenderState render_state;
        auto shader = RefMake<Shader>(
            vs,
            Vector<String>(),
            fs,
            Vector<String>(),
            render_state);
        m_material = RefMake<Material>(shader);
        auto renderer = RefMake<MeshRenderer>();
        renderer->SetMaterial(m_material);
    }

    ~App()
    {
        m_material.reset();
        Display::GetDisplay()->DestroyCamera(m_camera);
        m_camera = nullptr;
    }

    void Update()
    {
        
    }
};


#if VR_WINDOWS
#include <Windows.h>

static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_CLOSE:
            PostQuitMessage(0);
            break;

        case WM_SIZE:
            if (wParam != SIZE_MINIMIZED)
            {
                int width = lParam & 0xffff;
                int height = (lParam & 0xffff0000) >> 16;

                if (Display::GetDisplay())
                {
                    Display::GetDisplay()->OnResize(width, height);
                }
            }
            break;

        default:
            break;
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    Application::SetName("viry3d-vk-demo");
    int width = 1280;
    int height = 720;

    String name = Application::Name();

    WNDCLASSEX win_class;
    ZeroMemory(&win_class, sizeof(win_class));

    win_class.cbSize = sizeof(WNDCLASSEX);
    win_class.style = CS_HREDRAW | CS_VREDRAW;
    win_class.lpfnWndProc = WindowProc;
    win_class.cbClsExtra = 0;
    win_class.cbWndExtra = 0;
    win_class.hInstance = hInstance;
    win_class.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
    win_class.lpszMenuName = nullptr;
    win_class.lpszClassName = name.CString();
    win_class.hCursor = LoadCursor(nullptr, IDC_ARROW);
    win_class.hIcon = (HICON) LoadImage(nullptr, "icon.ico", IMAGE_ICON, SM_CXICON, SM_CYICON, LR_LOADFROMFILE);
    win_class.hIconSm = win_class.hIcon;

    if (!RegisterClassEx(&win_class))
    {
        return 0;
    }

    DWORD style = WS_OVERLAPPEDWINDOW;
    DWORD style_ex = 0;

    RECT wr = { 0, 0, width, height };
    AdjustWindowRect(&wr, style, FALSE);

    int x = (GetSystemMetrics(SM_CXSCREEN) - width) / 2 + wr.left;
    int y = (GetSystemMetrics(SM_CYSCREEN) - height) / 2 + wr.top;

    HWND hwnd = CreateWindowEx(
        style_ex,			// window ex style
        name.CString(),		// class name
        name.CString(),		// app name
        style,			    // window style
        x, y,				// x, y
        wr.right - wr.left, // width
        wr.bottom - wr.top, // height
        nullptr,		    // handle to parent
        nullptr,            // handle to menu
        hInstance,			// hInstance
        nullptr);           // no extra parameters
    if (!hwnd)
    {
        return 0;
    }

    ShowWindow(hwnd, SW_SHOW);

    Display* display = new Display(hwnd, width, height);

    Ref<App> app = RefMake<App>();

    bool exit = false;
    MSG msg;

    while (true)
    {
        while (::PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (WM_QUIT == msg.message)
            {
                exit = true;
                break;
            }
            else
            {
                ::TranslateMessage(&msg);
                ::DispatchMessage(&msg);
            }
        }

        if (exit)
        {
            break;
        }

        app->Update();

        display->OnDraw();

        ::Sleep(1);
    }

    app.reset();

    delete display;

    return 0;
}
#endif
