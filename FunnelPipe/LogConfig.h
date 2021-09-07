#pragma once
#include <functional>
#include <Plog/Converters/UTF8Converter.h>

///
/// logger setup
///
namespace plog
{
class MyFormatter
{
public:
    static util::nstring header()
    {
        return util::nstring();
    }

    static util::nstring format(const Record &r)
    {
        tm t;
        util::localtime_s(&t, &r.getTime().time);

        util::nostringstream ss;
        ss
            << std::setw(2) << t.tm_hour << ':' << std::setw(2) << t.tm_min << '.' << std::setw(2) << t.tm_sec
            << '[' << severityToString(r.getSeverity()) << ']'
            << r.getFunc() << '(' << r.getLine() << ") "
            << r.getMessage()

            << "\n"; // Produce a simple string with a log message.

        return ss.str();
    }
};

template <class Formatter>          // Typically a formatter is passed as a template parameter.
class MyAppender : public IAppender // All appenders MUST inherit IAppender interface.
{
    using OnWrite = std::function<void(const char *)>;
    OnWrite m_onWrite;

public:
    void write(const Record &record) override // This is a method from IAppender that MUST be implemented.
    {
        util::nstring str = Formatter::format(record); // Use the formatter to get a string from a record.
        if (m_onWrite)
        {
            auto utf8 = UTF8Converter::convert(str);
            m_onWrite(utf8.c_str());
        }
    }

    void onWrite(const OnWrite &callback)
    {
        m_onWrite = callback;
    }
};

} // namespace plog
