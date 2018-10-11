#ifndef RA_JSON_H
#define RA_JSON_H
#pragma once

#include "services\TextReader.hh"
#include "services\TextWriter.hh"

#ifndef PCH_H
#define RAPIDJSON_HAS_STDSTRING 1
#define RAPIDJSON_NOMEMBERITERATORCLASS 1

#include <rapidjson\document.h> // has reader.h
#include <rapidjson\writer.h> // has stringbuffer.h
#include <rapidjson\istreamwrapper.h>
#include <rapidjson\ostreamwrapper.h>
#include <rapidjson\error\en.h>  
#endif /* !PCH_H */

void _WriteBufferToFile(const std::wstring& sFileName, const rapidjson::Document& doc);

_Success_(return)
_NODISCARD bool LoadDocument(_Out_ rapidjson::Document& doc, _In_ ra::services::TextReader& reader);
bool SaveDocument(_In_ const rapidjson::Document& doc, ra::services::TextWriter& writer);

#endif // !RA_JSON_H
