#pragma once

#include "include/util/Timer.h"

#include "Parameter.h"
#include "Channel.h"

class SlideController : public Parameter {
    SlideController() = delete;
    SlideController(const SlideController&) = delete;
    SlideController(SlideController&&) = delete;
    SlideController& operator=(const SlideController&) = delete;
    SlideController& operator=(SlideController&&) = delete;

public:
    explicit SlideController(uint32_t parameter, float startValue, float endValue, uint32_t time) noexcept;

    ~SlideController() noexcept = default;

public:
    void Apply(const Channel& channel) const noexcept override;

private:
    const float ratio;
    const Timer::time_t endTime;
    const float endValue;
};

using SlideControllerPtr = std::unique_ptr<SlideController>;
#define MakeSlideController std::make_unique<SlideController>
