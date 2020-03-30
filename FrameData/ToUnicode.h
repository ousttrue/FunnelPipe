#include <string>
#include <stdint.h>

std::wstring ToUnicode(std::string const &src, uint32_t CP);
std::wstring Utf8ToUnicode(const std::string &src);
std::wstring SJISToUnicode(const std::string &src);
