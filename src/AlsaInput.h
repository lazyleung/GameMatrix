#ifndef _alsainput
#define _alsainput

#include "AudioInput.h"

#include <alsa/asoundlib.h>

// ALSA sound input implementation.
class AlsaInput : public AudioInput {
public:
    AlsaInput(volatile bool isStopped, std::shared_ptr<ThreadSync> ts);
    ~AlsaInput();

private:
    void input_audio() override;

    snd_pcm_t* handle; // ALSA sound device handle
};

#endif