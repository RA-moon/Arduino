#include "animation_manager.h"
#include "animtated_circles.h"
#include "animtated_lines.h"
#include <Arduino.h>

using FrameFunction = std::vector<std::vector<int>>(*)();

static unsigned long lastSwitchTime = 0;
static int currentAnimation = 0;
static std::vector<std::vector<int>> activeFrames;

static FrameFunction animations[] = {
    getAnimationFrames,
    getAnimationFramesLines
};

const std::vector<std::vector<int>>& getCurrentAnimationFrames() {
    return activeFrames;
}

void updateAnimationSwitch() {
    unsigned long now = millis();
    if (now - lastSwitchTime >= 10000 || activeFrames.empty()) {
        lastSwitchTime = now;
        currentAnimation = (currentAnimation + 1) % (sizeof(animations) / sizeof(animations[0]));
        activeFrames = animations[currentAnimation]();
    }
}
