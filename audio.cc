#include "audio.h"

#include <chrono>
#include <iostream>
#include <thread>

#define STB_VORBIS_NO_PUSHDATA_API
#include <stb_vorbis.c>

using namespace std::chrono_literals;

namespace {
ALCdevice* device = nullptr;
ALCcontext* context = nullptr;
}  // namespace

void init_audio() {
    device = alcOpenDevice(nullptr);
    if (!device) throw std::runtime_error("cannot open audio device");

    context = alcCreateContext(device, nullptr);
    if (!context) throw std::runtime_error("cannot create audio context");

    alcMakeContextCurrent(context);
}

void destroy_audio() {
    alcMakeContextCurrent(nullptr);
    alcDestroyContext(context);
    alcCloseDevice(device);

    context = nullptr;
    device = nullptr;
}

void play_audio(ALenum format, ALvoid* data, ALsizei size, ALsizei samplerate, bool autofree) {
    ALuint buffer;
    alGenBuffers(1, &buffer);
    alBufferData(buffer, format, data, size, samplerate);
    if (autofree) free(data);

    ALuint source;
    alGenSources(1, &source);
    alSourceQueueBuffers(source, 1, &buffer);
    alSourcePlay(source);

    ALint state = AL_PLAYING;
    while (state == AL_PLAYING) {
        std::this_thread::sleep_for(10ms);
        alGetSourcei(source, AL_SOURCE_STATE, &state);
    }

    alDeleteSources(1, &source);
    alDeleteBuffers(1, &buffer);
}

void play_ogg(const std::string& filename) {
    int channels, sample_rate;
    short* output;
    int samples = stb_vorbis_decode_filename(filename.c_str(), &channels, &sample_rate, &output);
    play_audio(AL_FORMAT_STEREO16, output, samples * 2 * sizeof(short), sample_rate);
}

void play_ogg_async(const std::string& filename) {
    std::thread([filename]() { play_ogg(filename); }).detach();
}
