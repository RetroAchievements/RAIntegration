#ifndef RA_JSON_H
#define RA_JSON_H
#pragma once

#define RAPIDJSON_HAS_STDSTRING 1
#define RAPIDJSON_NOMEMBERITERATORCLASS 1

#include <rapidjson/document.h> // has reader.h
#include <rapidjson/writer.h> // has stringbuffer.h
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/error/en.h>

extern void _WriteBufferToFile(const std::wstring& sFileName, const rapidjson::Document& doc);

#endif // !RA_JSON_H
