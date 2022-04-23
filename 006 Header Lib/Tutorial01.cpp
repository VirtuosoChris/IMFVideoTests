#define WINVER _WIN32_WINNT_WIN8
#include <Windows.h>
#include <d3d11_1.h>
#include <directxcolors.h>
#include "resource.h"
#include <atltypes.h>
#include <d3d11_3.h>

// https://docs.microsoft.com/en-us/windows/win32/api/mfobjects/nn-mfobjects-imfbytestream
// byte stream could be created async too

#include <assert.h>


#include <dxgi.h>
#define GLEW_STATIC
#include "include/GL/glew.h"
#include "include/GL/wglew.h"
#include <gl/GL.h>

#pragma comment(lib, "glew32s.lib")
#pragma comment(lib, "opengl32.lib")

#include "IMFVideo.h"

using namespace DirectX;

//--------------------------------------------------------------------------------------
// Global Variables
//--------------------------------------------------------------------------------------
HINSTANCE               g_hInst = nullptr;
HDC                     g_hDCGL = nullptr;

HWND                    hWndGL = nullptr;


//--------------------------------------------------------------------------------------
// Forward declarations
//--------------------------------------------------------------------------------------
HRESULT InitWindow( HINSTANCE hInstance, int nCmdShow );
LRESULT CALLBACK    WndProc( HWND, UINT, WPARAM, LPARAM );


//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain( _In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow )
{
    UNREFERENCED_PARAMETER( hPrevInstance );
    UNREFERENCED_PARAMETER( lpCmdLine );

    if( FAILED( InitWindow( hInstance, nCmdShow ) ) )
        return 0;

    Virtuoso::Win32::IMFMediaPlayer virtuosoPlayer;

    virtuosoPlayer.playMedia("./sample.mp4");

    GLenum err = GL_FRAMEBUFFER_COMPLETE;

    // Main message loop
    MSG msg = {0};
    while( WM_QUIT != msg.message )
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        if( PeekMessage( &msg, nullptr, 0, 0, PM_REMOVE ) )
        {
            TranslateMessage( &msg );
            DispatchMessage( &msg );
        }
        else
        {
            ///////////////////////////////////////////////////////

            if (virtuosoPlayer.updateStep())
            {
                virtuosoPlayer.glState.lockForGL();

                    glViewport(0, 0, 800, 600);
                    glClearColor(1.f, 0.f, 0.f, 1.f);
                    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

                    // blit the video player's gl fbo to the main fbo 0
                    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
                    glBindFramebuffer(GL_READ_FRAMEBUFFER, virtuosoPlayer.glState.readFBO);

                    assert((err = glCheckFramebufferStatus(GL_READ_FRAMEBUFFER)) == GL_FRAMEBUFFER_COMPLETE);

                    glBlitFramebuffer(//readFB, targetFB,
                        0, 0, 800, 600,
                        0, 600, 800, 0,
                        GL_COLOR_BUFFER_BIT,
                        GL_LINEAR
                    );

                virtuosoPlayer.glState.unlockForGL();


                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                SwapBuffers(g_hDCGL);
            }
        }
    }

    return ( int )msg.wParam;
}


void InitGL(HWND hWndGL)
{
    static	PIXELFORMATDESCRIPTOR pfd =
    {
        sizeof(PIXELFORMATDESCRIPTOR),              // Size Of This Pixel Format Descriptor
        1,                                          // Version Number
        PFD_DRAW_TO_WINDOW |                        // Format Must Support Window
        PFD_SUPPORT_OPENGL |                        // Format Must Support OpenGL
        PFD_DOUBLEBUFFER,                           // Must Support Double Buffering
        PFD_TYPE_RGBA,                              // Request An RGBA Format
        32,                                         // Select Our Color Depth
        0, 0, 0, 0, 0, 0,                           // Color Bits Ignored
        0,                                          // No Alpha Buffer
        0,                                          // Shift Bit Ignored
        0,                                          // No Accumulation Buffer
        0, 0, 0, 0,                                 // Accumulation Bits Ignored
        16,                                         // 16Bit Z-Buffer (Depth Buffer)  
        0,                                          // No Stencil Buffer
        0,                                          // No Auxiliary Buffer
        PFD_MAIN_PLANE,                             // Main Drawing Layer
        0,                                          // Reserved
        0, 0, 0                                     // Layer Masks Ignored
    };

    g_hDCGL = GetDC(hWndGL);
    int PixelFormat = ChoosePixelFormat(g_hDCGL, &pfd);
    SetPixelFormat(g_hDCGL, PixelFormat, &pfd);
    HGLRC hRC = wglCreateContext(g_hDCGL);
    wglMakeCurrent(g_hDCGL, hRC);


    GLenum x = glewInit();
}


//--------------------------------------------------------------------------------------
// Register class and create window
//--------------------------------------------------------------------------------------
HRESULT InitWindow( HINSTANCE hInstance, int nCmdShow )
{
    // Register class
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof( WNDCLASSEX );
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon( hInstance, ( LPCTSTR )IDI_TUTORIAL1 );
    wcex.hCursor = LoadCursor( nullptr, IDC_ARROW );
    wcex.hbrBackground = ( HBRUSH )( COLOR_WINDOW + 1 );
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = L"TutorialWindowClass";
    wcex.hIconSm = LoadIcon( wcex.hInstance, ( LPCTSTR )IDI_TUTORIAL1 );
    if( !RegisterClassEx( &wcex ) )
        return E_FAIL;

    // Create window
    g_hInst = hInstance;
    RECT rc = { 0, 0, 800, 600 };
    AdjustWindowRect( &rc, WS_OVERLAPPEDWINDOW, FALSE );

    /*g_hWnd = CreateWindow(L"TutorialWindowClass", L"Direct3D 11 Tutorial 1: Direct3D 11 Basics",
                           WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                           CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInstance,
                           nullptr );*/

    hWndGL = CreateWindow(L"TutorialWindowClass", L"GL Window",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInstance,
        nullptr);

    if (!hWndGL)
        return E_FAIL;

    ShowWindow(hWndGL, nCmdShow);

    InitGL(hWndGL);

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Called every time the application receives a message
//--------------------------------------------------------------------------------------
LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    PAINTSTRUCT ps;
    HDC hdc;

    switch( message )
    {
    case WM_PAINT:
        hdc = BeginPaint( hWnd, &ps );
        EndPaint( hWnd, &ps );
        break;

    case WM_DESTROY:
        PostQuitMessage( 0 );
        break;

        // Note that this tutorial does not handle resizing (WM_SIZE) requests,
        // so we created the window without the resize border.

    default:
        return DefWindowProc( hWnd, message, wParam, lParam );
    }

    return 0;
}


