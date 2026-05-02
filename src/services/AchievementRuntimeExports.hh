#ifndef RA_SERVICES_ACHIEVEMENT_RUNTIME_EXPORTS_HH
#define RA_SERVICES_ACHIEVEMENT_RUNTIME_EXPORTS_HH
#pragma once

#include "data/models/AchievementModel.hh"

#include <rcheevos/include/rc_client.h>

void RaiseClientExternalMenuChanged() noexcept;
void SyncClientExternalRAIntegrationMenuItem(int nMenuItemId);
void SyncClientExternalHardcoreState();
bool IsExternalRcheevosClient() noexcept;
void ResetExternalRcheevosClient() noexcept;
void ResetEmulatorMemoryRegionsForRcheevosClient();

const ra::data::models::AchievementModel* CheckForPauseOnTrigger(const rc_client_achievement_t& pAchievement);

#endif // !RA_SERVICES_ACHIEVEMENT_RUNTIME_EXPORTS_HH
