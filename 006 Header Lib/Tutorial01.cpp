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
HWND                    g_hWnd = nullptr;
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

#if 0
    DXGI_SWAP_CHAIN_DESC1 sd = {};
    sd.Width = 800;
    sd.Height = 600;
    sd.Format = TEXFMT;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_BACK_BUFFER;
    sd.BufferCount = 4;
    sd.Scaling = DXGI_SCALING_NONE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;

    HANDLE hDevice = 0;
    DXGIManager->OpenDeviceHandle(&hDevice);

    ID3D11Device* ldevice;
    DXGIManager->LockDevice(hDevice, __uuidof(ID3D11Device), (void**)&ldevice, TRUE);

    {
        IDXGIDevice2* spDXGIDevice;
        ldevice->QueryInterface<IDXGIDevice2>(&spDXGIDevice);

        spDXGIDevice->SetMaximumFrameLatency(1);

        IDXGIAdapter* spDXGIAdapter;
        spDXGIDevice->GetParent(IID_PPV_ARGS(&spDXGIAdapter));

        IDXGIFactory2* spDXGIFactory;
        spDXGIAdapter->GetParent(IID_PPV_ARGS(&spDXGIFactory));

        //auto hr = spDXGIFactory->CreateSwapChainForHwnd(g_pd3dDevice, g_hWnd, &sd, nullptr, nullptr, &g_pSwapChain);
    }

    DXGIManager->UnlockDevice(hDevice, 0);
#endif

    /*{// proof the new swap chain works
        ID3D11Texture2D* pBackBuffer = nullptr;
        auto hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pBackBuffer));
        if (FAILED(hr))
            return hr;

        hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_pRenderTargetView);
        pBackBuffer->Release();
        if (FAILED(hr))
            return hr;

        g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, nullptr);
    }*/


    /*const char* path = "./sample.mp4";

    const int BUFSIZE = 1024;
    DWORD  retval = 0;
    BOOL   success;
    TCHAR  buffer[BUFSIZE] = TEXT("");
    TCHAR  buf[BUFSIZE] = TEXT("");
    TCHAR** lppPart = { NULL };

    BSTR bpath = _com_util::ConvertStringToBSTR(path);
    retval = GetFullPathName(bpath, BUFSIZE, buffer, lppPart);
    */

    //CHECKLN_HRESULT(virtuosoPlayer.mediaEngine->SetSource(buffer));
    //IMFByteStream* byteStream = nullptr;
    //CHECKLN_HRESULT(MFCreateFile(MF_ACCESSMODE_READ, MF_OPENMODE_FAIL_IF_NOT_EXIST, MF_FILEFLAGS_NONE, buffer, &byteStream));
    //CHECKLN_HRESULT( virtuosoPlayer.mediaEngineEx->SetSourceFromByteStream(byteStream, buffer));

//create texture

    GLuint glTex = 0;
    HANDLE glHandle = 0;

    GLuint readFB=0;
    glGenFramebuffers(1, &readFB);
    assert(readFB);

#if 0
    {
        glGenTextures(1, &glTex);
        assert(glTex);
        glHandle = wglDXRegisterObjectNV(gl_handleD3D, tex, glTex, GL_TEXTURE_2D, WGL_ACCESS_READ_ONLY_NV);
        assert(glHandle);
    }

#endif
    glBindFramebuffer(GL_FRAMEBUFFER, readFB);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, glTex,0);

    GLenum err = GL_FRAMEBUFFER_COMPLETE;

    // Main message loop
    MSG msg = {0};
    while( WM_QUIT != msg.message )
    {
        if( PeekMessage( &msg, nullptr, 0, 0, PM_REMOVE ) )
        {
            TranslateMessage( &msg );
            DispatchMessage( &msg );
        }
        else
        {
            ///////////////////////////////////////////////////////

            LONGLONG pts;

#if 0
            if (virtuosoPlayer.mediaEngine->OnVideoStreamTick(&pts) == S_OK)
            {
                const MFARGB border = { 0, 0, 0, 255 };
                RECT dimensions = CRect(CPoint(0, 0), CSize(1920, 1080));

                MFVideoNormalizedRect srcr = { 0.f,0.f,1.f,1.f };

                CHECKLN_HRESULT(virtuosoPlayer.mediaEngine->TransferVideoFrame(tex, nullptr, &dimensions, &border));
            }
#endif
            //////////////////////////////////////////////////////////////////

            // lock the render targets for GL access
            // *******************************************************
         //   wglDXLockObjectsNV(gl_handleD3D, 1, &glHandle);

            //////////////////////////////////////////////////////////////////

            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            glViewport(0, 0, 800, 600);
            glClearColor(1.f, 0.f, 0.f, 1.f);
            glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

            const GLint targetFB = 0;

            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, targetFB);
            glBindFramebuffer(GL_READ_FRAMEBUFFER, readFB);

            assert((err = glCheckFramebufferStatus(GL_READ_FRAMEBUFFER)) == GL_FRAMEBUFFER_COMPLETE);


            glBlitFramebuffer(//readFB, targetFB,
                0, 0, 800, 600,
                0, 600, 800, 0,
                GL_COLOR_BUFFER_BIT,
                GL_LINEAR
            );


            ////////////////
            //********************************************************
            //wglDXUnlockObjectsNV(gl_handleD3D, 1, &glHandle);

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            SwapBuffers(g_hDCGL);

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

    g_hWnd = CreateWindow( L"TutorialWindowClass", L"Direct3D 11 Tutorial 1: Direct3D 11 Basics",
                           WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                           CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInstance,
                           nullptr );

    hWndGL = CreateWindow(L"TutorialWindowClass", L"GL Window",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInstance,
        nullptr);


    if( !g_hWnd || !hWndGL)
        return E_FAIL;

    ShowWindow( g_hWnd, nCmdShow );
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


