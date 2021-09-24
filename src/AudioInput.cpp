#include "AudioInput.h"

#include <cinttypes>
#include <iostream>
#include <stdlib.h>
#include <vector>

//AudioInput::AudioInput(GlobalState* state, std::shared_ptr<ThreadSync> ts)
// : global(state)
// , sync(ts)
// , samples(new CircularBuffer<Sample>(524288))
AudioInput::AudioInput(volatile bool isStopped, std::shared_ptr<ThreadSync> ts)
    : isStopped(isStopped)
    , sync(ts)
    , samples(new CircularBuffer<Sample>(524288))
{
}

void AudioInput::start_thread()
{
    thread = std::thread([this] { input_audio(); });
}

void AudioInput::join_thread() { thread.join(); }
