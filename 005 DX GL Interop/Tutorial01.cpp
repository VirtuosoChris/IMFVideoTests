// take the minimal DX 11 tutorial 1 and modify it to mimic the behavior of the UWP demo
// and thus play a video in win32 / non UWP api

//--------------------------------------------------------------------------------------
// File: Tutorial01.cpp
//
// This application demonstrates creating a Direct3D 11 device
//
// http://msdn.microsoft.com/en-us/library/windows/apps/ff729718.aspx
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License (MIT).
//--------------------------------------------------------------------------------------


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


//IMFDXGIDeviceManager* DXGIManager;
#include "IMFVideo.h"

using namespace DirectX;

//--------------------------------------------------------------------------------------
// Global Variables
//--------------------------------------------------------------------------------------
HINSTANCE               g_hInst = nullptr;
HWND                    g_hWnd = nullptr;
HDC                     g_hDCGL = nullptr;


HWND                    hWndGL = nullptr;

D3D_DRIVER_TYPE         g_driverType = D3D_DRIVER_TYPE_NULL;
D3D_FEATURE_LEVEL       g_featureLevel = D3D_FEATURE_LEVEL_11_1;
ID3D11Device*           g_pd3dDevice = nullptr;
ID3D11Device1*          g_pd3dDevice1 = nullptr;
ID3D11DeviceContext*    g_pImmediateContext = nullptr;
ID3D11DeviceContext1*   g_pImmediateContext1 = nullptr;
IDXGISwapChain*         g_pSwapChain = nullptr;
IDXGISwapChain1*        g_pSwapChain1 = nullptr;
ID3D11RenderTargetView* g_pRenderTargetView = nullptr;


HANDLE gl_handleD3D;

//--------------------------------------------------------------------------------------
// Forward declarations
//--------------------------------------------------------------------------------------
HRESULT InitWindow( HINSTANCE hInstance, int nCmdShow );
HRESULT InitDevice();
void CleanupDevice();
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

    if( FAILED( InitDevice() ) )
    {
        CleanupDevice();
        return 0;
    }

    Virtuoso::Win32::IMFMediaPlayer virtuosoPlayer;

  

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

        auto hr = spDXGIFactory->CreateSwapChainForHwnd(g_pd3dDevice, g_hWnd, &sd, nullptr, nullptr, &g_pSwapChain1);
        g_pSwapChain = g_pSwapChain1;
        /////spDXGIFactory->CreateSwapChainForCoreWindow(ldevice, reinterpret_cast<IUnknown*>((Windows::UI::Core::CoreWindow^)m_window.Get()), &swapChainDesc, nullptr, &m_spDX11SwapChain)

    }

    DXGIManager->UnlockDevice(hDevice, 0);


    {// proof the new swap chain works
        ID3D11Texture2D* pBackBuffer = nullptr;
        auto hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pBackBuffer));
        if (FAILED(hr))
            return hr;

        hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_pRenderTargetView);
        pBackBuffer->Release();
        if (FAILED(hr))
            return hr;

        g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, nullptr);
    }


    const char* path = "./sample.mp4";

    const int BUFSIZE = 1024;
    DWORD  retval = 0;
    BOOL   success;
    TCHAR  buffer[BUFSIZE] = TEXT("");
    TCHAR  buf[BUFSIZE] = TEXT("");
    TCHAR** lppPart = { NULL };

    BSTR bpath = _com_util::ConvertStringToBSTR(path);
    retval = GetFullPathName(bpath, BUFSIZE, buffer, lppPart);


    //CHECKLN_HRESULT(virtuosoPlayer.mediaEngine->SetSource(buffer));
    IMFByteStream* byteStream = nullptr;
    CHECKLN_HRESULT(MFCreateFile(MF_ACCESSMODE_READ, MF_OPENMODE_FAIL_IF_NOT_EXIST, MF_FILEFLAGS_NONE, buffer, &byteStream));

    CHECKLN_HRESULT( virtuosoPlayer.mediaEngineEx->SetSourceFromByteStream(byteStream, buffer));

    ID3D11Texture2D* tex = nullptr;
    {
        CD3D11_TEXTURE2D_DESC desc(
            TEXFMT,
            800,
            600,
            1,
            1,
            D3D11_BIND_RENDER_TARGET | D3D11_BIND_FLAG::D3D11_BIND_SHADER_RESOURCE,
            D3D11_USAGE::D3D11_USAGE_DEFAULT,
            0,// D3D11_CPU_ACCESS_FLAG::D3D11_CPU_ACCESS_WRITE//--- | D3D11_CPU_ACCESS_READ
            1,
            0,
            0);

        //desc.MiscFlags |= D3D11_RESOURCE_MISC_SHARED;
        CHECKLN_HRESULT(g_pd3dDevice->CreateTexture2D(&desc, nullptr, &tex));
    }

    if (WGLEW_NV_DX_interop2)
    {
        gl_handleD3D = wglDXOpenDeviceNV(g_pd3dDevice);
        assert(gl_handleD3D);

    }
    else
    {
        assert(0 && "ERROR NO DX INTEROP EXTENSION");
    }


    GLuint glTex = 0;
    HANDLE glHandle = 0;

    GLuint readFB=0;
    glGenFramebuffers(1, &readFB);
    assert(readFB);

    {
        glGenTextures(1, &glTex);
        assert(glTex);
        glHandle = wglDXRegisterObjectNV(gl_handleD3D, tex, glTex, GL_TEXTURE_2D, WGL_ACCESS_READ_ONLY_NV);
        assert(glHandle);
    }


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

            if (virtuosoPlayer.mediaEngine->OnVideoStreamTick(&pts) == S_OK)
            {
                g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, Colors::MidnightBlue);

                const MFARGB border = { 0, 0, 0, 255 };
                RECT dimensions = CRect(CPoint(0, 0), CSize(1920, 1080));

                MFVideoNormalizedRect srcr = { 0.f,0.f,1.f,1.f };

                ID3D11Texture2D* spTextureDst;

                CHECKLN_HRESULT(g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&spTextureDst)));

                CHECKLN_HRESULT(virtuosoPlayer.mediaEngine->TransferVideoFrame(tex, nullptr, &dimensions, &border));

                // blit from the offscreen texture to the back buffer
                g_pImmediateContext->CopyResource(spTextureDst, tex);

                g_pSwapChain1->Present(1, 0);
            }

            //////////////////////////////////////////////////////////////////

            // lock the render targets for GL access
            wglDXLockObjectsNV(gl_handleD3D, 1, &glHandle);

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

            wglDXUnlockObjectsNV(gl_handleD3D, 1, &glHandle);

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            SwapBuffers(g_hDCGL);

        }
    }

    CleanupDevice();

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


//--------------------------------------------------------------------------------------
// Create Direct3D device and swap chain
//--------------------------------------------------------------------------------------
HRESULT InitDevice()
{
    HRESULT hr = S_OK;

    RECT rc;
    GetClientRect( g_hWnd, &rc );
    UINT width = rc.right - rc.left;
    UINT height = rc.bottom - rc.top;

    UINT createDeviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_VIDEO_SUPPORT;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_DRIVER_TYPE driverTypes[] =
    {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE,
    };
    UINT numDriverTypes = ARRAYSIZE( driverTypes );

    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
        D3D_FEATURE_LEVEL_9_3,
        D3D_FEATURE_LEVEL_9_2,
        D3D_FEATURE_LEVEL_9_1
    };

	UINT numFeatureLevels = ARRAYSIZE( featureLevels );

    hr = D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        createDeviceFlags,
        featureLevels, numFeatureLevels,
        D3D11_SDK_VERSION,
        &g_pd3dDevice,
        &g_featureLevel,
        &g_pImmediateContext
    );

    if (FAILED(hr))
    {
        throw Virtuoso::Win32::ComException(hr, "D3D11CreateDevice");
    }

    ID3D10Multithread* spMultithread;
    g_pd3dDevice->QueryInterface<ID3D10Multithread>(&spMultithread);
    spMultithread->SetMultithreadProtected(TRUE);

    UINT resetToken;
    MFCreateDXGIDeviceManager(&resetToken, &DXGIManager);
    (DXGIManager->ResetDevice(g_pd3dDevice, resetToken));

    return S_OK;
}



//--------------------------------------------------------------------------------------
// Clean up the objects we've created
//--------------------------------------------------------------------------------------
void CleanupDevice()
{
    if( g_pImmediateContext ) g_pImmediateContext->ClearState();

    if( g_pRenderTargetView ) g_pRenderTargetView->Release();
    if( g_pSwapChain1 ) g_pSwapChain1->Release();
    if( g_pSwapChain ) g_pSwapChain->Release();
    if( g_pImmediateContext1 ) g_pImmediateContext1->Release();
    if( g_pImmediateContext ) g_pImmediateContext->Release();
    if( g_pd3dDevice1 ) g_pd3dDevice1->Release();
    if( g_pd3dDevice ) g_pd3dDevice->Release();
}
