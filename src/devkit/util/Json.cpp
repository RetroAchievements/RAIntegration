#include "Json.hh"

#include "services/impl/FileTextReader.hh"
#include "services/impl/FileTextWriter.hh"
#include "services/impl/StringTextReader.hh"
#include "services/impl/StringTextWriter.hh"

#pragma warning(push, 0) // disable all warnings for these headers
#pragma warning(disable: ALL_CODE_ANALYSIS_WARNINGS)
#pragma warning(disable: 6313)
#pragma warning(disable: 26429)
#pragma warning(disable: 26432)
#pragma warning(disable: 26440)
#pragma warning(disable: 26446)
#pragma warning(disable: 26447)
#pragma warning(disable: 26451)
#pragma warning(disable: 26455)
#pragma warning(disable: 26457)
#pragma warning(disable: 26460)
#pragma warning(disable: 26461)
#pragma warning(disable: 26462)
#pragma warning(disable: 26471)
#pragma warning(disable: 26472)
#pragma warning(disable: 26473)
#pragma warning(disable: 26475)
#pragma warning(disable: 26477)
#pragma warning(disable: 26490)
#pragma warning(disable: 26492)
#pragma warning(disable: 26493)
#pragma warning(disable: 26494)
#pragma warning(disable: 26495)
#pragma warning(disable: 26496)
#pragma warning(disable: 26497)
#pragma warning(disable: 26814)
#pragma warning(disable: 33010)

#define RAPIDJSON_HAS_STDSTRING 1
#define RAPIDJSON_NOMEMBERITERATORCLASS 1
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/writer.h> // has stringbuffer.h

#pragma warning(pop)

namespace ra {
namespace util {

class Json::Impl : public rapidjson::Document
{
};

Json::Reader::Reader() noexcept
{
    // required because of forward declaration of Impl
}

Json::Reader::~Reader()
{
    // required because of forward declaration of Impl
}

bool Json::Reader::Parse(ra::services::TextReader& pReader)
{
    m_pDocument = std::make_unique<Json::Impl>();

    auto* pFileTextReader = dynamic_cast<ra::services::impl::FileTextReader*>(&pReader);
    if (pFileTextReader != nullptr)
    {
        auto& iFile = pFileTextReader->GetFStream();
        if (!iFile.is_open())
            return false;

        rapidjson::IStreamWrapper iStreamWrapper(iFile);
        m_pDocument->ParseStream(iStreamWrapper);
    }
    else
    {
        auto* pStringTextReader = dynamic_cast<ra::services::impl::StringTextReader*>(&pReader);
        if (pStringTextReader != nullptr)
        {
            m_pDocument->Parse(pStringTextReader->GetString());
        }
        else
        {
            assert(!"Unsupported TextReader");
            return false;
        }
    }

    return !m_pDocument->HasParseError();
}

bool Json::Reader::Parse(const std::string& sJson)
{
    m_pDocument = std::make_unique<Json::Impl>();
    m_pDocument->Parse(sJson);

    return !m_pDocument->HasParseError();
}

std::string Json::Reader::GetParseError() const
{
    return GetParseError_En(m_pDocument->GetParseError());
}

size_t Json::Reader::GetParseErrorOffset() const
{
    return m_pDocument->GetErrorOffset();
}

std::string Json::Reader::GetString(const std::string& sFieldName, const std::string& sDefaultValue) const
{
    std::string sValue;
    if (!TryGetString(sFieldName, sValue))
        return sDefaultValue;

    return sValue;
}

bool Json::Reader::TryGetString(const std::string& sFieldName, std::string& sValue) const
{
    const auto* pField = m_pDocument->FindMember(sFieldName);
    if (pField == m_pDocument->MemberEnd())
        return false;

    if (!pField->value.IsString())
        return false;

    sValue.assign(pField->value.GetString(), pField->value.GetStringLength());
    return true;
}

int Json::Reader::GetInteger(const std::string& sFieldName, int nDefaultValue) const
{
    int nValue;
    if (!TryGetInteger(sFieldName, nValue))
        return nDefaultValue;

    return nValue;
}

bool Json::Reader::TryGetInteger(const std::string& sFieldName, int& nValue) const
{
    const auto* pField = m_pDocument->FindMember(sFieldName);
    if (pField == m_pDocument->MemberEnd())
        return false;

    if (!pField->value.IsInt())
        return false;

    nValue = pField->value.GetInt();
    return true;
}

bool Json::Reader::GetBoolean(const std::string& sFieldName, bool bDefaultValue) const
{
    bool bValue;
    if (!TryGetBoolean(sFieldName, bValue))
        return bDefaultValue;

    return bValue;
}

bool Json::Reader::TryGetBoolean(const std::string& sFieldName, bool& bValue) const
{
    const auto* pField = m_pDocument->FindMember(sFieldName);
    if (pField == m_pDocument->MemberEnd())
        return false;

    if (!pField->value.IsBool())
        return false;

    bValue = pField->value.GetBool();
    return true;
}

bool Json::Reader::TryGetObject(const std::string& sFieldName, Node& pObject)
{
    const auto* pField = m_pDocument->FindMember(sFieldName);
    if (pField == m_pDocument->MemberEnd())
        return false;

    if (!pField->value.IsObject())
        return false;

    pObject.m_pNode = &pField->value;
    return true;
}

bool Json::Reader::TryGetObjectArray(const std::string& sFieldName, std::vector<Node>& vObjects)
{
    const auto* pField = m_pDocument->FindMember(sFieldName);
    if (pField == m_pDocument->MemberEnd())
        return false;

    if (!pField->value.IsArray())
        return false;

    for (const auto& pObject : pField->value.GetArray())
    {
        auto& pNode = vObjects.emplace_back();
        pNode.m_pNode = &pObject;
    }

    return true;
}

size_t Json::Reader::GetKeys(std::vector<std::string>& vKeys) const
{
    for (rapidjson::Value::ConstMemberIterator iter = m_pDocument->MemberBegin(); iter != m_pDocument->MemberEnd(); ++iter)
        vKeys.push_back(iter->name.GetString());

    return vKeys.size();
}

std::string Json::Reader::Node::GetString(const std::string& sFieldName, const std::string& sDefaultValue) const
{
    std::string sValue;
    if (!TryGetString(sFieldName, sValue))
        return sDefaultValue;

    return sValue;
}

bool Json::Reader::Node::TryGetString(const std::string& sFieldName, std::string& sValue) const
{
    const auto* pNode = static_cast<const rapidjson::Value*>(m_pNode);
    Expects(pNode != nullptr);

    const auto* pField = pNode->FindMember(sFieldName);
    if (pField == pNode->MemberEnd())
        return false;

    if (!pField->value.IsString())
        return false;

    sValue.assign(pField->value.GetString(), pField->value.GetStringLength());
    return true;
}

int Json::Reader::Node::GetInteger(const std::string& sFieldName, int nDefaultValue) const
{
    int nValue;
    if (!TryGetInteger(sFieldName, nValue))
        return nDefaultValue;

    return nValue;
}

bool Json::Reader::Node::TryGetInteger(const std::string& sFieldName, int& nValue) const
{
    const auto* pNode = static_cast<const rapidjson::Value*>(m_pNode);
    Expects(pNode != nullptr);

    const auto* pField = pNode->FindMember(sFieldName);
    if (pField == pNode->MemberEnd())
        return false;

    if (!pField->value.IsInt())
        return false;

    nValue = pField->value.GetInt();
    return true;
}

bool Json::Reader::Node::GetBoolean(const std::string& sFieldName, bool bDefaultValue) const
{
    bool bValue;
    if (!TryGetBoolean(sFieldName, bValue))
        return bDefaultValue;

    return bValue;
}

bool Json::Reader::Node::TryGetBoolean(const std::string& sFieldName, bool& bValue) const
{
    const auto* pNode = static_cast<const rapidjson::Value*>(m_pNode);
    Expects(pNode != nullptr);

    const auto* pField = pNode->FindMember(sFieldName);
    if (pField == pNode->MemberEnd())
        return false;

    if (!pField->value.IsBool())
        return false;

    bValue = pField->value.GetBool();
    return true;
}

bool Json::Reader::Node::TryGetObject(const std::string& sFieldName, Node& pObject)
{
    const auto* pNode = static_cast<const rapidjson::Value*>(m_pNode);
    Expects(pNode != nullptr);

    const auto* pField = pNode->FindMember(sFieldName);
    if (pField == pNode->MemberEnd())
        return false;

    if (!pField->value.IsObject())
        return false;

    pObject.m_pNode = &pField->value;
    return true;
}

size_t Json::Reader::Node::GetKeys(std::vector<std::string>& vKeys) const
{
    const auto* pNode = static_cast<const rapidjson::Value*>(m_pNode);
    Expects(pNode != nullptr);

    for (rapidjson::Value::ConstMemberIterator iter = pNode->MemberBegin(); iter != pNode->MemberEnd(); ++iter)
        vKeys.push_back(iter->name.GetString());

    return vKeys.size();
}

GSL_SUPPRESS_F6
Json::Writer::Writer() noexcept
{
    // required because of forward declaration of Impl
    m_pDocument = std::make_unique<Json::Impl>();
    m_pDocument->SetObject();
}

Json::Writer::~Writer()
{
    // required because of forward declaration of Impl
}

bool Json::Writer::Save(ra::services::TextWriter& pWriter)
{
    auto* pFileTextWriter = dynamic_cast<ra::services::impl::FileTextWriter*>(&pWriter);
    if (pFileTextWriter != nullptr)
    {
        auto& oFile = pFileTextWriter->GetFStream();
        if (!oFile.is_open())
            return false;

        rapidjson::OStreamWrapper oStreamWrapper(oFile);
        rapidjson::Writer<rapidjson::OStreamWrapper> oStreamWriter(oStreamWrapper);
        return m_pDocument->Accept(oStreamWriter);
    }

    auto* pStringTextWriter = dynamic_cast<ra::services::impl::StringTextWriter*>(&pWriter);
    if (pStringTextWriter != nullptr)
    {
        rapidjson::StringBuffer oStringBuffer;
        rapidjson::Writer<rapidjson::StringBuffer> oStreamWriter(oStringBuffer);
        if (!m_pDocument->Accept(oStreamWriter))
            return false;

        pStringTextWriter->GetString().assign(oStringBuffer.GetString());
        return true;
    }

    assert(!"Unsupported TextWriter");
    return false;
}

void Json::Writer::SetString(const std::string& sFieldName, const std::string& sValue)
{
    auto& a = m_pDocument->GetAllocator();
    m_pDocument->AddMember(rapidjson::Value(sFieldName, a), rapidjson::Value(sValue, a), a);
}

void Json::Writer::SetInteger(const std::string& sFieldName, int nValue)
{
    auto& a = m_pDocument->GetAllocator();
    rapidjson::Value pKey(sFieldName, a);
    m_pDocument->AddMember(pKey, nValue, a);
}

void Json::Writer::SetBoolean(const std::string& sFieldName, bool bValue)
{
    auto& a = m_pDocument->GetAllocator();
    rapidjson::Value pKey(sFieldName, a);
    m_pDocument->AddMember(pKey, bValue, a);
}

Json::Writer::Node Json::Writer::SetObject(const std::string& sFieldName)
{
    rapidjson::Value pObject(rapidjson::kObjectType);

    auto& a = m_pDocument->GetAllocator();
    rapidjson::Value pKey(sFieldName, a);
    m_pDocument->AddMember(pKey, pObject.Move(), a);

    Node node;
    node.m_pOwner = this;
    node.m_pNode = &m_pDocument->FindMember(sFieldName)->value;
    return node;
}

Json::Writer::Node Json::Writer::SetObjectArray(const std::string& sFieldName)
{
    rapidjson::Value pObject(rapidjson::kArrayType);

    auto& a = m_pDocument->GetAllocator();
    rapidjson::Value pKey(sFieldName, a);
    m_pDocument->AddMember(pKey, pObject.Move(), a);

    Node node;
    node.m_pOwner = this;
    node.m_pNode = &m_pDocument->FindMember(sFieldName)->value;
    return node;
}

void Json::Writer::Node::SetString(const std::string& sFieldName, const std::string& sValue)
{
    auto* pNode = static_cast<rapidjson::Value*>(m_pNode);
    Expects(pNode != nullptr);

    auto& a = m_pOwner->m_pDocument->GetAllocator();
    pNode->AddMember(rapidjson::Value(sFieldName, a), rapidjson::Value(sValue, a), a);
}

void Json::Writer::Node::SetInteger(const std::string& sFieldName, int nValue)
{
    auto* pNode = static_cast<rapidjson::Value*>(m_pNode);
    Expects(pNode != nullptr);

    auto& a = m_pOwner->m_pDocument->GetAllocator();
    rapidjson::Value pKey(sFieldName, a);
    pNode->AddMember(pKey, nValue, a);
}

void Json::Writer::Node::SetBoolean(const std::string& sFieldName, bool bValue)
{
    auto* pNode = static_cast<rapidjson::Value*>(m_pNode);
    Expects(pNode != nullptr);

    auto& a = m_pOwner->m_pDocument->GetAllocator();
    rapidjson::Value pKey(sFieldName, a);
    pNode->AddMember(pKey, bValue, a);
}

Json::Writer::Node Json::Writer::Node::SetObject(const std::string& sFieldName)
{
    auto* pNode = static_cast<rapidjson::Value*>(m_pNode);
    Expects(pNode != nullptr);

    rapidjson::Value pObject(rapidjson::kObjectType);

    auto& a = m_pOwner->m_pDocument->GetAllocator();
    rapidjson::Value pKey(sFieldName, a);
    pNode->AddMember(pKey, pObject.Move(), a);

    Node node;
    node.m_pOwner = m_pOwner;
    node.m_pNode = &pNode->FindMember(sFieldName)->value;
    return node;
}

Json::Writer::Node Json::Writer::Node::SetObjectArray(const std::string& sFieldName)
{
    auto* pNode = static_cast<rapidjson::Value*>(m_pNode);
    Expects(pNode != nullptr);

    rapidjson::Value pObject(rapidjson::kArrayType);

    auto& a = m_pOwner->m_pDocument->GetAllocator();
    rapidjson::Value pKey(sFieldName, a);
    pNode->AddMember(pKey, pObject.Move(), a);

    Node node;
    node.m_pOwner = m_pOwner;
    node.m_pNode = &pNode->FindMember(sFieldName)->value;
    return node;
}

Json::Writer::Node Json::Writer::Node::AppendObject()
{
    auto* pNode = static_cast<rapidjson::Value*>(m_pNode);
    Expects(pNode != nullptr);

    rapidjson::Value pObject(rapidjson::kObjectType);
    pNode->PushBack(pObject, m_pOwner->m_pDocument->GetAllocator());

    Node node;
    node.m_pOwner = m_pOwner;
    node.m_pNode = &(*pNode)[pNode->Size() - 1];
    return node;
}

} // namespace util
} // namespace ra
