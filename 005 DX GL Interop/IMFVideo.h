#pragma once

#include <string_view>
#include <versionhelpers.h>
#include <iostream>
#include <fstream>
#include <comutil.h>

#include <mfmediaengine.h>
#include <mfapi.h>
#include <cassert>
#include <numeric>
#include <stdexcept>

#include <string_view>

#include <d3d11_1.h>
#include <dxgiformat.h>

//DXGI_FORMAT_R8G8B8A8_UNORM
#define TEXFMT   DXGI_FORMAT_B8G8R8A8_UNORM//DXGI_FORMAT_B8G8R8A8_UNORM//DXGI_FORMAT_R8G8B8A8_UINT


/* -- list of media foundation libs --
dxva2.lib
evr.lib
mf.lib
mfplat.lib
mfplay.lib
mfreadwrite.lib
mfuuid.lib
*/

#pragma comment( lib, "mfplat" )

#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "comsuppw.lib")
#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "glew32s.lib")

#define CHECKLN_HRESULT( CODE_IN ) Virtuoso::Win32::CHECK_HRESULT( CODE_IN, #CODE_IN );

IMFDXGIDeviceManager* DXGIManager;

namespace Virtuoso
{
    namespace Win32
    {
        //typedef IDirect3DTexture9* DXTexture;

        union RenderTarget
        {
            //DXTexture texture;
            HWND hwnd;

            RenderTarget() : hwnd(0) {}

            RenderTarget(const HWND& hwndIn) : hwnd(hwndIn)
            {
            }

           // RenderTarget(const DXTexture& texIn) : texture(texIn)
            //{
           // }

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


        void videoInit(IMFMediaEngine** mediaEngine,
            IMFMediaEngineEx** mediaEngineEx,
            IMFMediaEngineNotify* callbacks,
            Virtuoso::Win32::RenderTarget target,
            bool isHWND)
        {
            CHECK_HRESULT(MFStartup(MF_VERSION), "MFStartup error : Media foundation initialization failed");
            CHECK_HRESULT(CoInitializeEx(NULL, COINIT_MULTITHREADED), "CoInitializeEx() error : COM failed to initialize");

            IMFMediaEngineClassFactory* pMediaEngineClassFactory = nullptr;

            CHECKLN_HRESULT(CoCreateInstance(CLSID_MFMediaEngineClassFactory, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&pMediaEngineClassFactory)));

            //https://docs.microsoft.com/en-us/windows/win32/api/mfmediaengine/ne-mfmediaengine-mf_media_engine_createflags
            DWORD flags = 0;// MF_MEDIA_ENGINE_WAITFORSTABLE_STATE; // was 0

            //flags |= MF_MEDIA_ENGINE_REAL_TIME_MODE;

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
                CHECKLN_HRESULT(attributes->SetUINT32(MF_MEDIA_ENGINE_VIDEO_OUTPUT_FORMAT, TEXFMT));
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

        struct IMFMediaPlayer : public CallbackInterface
        {
            RenderTarget target;
            IMFMediaEngine* mediaEngine = nullptr;
            IMFMediaEngineEx* mediaEngineEx = nullptr;

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

                switch (event)
                {
                case MF_MEDIA_ENGINE_EVENT_LOADSTART:
                    break;
                case MF_MEDIA_ENGINE_EVENT_PROGRESS:
                    break;
                case MF_MEDIA_ENGINE_EVENT_SUSPEND:
                    break;
                case MF_MEDIA_ENGINE_EVENT_ABORT:
                    break;
                case MF_MEDIA_ENGINE_EVENT_ERROR:
                {
                    IMFMediaError* pError = nullptr;
                    mediaEngine->GetError(&pError);

                    if (pError)
                    {
                        switch (pError->GetErrorCode())
                        {
                        case MF_MEDIA_ENGINE_ERR_NOERROR:
                            std::cerr << "ddd" << std::endl;
                            break;
                        case MF_MEDIA_ENGINE_ERR_ABORTED:
                            std::cerr << "ddd" << std::endl;
                            break;
                        case MF_MEDIA_ENGINE_ERR_NETWORK:
                            std::cerr << "ddd" << std::endl;
                            break;
                        case MF_MEDIA_ENGINE_ERR_DECODE:
                            std::cerr << "ddd" << std::endl;
                            break;
                        case MF_MEDIA_ENGINE_ERR_SRC_NOT_SUPPORTED:
                            std::cerr << "ddd" << std::endl;
                            break;
                        case MF_MEDIA_ENGINE_ERR_ENCRYPTED:
                            std::cerr << "ddd" << std::endl;
                            break;
                        default:
                            break;
                        }
                    }

                    break;
                }
                case MF_MEDIA_ENGINE_EVENT_EMPTIED:
                    break;
                case MF_MEDIA_ENGINE_EVENT_STALLED:
                    break;
                case MF_MEDIA_ENGINE_EVENT_PLAY:
                    std::clog << "playing " << std::endl;
                    break;
                case MF_MEDIA_ENGINE_EVENT_PAUSE:
                    break;
                case MF_MEDIA_ENGINE_EVENT_LOADEDMETADATA:
                    break;
                case MF_MEDIA_ENGINE_EVENT_LOADEDDATA:
                    break;
                case MF_MEDIA_ENGINE_EVENT_WAITING:
                    break;
                case MF_MEDIA_ENGINE_EVENT_PLAYING:
                    break;
                case MF_MEDIA_ENGINE_EVENT_CANPLAY:
                    mediaEngine->Play();
                    break;
                case MF_MEDIA_ENGINE_EVENT_CANPLAYTHROUGH:
                    break;
                case MF_MEDIA_ENGINE_EVENT_SEEKING:
                    break;
                case MF_MEDIA_ENGINE_EVENT_SEEKED:
                    break;
                case MF_MEDIA_ENGINE_EVENT_TIMEUPDATE:
                {
                    volatile int a = 2;
                    break;
                }
                case MF_MEDIA_ENGINE_EVENT_ENDED:
                    break;
                case MF_MEDIA_ENGINE_EVENT_RATECHANGE:
                    break;
                case MF_MEDIA_ENGINE_EVENT_DURATIONCHANGE:
                    break;
                case MF_MEDIA_ENGINE_EVENT_VOLUMECHANGE:
                    break;
                case MF_MEDIA_ENGINE_EVENT_FORMATCHANGE:
                    break;
                case MF_MEDIA_ENGINE_EVENT_PURGEQUEUEDEVENTS:
                    break;
                case MF_MEDIA_ENGINE_EVENT_TIMELINE_MARKER:
                    break;
                case MF_MEDIA_ENGINE_EVENT_BALANCECHANGE:
                    break;
                case MF_MEDIA_ENGINE_EVENT_DOWNLOADCOMPLETE:
                    break;
                case MF_MEDIA_ENGINE_EVENT_BUFFERINGSTARTED:
                    break;
                case MF_MEDIA_ENGINE_EVENT_BUFFERINGENDED:
                    break;
                case MF_MEDIA_ENGINE_EVENT_FRAMESTEPCOMPLETED:
                    break;
                case MF_MEDIA_ENGINE_EVENT_NOTIFYSTABLESTATE:
                    break;
                case  MF_MEDIA_ENGINE_EVENT_FIRSTFRAMEREADY:
                    break;
                case  MF_MEDIA_ENGINE_EVENT_TRACKSCHANGE:
                    break;
                case MF_MEDIA_ENGINE_EVENT_OPMINFO:
                    break;
                case MF_MEDIA_ENGINE_EVENT_RESOURCELOST:
                    break;
                case MF_MEDIA_ENGINE_EVENT_DELAYLOADEVENT_CHANGED:
                    break;
                case MF_MEDIA_ENGINE_EVENT_STREAMRENDERINGERROR:
                    break;
                case MF_MEDIA_ENGINE_EVENT_SUPPORTEDRATES_CHANGED:
                    break;
                case MF_MEDIA_ENGINE_EVENT_AUDIOENDPOINTCHANGE:
                    break;
                default:
                    break;
                }

                return S_OK;
            }


            inline bool exists_test0(const std::string& name) {
                std::ifstream f(name.c_str());
                return f.good();
            }

            void playMedia()
            {
                const char* path = "./fireworks_full.mp4";

                const int BUFSIZE = 1024;
                DWORD  retval = 0;
                BOOL   success;
                TCHAR  buffer[BUFSIZE] = TEXT("");
                TCHAR  buf[BUFSIZE] = TEXT("");
                TCHAR** lppPart = { NULL };


                BSTR bpath = _com_util::ConvertStringToBSTR(path);

                retval = GetFullPathName(bpath,
                    BUFSIZE,
                    buffer,
                    lppPart);


                assert(mediaEngine != nullptr);
                //Zassert(target.hwnd != 0 || target.texture != nullptr);

                CHECKLN_HRESULT(mediaEngine->SetSource(buffer));

                bool doesex = exists_test0(path);

                CHECKLN_HRESULT(mediaEngine->Play());

            }


            IMFMediaPlayer()
            {
                // create DX texture

                if (!IsWindows8OrGreater())
                {
                    std::cerr << "Windows version is less than windows 8 : IMF Media Interface Not Supported!" << std::endl;
                }

                videoInit(&mediaEngine,&mediaEngineEx, this, target, false);
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







