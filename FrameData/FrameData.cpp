#include "FrameData.h"

namespace framedata
{

std::pair<uint32_t, uint32_t> FrameData::PushCB(const ConstantBuffer *cb, const CBValue *value, int count)
{
    auto offset = (uint32_t)CB.size();
    if (cb)
    {
        CBRanges.push_back({offset, cb->End()});
        CB.resize(CB.size() + CBRanges.back().second);
        auto p = CB.data() + offset;
        for (int i = 0; i < count; ++i, ++value)
        {
            for (auto &var : cb->Variables)
            {
                if (var.Semantic == value->semantic)
                {
                    // copy value
                    memcpy(p + var.Offset, value->p, value->size);
                    break;
                }
            }
        }
    }
    else
    {
        // 空の場合
        CBRanges.push_back({offset, 256});
        CB.resize(CB.size() + CBRanges.back().second);
    }

    return CBRanges.back();
}

} // namespace framedata
