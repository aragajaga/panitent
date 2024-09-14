#include "../precomp.h"

#include "audiosource.h"

#define _USE_MATH_DEFINES
#include <math.h>

void AudioSource_Init(AudioSource* pAudioSource)
{
    pAudioSource->initialized = FALSE;
    pAudioSource->pcmPos = 0;
    pAudioSource->bufferSize = 0;
    pAudioSource->bufferPos = 0;
    pAudioSource->sampleCount = 96000 * 5;
    pAudioSource->frequency = 440.0f;
    pAudioSource->pcmAudio = NULL;

    pAudioSource->pcmAudio = malloc(pAudioSource->sampleCount * sizeof(float));
    const float radsPerSec = 2 * M_PI * pAudioSource->frequency / (float)pAudioSource->format.Format.nSamplesPerSec;

    for (unsigned long i = 0; i < pAudioSource->sampleCount; i++)
    {
        float sampleValue = sinf(radsPerSec * (float)i);
        pAudioSource->pcmAudio[i] = sampleValue;
    }

    pAudioSource->initialized = TRUE;
}

HRESULT AudioSource_SetFormat(AudioSource* pAudioSource, WAVEFORMATEX* wfex)
{
    if (wfex->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
    {
        pAudioSource->format = *(WAVEFORMATEXTENSIBLE*)wfex;
    }
    else {
        pAudioSource->format.Format = *wfex;
        pAudioSource->format.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
        INIT_WAVEFORMATEX_GUID(&pAudioSource->format.SubFormat, wfex->wFormatTag);
        pAudioSource->format.Samples.wValidBitsPerSample = pAudioSource->format.Format.wBitsPerSample;
        pAudioSource->format.dwChannelMask = 0;
    }
}

HRESULT AudioSource_LoadData(AudioSource* pAudioSource, UINT32 totalFrames, BYTE* dataOut, DWORD* flags)
{
    float* fData = (float*)dataOut;
    UINT32 totalSamples = totalFrames * pAudioSource->format.Format.nChannels;
    if (!pAudioSource->initialized)
    {
        AudioSource_Init(pAudioSource);
        pAudioSource->bufferSize = totalFrames * pAudioSource->format.Format.nChannels;
    }

    if (pAudioSource->pcmPos < pAudioSource->sampleCount)
    {
        for (UINT32 i = 0; i < totalSamples; i += pAudioSource->format.Format.nChannels)
        {
            for (size_t chan = 0; chan < pAudioSource->format.Format.nChannels; chan++)
            {
                fData[i + chan] = (pAudioSource->pcmPos < pAudioSource->sampleCount) ? pAudioSource->pcmAudio[pAudioSource->pcmPos] : 0.0f;
            }
            pAudioSource->pcmPos++;
        }
        pAudioSource->bufferPos += totalSamples;
        pAudioSource->bufferPos %= pAudioSource->bufferSize;
    }
    else {
        *flags = AUDCLNT_BUFFERFLAGS_SILENT;
    }

    return S_OK;
}
