#ifndef PANITENT_BINVIEW_AUDIOSOURCE_H
#define PANITENT_BINVIEW_AUDIOSOURCE_H

#include <Audioclient.h>

typedef struct AudioSource AudioSource;

struct AudioSource {
    BOOL initialized;
    WAVEFORMATEXTENSIBLE format;
    unsigned int pcmPos;
    UINT32 bufferSize;
    UINT32 bufferPos;
    unsigned int sampleCount;
    float frequency;
    float* pcmAudio;
};

void AudioSource_Init(AudioSource* pAudioSource);
HRESULT AudioSource_SetFormat(AudioSource* pAudioSource, WAVEFORMATEX* wfex);
HRESULT AudioSource_LoadData(AudioSource* pAudioSource, UINT32 totalFrames, BYTE* dataOut, DWORD* flags);

#endif  /* PANITENT_BINVIEW_AUDIOSOURCE_H */
