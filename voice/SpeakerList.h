#pragma once

#include "../gui/gui.h"
#include "../game/RW/RenderWare.h"

#include "Stream.h"

class SpeakerList {
    SpeakerList() = delete;
    ~SpeakerList() = delete;
    SpeakerList(const SpeakerList&) = delete;
    SpeakerList(SpeakerList&&) = delete;
    SpeakerList& operator=(const SpeakerList&) = delete;
    SpeakerList& operator=(SpeakerList&&) = delete;

private:
    static constexpr float kBaseIconSize = 36.f;

public:
    static bool Init() noexcept;
    static void Free() noexcept;

    static void Show() noexcept;
    static bool IsShowed() noexcept;
    static void Hide() noexcept;

    static void Render();
    static void Draw(VECTOR* vec, float fDist);

public:
    static void OnSpeakerPlay(const Stream& stream, uint16_t speaker) noexcept;
    static void OnSpeakerStop(const Stream& stream, uint16_t speaker) noexcept;

private:
    static bool initStatus;
    static bool showStatus;

    static RwTexture* tSpeakerIcon;

    static std::array<std::unordered_map<Stream*, StreamInfo>, MAX_PLAYERS> playerStreams;
};
