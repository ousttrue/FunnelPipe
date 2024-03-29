#pragma once
#include <ScreenState.h>
#include <hierarchy.h>
#include <FrameData.h>

namespace gui
{

bool View(framedata::FrameData *view, const screenstate::ScreenState &state,
          void *texture,
          screenstate::ScreenState *viewState);

} // namespace gui
