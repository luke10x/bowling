#include "../sounds/audio_buffer_policy.h"

#include <cassert>
#include <limits>

int main()
{
    // At 44.1 kHz, 4096 frames need 12.92 FPS with the 20% margin.
    assert(Sound_ChooseStartupAudioBufferSize(13.0, 44100, 2048) == 4096);
    assert(Sound_ChooseStartupAudioBufferSize(12.0, 44100, 2048) == 8192);
    assert(Sound_ChooseStartupAudioBufferSize(25.0, 44100, 2048) == 4096);
    assert(Sound_ChooseStartupAudioBufferSize(26.0, 44100, 2048) == 2048);

    // Use the device's actual sample rate; 27 FPS is enough at 44.1 kHz,
    // but not at 48 kHz for the 2048-frame buffer.
    assert(Sound_ChooseStartupAudioBufferSize(27.0, 44100, 2048) == 2048);
    assert(Sound_ChooseStartupAudioBufferSize(27.0, 48000, 2048) == 4096);

    assert(Sound_ChooseStartupAudioBufferSize(60.0, 44100, 4096) == 4096);
    assert(Sound_ChooseStartupAudioBufferSize(2.0, 44100, 2048) == 8192);
    assert(Sound_ChooseStartupAudioBufferSize(0.0, 44100, 2048) == 2048);
    assert(Sound_ChooseStartupAudioBufferSize(
               std::numeric_limits<double>::quiet_NaN(), 44100, 2048) == 2048);
}
