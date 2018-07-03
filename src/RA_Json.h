#ifndef RA_JSON_H
#define RA_JSON_H
#pragma once

//	RA-Only
#define RAPIDJSON_HAS_STDSTRING 1

// This is not needed the most recent version
#pragma warning(push, 1)
#define _SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING

//	RA-Only
#include "rapidjson/include/rapidjson/document.h"
#include "rapidjson/include/rapidjson/writer.h"
#include "rapidjson/include/rapidjson/filestream.h"
#include "rapidjson/include/rapidjson/error/en.h"

extern rapidjson::GetParseErrorFunc GetJSONParseErrorStr;

#pragma warning(pop)

#endif // !RA_JSON_H
