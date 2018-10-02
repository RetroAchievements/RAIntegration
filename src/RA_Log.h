#ifndef RA_LOG_H
#define RA_LOG_H
#pragma once

extern void RADebugLogNoFormat(const char* data);
extern void RADebugLog(const char* sFormat, ...);

inline constexpr auto& RA_LOG{ RADebugLog };

#endif // !RA_LOG_H
