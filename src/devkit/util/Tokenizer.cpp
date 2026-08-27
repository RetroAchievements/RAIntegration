#include "Tokenizer.hh"

namespace ra {
namespace util {

bool Tokenizer::Match(const std::string_view sText) noexcept
{
    if (m_nPosition + sText.length() <= m_sString.length())
    {
        if (m_sString.compare(m_nPosition, sText.length(), sText) == 0)
        {
            m_nPosition += sText.length();
            return true;
        }
    }

    return false;
}

std::string Tokenizer::ReadQuotedString()
{
    std::string sString;
    if (PeekChar() != '"')
        return sString;

    ++m_nPosition;
    while (m_nPosition < m_sString.length())
    {
        const auto c = m_sString.at(m_nPosition++);
        if (c == '"')
            break;

        if (c != '\\')
        {
            sString.push_back(c);
        }
        else
        {
            if (m_nPosition == m_sString.length())
                break;

            const auto c2 = m_sString.at(m_nPosition++);
            switch (c2)
            {
                case 'n':
                    sString.push_back('\n');
                    break;
                case 'r':
                    sString.push_back('\r');
                    break;
                case 't':
                    sString.push_back('\t');
                    break;
                default:
                    sString.push_back(c2);
                    break;
            }
        }
    }

    return sString;
}

} // namespace util
} // namespace ra
