#include "ShaderWatcher.h"
#include <vector>
#include <stdint.h>
#include <fstream>
#include "Shader.h"

namespace framedata
{
ShaderWatcher::ShaderWatcher(const std::string &name, bool isInclude)
    : m_name(name)
{
    if (!isInclude)
    {
        m_compiled = std::make_shared<Shader>(name);
    }
}

void ShaderWatcher::source(const std::string &source)
{
    if (source.empty())
    {
        return;
    }
    if (m_source == source)
    {
        return;
    }

    m_source = source;
    m_generation++;

    if (m_compiled)
    {
        m_compiled->Initialize(nullptr, source, m_generation);
    }
}

} // namespace framedata
