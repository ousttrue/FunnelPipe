#pragma once
#include "Scene.h"
#include <FrameData.h>

namespace hierarchy
{

class FrameDataBuilder
{
    class FrameDataBuilderImpl *m_impl = nullptr;

public:
    FrameDataBuilder(const Scene &scene);
    ~FrameDataBuilder();
    void UpdateFrameData(framedata::FrameData *framedata);
};

} // namespace hierarchy
