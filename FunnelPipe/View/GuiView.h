#pragma once
#include <ScreenState.h>
#include <hierarchy.h>
#include <FrameData.h>

namespace gui
{

bool View(hierarchy::DrawList *view, const screenstate::ScreenState &state, size_t textureID,
          screenstate::ScreenState *viewState);

} // namespace gui
