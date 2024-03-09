#pragma once

#include <AL/al.h>
#include <AL/alc.h>

#include <string>

void init_audio();
void destroy_audio();

void play_audio(ALenum format, ALvoid* data, ALsizei size, ALsizei samplerate, bool autofree = true);
void play_ogg(const std::string& filename);
void play_ogg_async(const std::string& filename);
