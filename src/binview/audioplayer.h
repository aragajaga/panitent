#ifndef PANITENT_BINVIEW_AUDIOPLAYER_H
#define PANITENT_BINVIEW_AUDIOPLAYER_H

#include <Audioclient.h>
#include <mmdeviceapi.h>
#include "../crashhandler.h"

typedef struct AudioPlayer AudioPlayer;

struct AudioPlayer {
    IMMDeviceEnumerator* pEnumerator;
    IMMDevice* pDevice;
    IAudioClient* pAudioClient;
    IAudioRenderClient* pRenderClient;
    WAVEFORMATEX* pwfx;

    volatile LONG isPlaying;
    CONDITION_VARIABLE playbackCV;
    CRITICAL_SECTION cs;
    HANDLE playbackThread;
};

void AudioPlayer_Init(AudioPlayer* pAudioPlayer);
void AudioPlayer_Cleanup(AudioPlayer* pAudioPlayer);
BOOL AudioPlayer_InitializeAudioClient(AudioPlayer* pAudioPlayer, WAVEFORMATEX* pwfx);
void AudioPlayer_FillBuffer(AudioPlayer* pAudioPlayer, UINT32 bufferFrameCount, BYTE* pData, PDWORD pdwFlags);
void AudioPlayer_StartPlayback(AudioPlayer* pAudioPlayer);
void AudioPlayer_StopPlayback(AudioPlayer* pAudioPlayer);

#endif  /* PANITENT_BINVIEW_AUDIOPLAYER_H */
