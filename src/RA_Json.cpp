#include "RA_Json.h"

#include "services\impl\FileTextReader.hh"
#include "services\impl\FileTextWriter.hh"
#include "services\impl\StringTextReader.hh"
#include "services\impl\StringTextWriter.hh"

void _WriteBufferToFile(const std::wstring& sFileName, const rapidjson::Document& doc)
{
    std::ofstream ofile{ sFileName };
    if (!ofile.is_open())
        return;

    rapidjson::OStreamWrapper osw{ ofile };
    rapidjson::Writer<rapidjson::OStreamWrapper> writer{ osw };
    doc.Accept(writer);
}

_Use_decl_annotations_
bool LoadDocument(rapidjson::Document& doc, ra::services::TextReader& reader)
{
    auto* pFileTextReader = dynamic_cast<ra::services::impl::FileTextReader*>(&reader);
    if (pFileTextReader != nullptr)
    {
        auto& iFile = pFileTextReader->GetFStream();
        if (!iFile.is_open())
            return false;

        rapidjson::IStreamWrapper iStreamWrapper(iFile);
        doc.ParseStream(iStreamWrapper);
    }
    else
    {
        auto* pStringTextReader = dynamic_cast<ra::services::impl::StringTextReader*>(&reader);
        if (pStringTextReader != nullptr)
        {
            doc.Parse(pStringTextReader->GetString());
        }
        else
        {
            assert(!"Unsupported TextReader");
            return false;
        }
    }

    return !doc.HasParseError();
}

bool SaveDocument(_In_ const rapidjson::Document& doc, ra::services::TextWriter& writer)
{
    auto* pFileTextWriter = dynamic_cast<ra::services::impl::FileTextWriter*>(&writer);
    if (pFileTextWriter != nullptr)
    {
        auto& oFile = pFileTextWriter->GetFStream();
        if (!oFile.is_open())
            return false;

        rapidjson::OStreamWrapper oStreamWrapper(oFile);
        rapidjson::Writer<rapidjson::OStreamWrapper> oStreamWriter(oStreamWrapper);
        return doc.Accept(oStreamWriter);
    }

    auto* pStringTextWriter = dynamic_cast<ra::services::impl::StringTextWriter*>(&writer);
    if (pStringTextWriter != nullptr)
    {
        rapidjson::StringBuffer oStringBuffer;
        rapidjson::Writer<rapidjson::StringBuffer> oStreamWriter(oStringBuffer);
        if (!doc.Accept(oStreamWriter))
            return false;

        pStringTextWriter->GetString().assign(oStringBuffer.GetString());
        return true;
    }

    assert(!"Unsupported TextWriter");
    return false;
}
