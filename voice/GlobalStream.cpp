#include "GlobalStream.h"

#include "StreamInfo.h"

GlobalStream::GlobalStream(const uint32_t color, std::string name) noexcept
    : Stream(NULL, StreamType::GlobalStream, color, std::move(name)) {}
