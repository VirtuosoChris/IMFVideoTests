#pragma once

#include <string_view>
#include <vector>
#include <sstream>
#include <iostream>
#include <numeric>
#include <stdexcept>
#include <cassert>

#if _DEBUG
#include <fstream>
#endif

#include <versionhelpers.h>
#include <comutil.h>

#include <mfmediaengine.h>
#include <mfapi.h>

#include <d3d11_1.h>
#include <dxgiformat.h>

#pragma comment( lib, "mfplat" )
#pragma comment(lib, "comsuppw.lib")
#pragma comment(lib, "kernel32.lib")

#define CHECKLN_HRESULT( CODE_IN ) Virtuoso::Win32::CHECK_HRESULT( CODE_IN, #CODE_IN );
#define ERRORCASE( caseId ) case caseId: {throw std::runtime_error("IMFMediaError" #caseId); break;}

namespace Virtuoso
{
    namespace Win32
    {
        union RenderTarget
        {
            //DXTexture texture;
            HWND hwnd;

            RenderTarget() : hwnd(0) {}

            RenderTarget(const HWND& hwndIn) : hwnd(hwndIn)
            {
            }

            RenderTarget& operator=(const HWND& in)
            {
                this->hwnd = in;
                return *this;
            }
        };

        struct ComException : public std::runtime_error
        {
            HRESULT err;
            ComException(HRESULT errIn, const std::string_view& str) :
                err(errIn),
                std::runtime_error(str.data())
            {
            }
        };

        inline void CHECK_HRESULT(HRESULT err, const std::string_view& str)
        {
            if (err != 0)
            {
                throw ComException(err, str);
            }
        }

        struct DX11Context
        {
            const static DXGI_FORMAT texFormat = DXGI_FORMAT_B8G8R8A8_UNORM;

            ID3D11Device* pd3dDevice = nullptr;
            ID3D11DeviceContext* pImmediateContext = nullptr;
            IMFDXGIDeviceManager* DXGIManager = nullptr;
            HANDLE gl_handleD3D;

            D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;

            ID3D11Texture2D* createTexture(UINT width, UINT height)
            {
                ID3D11Texture2D* tex = nullptr;

                CD3D11_TEXTURE2D_DESC desc(
                    texFormat,
                    width,
                    height,
                    1,
                    1,
                    D3D11_BIND_RENDER_TARGET | D3D11_BIND_FLAG::D3D11_BIND_SHADER_RESOURCE,
                    D3D11_USAGE::D3D11_USAGE_DEFAULT,
                    0,
                    1,
                    0,
                    0);

                CHECKLN_HRESULT(pd3dDevice->CreateTexture2D(&desc, nullptr, &tex));
                return tex;
            }

            ~DX11Context()
            {
                if (gl_handleD3D)
                {
                    wglDXCloseDeviceNV(gl_handleD3D);
                }

                if (pImmediateContext)
                {
                    pImmediateContext->ClearState();
                    pImmediateContext->Release();
                }

                if (DXGIManager)
                {
                    DXGIManager->Release();
                }

                if (pd3dDevice)
                {
                    pd3dDevice->Release();
                }
            }

            DX11Context()
            {
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
                UINT numDriverTypes = ARRAYSIZE(driverTypes);

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

                UINT numFeatureLevels = ARRAYSIZE(featureLevels);

                CHECKLN_HRESULT(
                    D3D11CreateDevice(
                        nullptr,
                        D3D_DRIVER_TYPE_HARDWARE,
                        nullptr,
                        createDeviceFlags,
                        featureLevels,
                        numFeatureLevels,
                        D3D11_SDK_VERSION,
                        &pd3dDevice,
                        &featureLevel,
                        &pImmediateContext
                    ));

                ID3D10Multithread* spMultithread;
                pd3dDevice->QueryInterface<ID3D10Multithread>(&spMultithread);
                spMultithread->SetMultithreadProtected(TRUE);

                spMultithread->Release();

                UINT resetToken;
                MFCreateDXGIDeviceManager(&resetToken, &DXGIManager);
                (DXGIManager->ResetDevice(pd3dDevice, resetToken));

                if (WGLEW_NV_DX_interop2)
                {
                    gl_handleD3D = wglDXOpenDeviceNV(pd3dDevice);
                    assert(gl_handleD3D);
                }
                else
                {
                    assert(0 && "ERROR NO DX INTEROP EXTENSION");
                }
            }
        };

        void videoInit(IMFMediaEngine** mediaEngine,
            IMFMediaEngineEx** mediaEngineEx,
            IMFMediaEngineNotify* callbacks,
            Virtuoso::Win32::RenderTarget target,
            bool isHWND,
            IMFDXGIDeviceManager* DXGIManager=nullptr)
        {
            CHECK_HRESULT(MFStartup(MF_VERSION), "MFStartup error : Media foundation initialization failed");
            CHECK_HRESULT(CoInitializeEx(NULL, COINIT_MULTITHREADED), "CoInitializeEx() error : COM failed to initialize");

            IMFMediaEngineClassFactory* pMediaEngineClassFactory = nullptr;

            CHECKLN_HRESULT(CoCreateInstance(CLSID_MFMediaEngineClassFactory, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&pMediaEngineClassFactory)));

            //https://docs.microsoft.com/en-us/windows/win32/api/mfmediaengine/ne-mfmediaengine-mf_media_engine_createflags
            DWORD flags = 0;// MF_MEDIA_ENGINE_WAITFORSTABLE_STATE;

            IMFAttributes* attributes = NULL;

            //https://docs.microsoft.com/en-us/windows/win32/medfound/media-engine-attributes
            CHECK_HRESULT(MFCreateAttributes(&attributes, 1), "MFCreateAttributes() error: failed to create IMFAttributes for media engine");

            CHECKLN_HRESULT(attributes->SetUnknown(MF_MEDIA_ENGINE_CALLBACK, callbacks));

            if (isHWND) // Rendering Mode
            {
                /*In this mode, the Media Engine renders both audio and video.
                The video is rendered to a window or Microsoft DirectComposition visual provided by the application.
                To enable rendering mode, set either the
                MF_MEDIA_ENGINE_PLAYBACK_HWND attribute or the
                MF_MEDIA_ENGINE_PLAYBACK_VISUAL attribute.*/

                CHECKLN_HRESULT(attributes->SetUINT64(MF_MEDIA_ENGINE_PLAYBACK_HWND, (UINT64)target.hwnd));

            }
            else // Frame-server mode
            {
                /*In this mode, the Media Engine delivers uncompressed video frames to the application.
                The application is responsible for displaying each frame, using Microsoft Direct3D or any other rendering technique.
                The Media Engine renders the audio; the application is not responsible for audio rendering.
                Frame-server mode is the default mode.*/

                ///\todo make sure this format is ok...
                CHECKLN_HRESULT(attributes->SetUINT32(MF_MEDIA_ENGINE_VIDEO_OUTPUT_FORMAT, DX11Context::texFormat));
                CHECKLN_HRESULT(attributes->SetUnknown(MF_MEDIA_ENGINE_DXGI_MANAGER, (IUnknown*)DXGIManager));

            }

            // third mode: To enable audio mode, set the MF_MEDIA_ENGINE_AUDIOONLY flag in the dwFlags parameter.
            // not relevant to video player!

            CHECKLN_HRESULT(pMediaEngineClassFactory->CreateInstance(flags, attributes, mediaEngine));


            (*mediaEngine)->QueryInterface<IMFMediaEngineEx>(mediaEngineEx);

            /// cleanup stuff
            pMediaEngineClassFactory->Release();
            attributes->Release();
        }

        struct CallbackInterface : public IMFMediaEngineNotify
        {
            long m_cRef = 1;

            virtual HRESULT STDMETHODCALLTYPE EventNotify(
                /* [annotation][in] */
                _In_  DWORD event,
                /* [annotation][in] */
                _In_  DWORD_PTR param1,
                /* [annotation][in] */
                _In_  DWORD param2) override
            {
                //https://docs.microsoft.com/en-us/windows/win32/api/mfmediaengine/ne-mfmediaengine-mf_media_engine_event
                /*
                   MF_MEDIA_ENGINE_EVENT_FRAMESTEPCOMPLETED = 1007
                   MF_MEDIA_ENGINE_EVENT_FIRSTFRAMEREADY = 1009,
                */

                return S_OK;
            }

            STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override
            {
                if (__uuidof(IMFMediaEngineNotify) == riid)
                {
                    *ppv = static_cast<IMFMediaEngineNotify*>(this);
                }
                else
                {
                    *ppv = nullptr;
                    return E_NOINTERFACE;
                }

                AddRef();

                return S_OK;
            }

            STDMETHODIMP_(ULONG) AddRef() override
            {
                return InterlockedIncrement(&m_cRef);
            }

            STDMETHODIMP_(ULONG) Release() override
            {
                LONG cRef = InterlockedDecrement(&m_cRef);
                if (cRef == 0)
                {
                    delete this;
                }
                return cRef;
            }
        };

        struct IMFMediaPlayer : public CallbackInterface, public DX11Context
        {
            RenderTarget target;
            IMFMediaEngine* mediaEngine = nullptr;
            IMFMediaEngineEx* mediaEngineEx = nullptr;

            DWORD width;
            DWORD height;
            double durationSeconds;
            double currentTime;

            void queryProperties()
            {
                mediaEngine->GetNativeVideoSize(&width, &height);
                durationSeconds = mediaEngine->GetDuration();
            }

            virtual HRESULT STDMETHODCALLTYPE EventNotify(
                /* [annotation][in] */
                _In_  DWORD event,
                /* [annotation][in] */
                _In_  DWORD_PTR param1,
                /* [annotation][in] */
                _In_  DWORD param2) override
            {
                //https://docs.microsoft.com/en-us/windows/win32/api/mfmediaengine/ne-mfmediaengine-mf_media_engine_event

                switch (event)
                {
                    case MF_MEDIA_ENGINE_EVENT_ERROR:
                    {
                        IMFMediaError* pError = nullptr;
                        mediaEngine->GetError(&pError);

                        if (pError)
                        {
                            auto code = pError->GetErrorCode();
                            switch (code)
                            {
                                ERRORCASE(MF_MEDIA_ENGINE_ERR_NOERROR)
                                ERRORCASE(MF_MEDIA_ENGINE_ERR_ABORTED)
                                ERRORCASE(MF_MEDIA_ENGINE_ERR_NETWORK)
                                ERRORCASE(MF_MEDIA_ENGINE_ERR_DECODE)
                                ERRORCASE(MF_MEDIA_ENGINE_ERR_SRC_NOT_SUPPORTED)
                                ERRORCASE(MF_MEDIA_ENGINE_ERR_ENCRYPTED)
                                default:
                                {
                                    std::stringstream sstr;
                                    sstr << "IMFMediaError::GetErrorCode() returned unknown error with id " << code;
                                    throw std::runtime_error(sstr.str());
                                    break;
                                }
                            }
                        }

                        break;
                    }
                    case MF_MEDIA_ENGINE_EVENT_CANPLAY:
                    {
                        queryProperties();

                        mediaEngine->Play();
                        break;
                    }
                    case MF_MEDIA_ENGINE_EVENT_ENDED:
                    {
                        currentTime = durationSeconds;
                        break;
                    }
                    case MF_MEDIA_ENGINE_EVENT_SEEKING:
                    {
                        currentTime = mediaEngine->GetCurrentTime();
                        break;
                    }
                    case MF_MEDIA_ENGINE_EVENT_SEEKED:
                    {
                        currentTime = mediaEngine->GetCurrentTime();
                        break;
                    }
                    case MF_MEDIA_ENGINE_EVENT_TIMEUPDATE:
                    {                        
                        currentTime = mediaEngine->GetCurrentTime();
                        break;
                    }
                    default:
                    {
                        break;
                    }
                }

                return S_OK;
            }

#if _DEBUG
            inline bool file_exists_test(const std::string_view& name)
            {
                std::ifstream f(name.data());
                return f.good();
            }
#endif

            void playMedia(const std::string_view& path)
            {
                const int BUFSIZE = path.length()+1;
                DWORD  retval = 0;
                BOOL   success;

                std::vector<TCHAR> buffer(BUFSIZE);
                TCHAR** lppPart = { NULL };

                BSTR bpath = _com_util::ConvertStringToBSTR(path.data());

                retval = GetFullPathName(bpath, BUFSIZE, buffer.data(), lppPart);

                assert(mediaEngine != nullptr);

                CHECKLN_HRESULT(mediaEngine->SetSource(buffer.data()));

#if _DEBUG
                assert(file_exists_test(path));
#endif

                CHECKLN_HRESULT(mediaEngine->Play());
            }

            // wraps:
            // https://docs.microsoft.com/en-us/windows/win32/api/mfmediaengine/nn-mfmediaengine-imfmediaengine
            IMFMediaPlayer()
            {
                // create DX texture

                if (!IsWindows8OrGreater())
                {
                    std::cerr << "Windows version is less than windows 8 : IMF Media Interface Not Supported!" << std::endl;
                }

                videoInit(&mediaEngine,&mediaEngineEx, this, target, false, DXGIManager);
            }

            virtual ~IMFMediaPlayer()
            {
                if (mediaEngine)
                {
                    mediaEngine->Shutdown();
                }
                // release dx resources
            }
        };
    }
}

#undef CHECKLN_HRESULT( CODE_IN )
#undef ERRORCASE( caseId )
