#include "../precomp.h"

#include "audioplayer.h"
#include "../util/assert.h"

// CLSID_MMDeviceEnumerator
DEFINE_GUID(CLSID_MMDeviceEnumerator, 0xBCDE0395, 0xE52F, 0x467C, 0x8E, 0x3D, 0xC4, 0x57, 0x92, 0x91, 0x69, 0x2E);

// IID_IMMDeviceEnumerator
DEFINE_GUID(IID_IMMDeviceEnumerator, 0xA95664D2, 0x9614, 0x4F35, 0xA7, 0x46, 0xDE, 0x8D, 0xB6, 0x36, 0x17, 0xE6);

// IID_IAudioRenderClient
DEFINE_GUID(IID_IAudioRenderClient, 0xF294ACFC, 0x3146, 0x4483, 0xA7, 0xBF, 0xAD, 0xDC, 0xA7, 0xC2, 0x60, 0xE2);

// KSDATAFORMAT_SUBTYPE_WAVEFORMATEX
DEFINE_GUID(KSDATAFORMAT_SUBTYPE_WAVEFORMATEX, 0x00000003, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71);

#define IUnknown_Release(_This)((_This)->lpVtbl->Release((_This)))

#define IMMDeviceEnumerator_GetDefaultAudioEndpoint(_This, dataFlow, role, endpoint)((_This)->lpVtbl->GetDefaultAudioEndpoint((_This), (dataFlow), (role), (endpoint)))

#define IAudioClient_Stop(_This)((_This)->lpVtbl->Stop((_This)))
#define IAudioClient_GetMixFormat(_This, ppDeviceFormat)((_This)->lpVtbl->GetMixFormat((_This), (ppDeviceFormat)))
#define IAudioClient_Initialize(_This, ShareMode, StreamFlags, hnsBufferDuration, hnsPeriodicity, pFormat, AudioSessionGuid)((_This)->lpVtbl->Initialize((_This), (ShareMode), (StreamFlags), (hnsBufferDuration), (hnsPeriodicity), (pFormat), (AudioSessionGuid)))
#define IAudioClient_GetService(_This, riid, ppv)((_This)->lpVtbl->GetService((_This), (riid), (ppv)))

#define REFTIMES_PER_SEC  10000000
#define REFTIMES_PER_MILLISEC  10000

void AudioPlayer_Init(AudioPlayer* pAudioPlayer)
{
    ASSERT(pAudioPlayer);

    HRESULT hr;

    hr = CoInitialize(NULL);
    if (FAILED(hr))
    {
        Panitent_RaiseException(L"Failed to initialize COM");
        return;
    }

    hr = CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, &IID_IMMDeviceEnumerator, &pAudioPlayer->pEnumerator);
    if (FAILED(hr))
    {
        Panitent_RaiseException(L"Failed to create MMDeviceEnumerator");
        return;
    }

    hr = IMMDeviceEnumerator_GetDefaultAudioEndpoint(pAudioPlayer->pEnumerator, eRender, eConsole, &pAudioPlayer->pDevice);
    if (FAILED(hr))
    {
        Panitent_RaiseException(L"Failed to activate audio client");
        return;
    }
}

void AudioPlayer_Cleanup(AudioPlayer* pAudioPlayer)
{
    ASSERT(pAudioPlayer);

    if (pAudioPlayer->pAudioClient)
    {
        IAudioClient_Stop(pAudioPlayer->pAudioClient);
    }

    if (pAudioPlayer->pEnumerator)
    {
        IUnknown_Release(pAudioPlayer->pEnumerator);
        pAudioPlayer->pEnumerator = NULL;
    }

    if (pAudioPlayer->pDevice)
    {
        IUnknown_Release(pAudioPlayer->pDevice);
        pAudioPlayer->pDevice = NULL;
    }

    if (pAudioPlayer->pAudioClient)
    {
        IUnknown_Release(pAudioPlayer->pAudioClient);
        pAudioPlayer->pAudioClient = NULL;
    }

    if (pAudioPlayer->pRenderClient)
    {
        IUnknown_Release(pAudioPlayer->pRenderClient);
        pAudioPlayer->pRenderClient = NULL;
    }

    CoTaskMemFree(pAudioPlayer->pwfx);
    CoUninitialize();
}

BOOL AudioPlayer_InitializeAudioClient(AudioPlayer* pAudioPlayer, WAVEFORMATEX* pwfx)
{
    ASSERT(pAudioPlayer);

    REFERENCE_TIME hnsRequestedDuration = REFTIMES_PER_SEC;
    HRESULT hr;
    hr = IAudioClient_GetMixFormat(pAudioPlayer->pAudioClient, &pAudioPlayer->pwfx);
    if (FAILED(hr))
    {
        return FALSE;
    }

    hr = IAudioClient_Initialize(pAudioPlayer->pAudioClient, AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_RATEADJUST, hnsRequestedDuration, 0, pAudioPlayer->pwfx, NULL);
    if (FAILED(hr))
    {
        return FALSE;
    }

    hr = IAudioClient_GetService(pAudioPlayer->pAudioClient, &IID_IAudioRenderClient, &pAudioPlayer->pRenderClient);
    if (FAILED(hr))
    {
        return FALSE;
    }

    return TRUE;
}

void AudioPlayer_FillBuffer(AudioPlayer* pAudioPlayer, UINT32 bufferFrameCount, BYTE* pData, PDWORD pdwFlags)
{
    ASSERT(pAudioPlayer);

    HRESULT hr = S_OK;
    /* Load the data into the buffer */
    if (FAILED(hr))
    {
        Panitent_RaiseException(L"Failed to load data into buffer");
    }
}

DWORD WINAPI PlaybackThreadProc(PVOID pParam)
{
    AudioPlayer* pAudioPlayer = (AudioPlayer*)pParam;

    /* Try to play the audio stream */
    __try {
        // AudioPlayer_PlayAudioStream(pAudioPlayer, pMySource);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        Panitent_RaiseException(L"Error message");
    }

    return 0;
}

void AudioPlayer_StartPlayback(AudioPlayer* pAudioPlayer)
{
    if (InterlockedCompareExchange(pAudioPlayer->isPlaying, 0, FALSE))
    {
        InterlockedExchange(pAudioPlayer->isPlaying, TRUE);
        
        pAudioPlayer->playbackThread = CreateThread(NULL, 0, PlaybackThreadProc, NULL, 0, NULL);

        /* Check if thread creation was successful */
        if (!pAudioPlayer->playbackThread)
        {
            Panitent_RaiseException(L"Failed to create playback thread");
            InterlockedExchange(pAudioPlayer->isPlaying, FALSE);
        }
        
    }
}

void AudioPlayer_StopPlayback(AudioPlayer* pAudioPlayer)
{
    if (InterlockedCompareExchange(pAudioPlayer->isPlaying, 0, TRUE))
    {
        InterlockedExchange(pAudioPlayer->isPlaying, FALSE);

        /* Signal the condition variable to stop playback */
        EnterCriticalSection(&pAudioPlayer->cs);
        WakeAllConditionVariable(&pAudioPlayer->playbackCV);
        LeaveCriticalSection(&pAudioPlayer->cs);

        /* Wait for the playback thread to complete */
        if (pAudioPlayer->playbackThread)
        {
            WaitForSingleObject(pAudioPlayer->playbackThread, INFINITE);
            CloseHandle(pAudioPlayer->playbackThread);
            pAudioPlayer->playbackThread = NULL;
        }
    }
}
