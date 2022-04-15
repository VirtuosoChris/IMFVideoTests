#pragma once

struct TestCallbackInterface : public IMFMediaEngineNotify
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
