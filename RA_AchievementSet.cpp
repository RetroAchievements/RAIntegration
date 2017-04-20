#pragma once

#include "RA_AchievementSet.h"
#include "RA_Core.h"
#include "RA_Dlg_Achievement.h"
#include "RA_User.h"
#include "RA_PopupWindows.h"
#include "RA_httpthread.h"
#include "RA_RichPresence.h"
#include "RA_md5factory.h"

#include <io.h>		//	_access()

AchievementSet* CoreAchievements = nullptr;
AchievementSet* UnofficialAchievements = nullptr;
AchievementSet* LocalAchievements = nullptr;

AchievementSet** ACH_SETS[] = { &CoreAchievements, &UnofficialAchievements, &LocalAchievements };
static_assert( SIZEOF_ARRAY( ACH_SETS ) == NumAchievementSetTypes, "Must match!" );

AchievementSetType g_nActiveAchievementSet = Core;
AchievementSet* g_pActiveAchievements = CoreAchievements;

//static 
std::string AchievementSet::m_sPreferredGameTitle;
GameID AchievementSet::m_nGameID = 0;

void RASetAchievementCollection( AchievementSetType Type )
{
	g_nActiveAchievementSet = Type;
	g_pActiveAchievements = *ACH_SETS[ Type ];
}

////////////////////////////////////////////////////////////////////////// 
//	static
std::string AchievementSet::GetAchievementSetFilename( GameID nGameID )
{
	//return RA_DIR_DATA + std::to_string( nGameID ) + ".txt";

	switch (g_nActiveAchievementSet)
	{
	case Local:
		return RA_DIR_DATA + std::to_string(nGameID) + "-User.txt";
	default:
		return RA_DIR_DATA + std::to_string(nGameID) + ".txt";
	}
}

//	static
BOOL AchievementSet::DeletePatchFile( AchievementSetType nSet, GameID nGameID )
{
	if( nGameID != 0 && g_nActiveAchievementSet != Local)
	{
		std::string sFilename = AchievementSet::GetAchievementSetFilename( nGameID );
							
		//	Remove the text file
		SetCurrentDirectory( Widen( g_sHomeDir ).c_str() );
		
		if( _access( sFilename.c_str(), 06 ) != -1 )	//	06= Read/write permission
		{
			if( remove( sFilename.c_str() ) == -1 )
			{
				//	Remove issues?
				ASSERT( !"Could not remove patch file!?" );
				return FALSE;
			}
		}

		return TRUE;
	}
	else
	{
		//	m_nGameID == 0, therefore there will be no patch file.
		return TRUE;
	}
}

//static 
void AchievementSet::OnRequestUnlocks( const Document& doc )
{
	if( !doc.HasMember( "Success" ) || doc[ "Success" ].GetBool() == false )
	{
		ASSERT( !"Invalid unlocks packet!" );
		return;
	}
	
	const GameID nGameID = static_cast<GameID>( doc["GameID"].GetUint() );
	const bool bHardcoreMode = doc[ "HardcoreMode" ].GetBool();
	const Value& UserUnlocks = doc[ "UserUnlocks" ];

	for( SizeType i = 0; i < UserUnlocks.Size(); ++i )
	{
		AchievementID nNextAchID = static_cast<AchievementID>( UserUnlocks[ i ].GetUint() );
		//	IDs could be present in either core or unofficial:
		if( CoreAchievements->Find( nNextAchID ) != nullptr )
			CoreAchievements->Unlock( nNextAchID );
		else if( UnofficialAchievements->Find( nNextAchID ) != nullptr )
			UnofficialAchievements->Unlock( nNextAchID );
	}
}

Achievement& AchievementSet::AddAchievement()
{
	m_Achievements.push_back( Achievement( m_nSetType ) );
	return m_Achievements.back();
}

BOOL AchievementSet::RemoveAchievement( size_t nIter )
{
	if( nIter < m_Achievements.size() )
	{
		m_Achievements.erase( m_Achievements.begin()+nIter );
		return TRUE;
	}
	else
	{
		ASSERT( !"There are no achievements to remove..." );
		return FALSE;
	}
}

Achievement* AchievementSet::Find( AchievementID nAchievementID )
{
	std::vector<Achievement>::iterator iter = m_Achievements.begin();
	while( iter != m_Achievements.end() )
	{
		if( (*iter).ID() == nAchievementID )
			return &(*iter);
		iter++;
	}

	return nullptr;
}

size_t AchievementSet::GetAchievementIndex( const Achievement& Ach )
{
	for( size_t nOffset = 0; nOffset < g_pActiveAchievements->NumAchievements(); ++nOffset )
	{
		if( &Ach == &GetAchievement( nOffset ) )
			return nOffset;
	}

	//	Not found
	return -1;
}

unsigned int AchievementSet::NumActive() const
{
	unsigned int nNumActive = 0;
	std::vector<Achievement>::const_iterator iter = m_Achievements.begin();
	while( iter != m_Achievements.end() )
	{
		if( (*iter).Active() )
			nNumActive++;
		iter++;
	}
	return nNumActive;
}

void AchievementSet::Clear()
{
	std::vector<Achievement>::iterator iter = m_Achievements.begin();
	while( iter != m_Achievements.end() )
	{
		iter->Clear();
		iter++;
	}

	m_Achievements.clear();
	m_bProcessingActive = TRUE;
}

void AchievementSet::Test()
{
	if( !m_bProcessingActive )
		return;
	
	for( std::vector<Achievement>::iterator iter = m_Achievements.begin(); iter != m_Achievements.end(); ++iter )
	{
		Achievement& ach = (*iter);
		if( !ach.Active() )
			continue;

		if( ach.Test() == TRUE )
		{
			//	Award. If can award or have already awarded, set inactive:
			ach.SetActive( FALSE );

			//	Reverse find where I am in the list:
			unsigned int nOffset = 0;
			for( nOffset = 0; nOffset < g_pActiveAchievements->NumAchievements(); ++nOffset )
 			{
 				if( &ach == &g_pActiveAchievements->GetAchievement( nOffset ) )
 					break;
 			}

 			ASSERT( nOffset < NumAchievements() );
 			if( nOffset < NumAchievements() )
				g_AchievementsDialog.ReloadLBXData( nOffset );

			if( RAUsers::LocalUser().IsLoggedIn() )
			{
				const std::string sPoints = std::to_string( ach.Points() );

				if( ach.ID() == 0 )
				{
					g_PopupWindows.AchievementPopups().AddMessage( 
						MessagePopup( "Test: Achievement Unlocked",
									  ach.Title() + " (" + sPoints + ") (Unofficial)",
									  PopupAchievementUnlocked,
									  ach.BadgeImage() ) );
				}
				else if( ach.Modified() )
				{
					g_PopupWindows.AchievementPopups().AddMessage( 
						MessagePopup( "Modified: Achievement Unlocked",
									  ach.Title() + " (" + sPoints + ") (Unofficial)",
									  PopupAchievementUnlocked,
									  ach.BadgeImage() ) );
				}
				else if( RAKeys_DoValidation == nullptr )
				{
					g_PopupWindows.AchievementPopups().AddMessage( 
						MessagePopup( "(Missing RA_Keys.DLL): Achievement Unlocked",
									  ach.Title() + " (" + sPoints + ") (Unofficial)",
									  PopupAchievementError,
									  ach.BadgeImage() ) );
				}
				else if( g_bRAMTamperedWith )
				{
					g_PopupWindows.AchievementPopups().AddMessage( 
						MessagePopup( "(RAM tampered with!): Achievement Unlocked",
									  ach.Title() + " (" + sPoints + ") (Unofficial)",
									  PopupAchievementError,
									  ach.BadgeImage() ) );
				}
				else
				{
					char sValidation[ 50 ];
					RAKeys_DoValidation( sValidation, RAUsers::LocalUser().Username().c_str(), RAUsers::LocalUser().Token().c_str(), ach.ID() );

					PostArgs args;
					args['u'] = RAUsers::LocalUser().Username();
					args['t'] = RAUsers::LocalUser().Token();
					args['a'] = std::to_string( ach.ID() );
					args['v'] = sValidation;
					args['h'] = std::to_string( static_cast<int>( g_bHardcoreModeActive ) );
					
					RAWeb::CreateThreadedHTTPRequest( RequestSubmitAwardAchievement, args );
				}
			}
		}
	}
}

BOOL AchievementSet::SaveToFile()
{
	//	Takes all achievements in this group and dumps them in the filename provided.
	FILE* pFile = NULL;
	char sNextLine[2048];
	char sMem[2048];
	unsigned int i = 0;

	const std::string sFilename = AchievementSet::GetAchievementSetFilename(m_nGameID);

	fopen_s(&pFile, sFilename.c_str(), "w");
	if (pFile != NULL)
	{
		sprintf_s(sNextLine, 2048, "0.030\n");						//	Min ver
		fwrite(sNextLine, sizeof(char), strlen(sNextLine), pFile);

		sprintf_s(sNextLine, 2048, "%s\n", m_sPreferredGameTitle.c_str());
		fwrite(sNextLine, sizeof(char), strlen(sNextLine), pFile);

		//std::vector<Achievement>::const_iterator iter = g_pActiveAchievements->m_Achievements.begin();
		//while (iter != g_pActiveAchievements->m_Achievements.end())
		for (i = 0; i < LocalAchievements->NumAchievements(); ++i)
		{
			Achievement* pAch = &LocalAchievements->GetAchievement(i);

			ZeroMemory(sMem, 2048);
			pAch->CreateMemString();

			ZeroMemory(sNextLine, 2048);
			sprintf_s(sNextLine, 2048, "%d:%s:%s:%s:%s:%s:%s:%s:%d:%lu:%lu:%d:%d:%s\n",
				pAch->ID(),
				pAch->CreateMemString().c_str(),
				pAch->Title().c_str(),
				pAch->Description().c_str(),
				" ", //Ach.ProgressIndicator()=="" ? " " : Ach.ProgressIndicator(),
				" ", //Ach.ProgressIndicatorMax()=="" ? " " : Ach.ProgressIndicatorMax(),
				" ", //Ach.ProgressIndicatorFormat()=="" ? " " : Ach.ProgressIndicatorFormat(),
				pAch->Author().c_str(),
				(unsigned short)pAch->Points(),
				(unsigned long)pAch->CreatedDate(),
				(unsigned long)pAch->ModifiedDate(),
				(unsigned short)pAch->Upvotes(), // Fix values
				(unsigned short)pAch->Downvotes(), // Fix values
				pAch->BadgeImageURI().c_str());

			fwrite(sNextLine, sizeof(char), strlen(sNextLine), pFile);
		}

		fclose(pFile);
		return TRUE;
	}
	
	return FALSE;

	//FILE* pf = NULL;
	//const std::string sFilename = AchievementSet::GetAchievementSetFilename( m_nGameID );
	//if( fopen_s( &pf, sFilename.c_str(), "wb" ) == 0 )
	//{
	//	FileStream fs( pf );
	//	
	//	CoreAchievements->Serialize( fs );
	//	UnofficialAchievements->Serialize( fs );
	//	LocalAchievements->Serialize( fs );

	//	fclose( pf );
	//}
}

BOOL AchievementSet::Serialize( FileStream& Stream )
{
	//	Why not submit each ach straight to cloud?
	return FALSE;

	//FILE* pf = NULL;
	//const std::string sFilename = AchievementSet::GetAchievementSetFilename( m_nGameID );
	//if( fopen_s( &pf, sFilename.c_str(), "wb" ) == 0 )
	//{
	//	FileStream fs( pf );
	//	Writer<FileStream> writer( fs );
	//	
	//	Document doc;
	//	doc.AddMember( "MinVer", "0.050", doc.GetAllocator() );
	//	doc.AddMember( "GameTitle", m_sPreferredGameTitle, doc.GetAllocator() );
	//	
	//	Value achElements;
	//	achElements.SetArray();

	//	std::vector<Achievement>::const_iterator iter = m_Achievements.begin();
	//	while( iter != m_Achievements.end() )
	//	{
	//		Value nextElement;

	//		const Achievement& ach = (*iter);
	//		nextElement.AddMember( "ID", ach.ID(), doc.GetAllocator() );
	//		nextElement.AddMember( "Mem", ach.CreateMemString(), doc.GetAllocator() );
	//		nextElement.AddMember( "Title", ach.Title(), doc.GetAllocator() );
	//		nextElement.AddMember( "Description", ach.Description(), doc.GetAllocator() );
	//		nextElement.AddMember( "Author", ach.Author(), doc.GetAllocator() );
	//		nextElement.AddMember( "Points", ach.Points(), doc.GetAllocator() );
	//		nextElement.AddMember( "Created", ach.CreatedDate(), doc.GetAllocator() );
	//		nextElement.AddMember( "Modified", ach.ModifiedDate(), doc.GetAllocator() );
	//		nextElement.AddMember( "Badge", ach.BadgeImageFilename(), doc.GetAllocator() );

	//		achElements.PushBack( nextElement, doc.GetAllocator() );
	//		iter++;
	//	}

	//	doc.AddMember( "Achievements", achElements, doc.GetAllocator() );

	//	//	Build a document to persist, then pass to doc.Accept();
	//	doc.Accept( writer );

	//	fclose( pf );
	//	return TRUE;
	//}
	//else
	//{
	//	//	Could not write to file?
	//	return FALSE;
	//}
}

BOOL AchievementSet::FetchFromWebBlocking( GameID nGameID )
{
	//	Can't open file: attempt to find it on SQL server!
	PostArgs args;
	args['u'] = RAUsers::LocalUser().Username();
	args['t'] = RAUsers::LocalUser().Token();
	args['g'] = std::to_string( nGameID );
	args['h'] = g_bHardcoreModeActive ? "1" : "0";

	Document doc;
	if( RAWeb::DoBlockingRequest( RequestPatch, args, doc ) && 
		doc.HasMember( "Success" ) && 
		doc[ "Success" ].GetBool() && 
		doc.HasMember( "PatchData" ) )
	{
		const Value& PatchData = doc[ "PatchData" ];
		SetCurrentDirectory( Widen( g_sHomeDir ).c_str() );
		FILE* pf = nullptr;
		fopen_s( &pf, std::string( RA_DIR_DATA + std::to_string( nGameID ) + ".txt" ).c_str(), "wb" );
		if( pf != nullptr )
		{
			FileStream fs( pf );
			Writer<FileStream> writer( fs );
			PatchData.Accept( writer );
			fclose( pf );
			return TRUE;
		}
		else
		{
			ASSERT( !"Could not open patch file for writing?" );
			RA_LOG( "Could not open patch file for writing?" );
			return FALSE;
		}
	}
	else
	{
		//	Could not connect...
		PopupWindows::AchievementPopups().AddMessage( 
			MessagePopup( "Could not connect to " RA_HOST_URL "...", "Working offline...", PopupInfo ) ); //?

		return FALSE;
	}
}

//	static
BOOL AchievementSet::LoadFromFile( GameID nGameID )
{
	if (nGameID == 0)
		return TRUE;

	const std::string sFilename = GetAchievementSetFilename(nGameID);

	SetCurrentDirectory(Widen(g_sHomeDir).c_str());
	FILE* pFile = NULL;
	errno_t err = 0;
	err = fopen_s(&pFile, sFilename.c_str(), "r");
	if (pFile != NULL)
	{
		//	Store this: we are now assuming this is the correct checksum if we have a file for it
		CoreAchievements->SetGameID(nGameID);
		UnofficialAchievements->SetGameID(nGameID);
		LocalAchievements->SetGameID(nGameID);

		if (g_nActiveAchievementSet == Local)
		{
			const char EndLine = '\n';
			char buffer[4096];
			unsigned long nCharsRead = 0;

			//	Get min ver:
			_ReadTil(EndLine, buffer, 4096, &nCharsRead, pFile);
			//	UNUSED at this point? TBD
			//	Get game title:
			_ReadTil(EndLine, buffer, 4096, &nCharsRead, pFile);
			if (nCharsRead > 0)
			{
				buffer[nCharsRead - 1] = '\0';	//	Turn that endline into an end-string
				m_sPreferredGameTitle = buffer;
			}

			while (!feof(pFile))
			{
				ZeroMemory(buffer, 4096);
				_ReadTil(EndLine, buffer, 4096, &nCharsRead, pFile);
				if (nCharsRead > 0)
				{
					if (buffer[0] == 'L')
					{
						//SKIP
						//RA_Leaderboard lb;
						//lb.ParseLine(buffer);

						//g_LeaderboardManager.AddLeaderboard(lb);
					}
					else if (isdigit(buffer[0]))
					{
						Achievement& newAch = LocalAchievements->AddAchievement();
						buffer[nCharsRead - 1] = '\0';	//	Turn that endline into an end-string

						newAch.ParseLine(buffer);
					}
				}
			}

			fclose(pFile);
			return TRUE;
		}
		else
		{

			Document doc;
			doc.ParseStream(FileStream(pFile));
			if (!doc.HasParseError())
			{
				//ASSERT( doc["Success"].GetBool() );
				//const Value& PatchData = doc["PatchData"];
				const GameID nGameIDFetched = doc["ID"].GetUint();
				ASSERT(nGameIDFetched == nGameID);
				const std::string& sGameTitle = doc["Title"].GetString();
				const unsigned int nConsoleID = doc["ConsoleID"].GetUint();
				const std::string& sConsoleName = doc["ConsoleName"].GetString();
				const unsigned int nForumTopicID = doc["ForumTopicID"].GetUint();
				const unsigned int nGameFlags = doc["Flags"].GetUint();
				const std::string& sImageIcon = doc["ImageIcon"].GetString();
				const std::string& sImageTitle = doc["ImageTitle"].GetString();
				const std::string& sImageIngame = doc["ImageIngame"].GetString();
				const std::string& sImageBoxArt = doc["ImageBoxArt"].GetString();
				const std::string& sPublisher = doc["Publisher"].IsNull() ? "Unknown" : doc["Publisher"].GetString();
				const std::string& sDeveloper = doc["Developer"].IsNull() ? "Unknown" : doc["Developer"].GetString();
				const std::string& sGenre = doc["Genre"].IsNull() ? "Unknown" : doc["Genre"].GetString();
				const std::string& sReleased = doc["Released"].IsNull() ? "Unknown" : doc["Released"].GetString();
				const bool bIsFinal = doc["IsFinal"].GetBool();
				const std::string& sRichPresencePatch = doc["RichPresencePatch"].IsNull() ? "" : doc["RichPresencePatch"].GetString();

				//##SD store all this data somewhere? Do we want it?
				m_sPreferredGameTitle = sGameTitle;

				RA_RichPresenceInterpretter::PersistAndParseScript(nGameIDFetched, sRichPresencePatch);

				const Value& AchievementsData = doc["Achievements"];

				for (SizeType i = 0; i < AchievementsData.Size(); ++i)
				{
					//	Parse into correct boxes
					unsigned int nFlags = AchievementsData[i]["Flags"].GetUint();
					if (nFlags == 3)
					{
						Achievement& newAch = CoreAchievements->AddAchievement();
						newAch.Parse(AchievementsData[i]);
					}
					else if (nFlags == 5)
					{
						Achievement& newAch = UnofficialAchievements->AddAchievement();
						newAch.Parse(AchievementsData[i]);
					}
					else
					{
						RA_LOG("Cannot deal with achievement with flags: %d\n", nFlags);
					}
				}

				const Value& LeaderboardsData = doc["Leaderboards"];

				for (SizeType i = 0; i < LeaderboardsData.Size(); ++i)
				{
					//"Leaderboards":[{"ID":"2","Mem":"STA:0xfe10=h0000_0xhf601=h0c_d0xhf601!=h0c_0xfff0=0_0xfffb=0::CAN:0xhfe13<d0xhfe13::SUB:0xf7cc!=0_d0xf7cc=0::VAL:0xhfe24*1_0xhfe25*60_0xhfe22*3600","Format":"TIME","Title":"Green Hill Act 1","Description":"Complete this act in the fastest time!"},

					RA_Leaderboard lb(LeaderboardsData[i]["ID"].GetUint());
					lb.LoadFromJSON(LeaderboardsData[i]);

					g_LeaderboardManager.AddLeaderboard(lb);
				}
			}
			else
			{
				fclose(pFile);
				ASSERT(!"Could not parse file?!");
				return FALSE;
			}

			fclose(pFile);

			unsigned int nTotalPoints = 0;
			for (size_t i = 0; i < CoreAchievements->NumAchievements(); ++i)
				nTotalPoints += CoreAchievements->GetAchievement(i).Points();

			if (RAUsers::LocalUser().IsLoggedIn())
			{
				//	Loaded OK: post a request for unlocks
				PostArgs args;
				args['u'] = RAUsers::LocalUser().Username();
				args['t'] = RAUsers::LocalUser().Token();
				args['g'] = std::to_string(nGameID);
				args['h'] = g_bHardcoreModeActive ? "1" : "0";

				RAWeb::CreateThreadedHTTPRequest(RequestUnlocks, args);

				std::string sNumCoreAch = std::to_string(CoreAchievements->NumAchievements());

				g_PopupWindows.AchievementPopups().AddMessage(
					MessagePopup("Loaded " + sNumCoreAch + " achievements, Total Score " + std::to_string(nTotalPoints), "", PopupInfo));	//	TBD
			}

			return TRUE;
		}
	}
	else
	{
		//	Cannot open file
		RA_LOG("Cannot open file %s\n", sFilename.c_str());
		RA_LOG("Error %s\n", strerror(err));
		return FALSE;
	}
}

void AchievementSet::SaveProgress( const char* sSaveStateFilename )
{
	if( !RAUsers::LocalUser().IsLoggedIn() )
		return;

	if( sSaveStateFilename == NULL )
		return;
	
	SetCurrentDirectory( Widen( g_sHomeDir ).c_str() );
	char buffer[ 4096 ];
	sprintf_s( buffer, 4096, "%s.rap", sSaveStateFilename );
	FILE* pf = NULL;
	fopen_s( &pf, buffer, "w" );
	if( pf == NULL )
	{
		ASSERT( !"Could not save progress!" );
		return;
	}

	for( size_t i = 0; i < NumAchievements(); ++i )
	{
		Achievement* pAch = &m_Achievements[i];
		if( !pAch->Active() )
			continue;

		//	Write ID of achievement and num conditions:
		char cheevoProgressString[4096];
		memset( cheevoProgressString, '\0', 4096 );

		for( unsigned int nGrp = 0; nGrp < pAch->NumConditionGroups(); ++nGrp )
		{
			sprintf_s( buffer, "%d:%d:", pAch->ID(), pAch->NumConditions( nGrp ) );
			strcat_s( cheevoProgressString, 4096, buffer );

			for( unsigned int j = 0; j < pAch->NumConditions( nGrp ); ++j )
			{
				Condition& cond = pAch->GetCondition( nGrp, j );
				sprintf_s( buffer, 4096, "%d:%d:%d:%d:%d:", 
					cond.CurrentHits(),
					cond.CompSource().RawValue(),
					cond.CompSource().RawPreviousValue(),
					cond.CompTarget().RawValue(),
					cond.CompTarget().RawPreviousValue() );
				strcat_s( cheevoProgressString, 4096, buffer );
			}
		}
		
		//	Generate a slightly different key to md5ify:
		char sCheevoProgressMangled[4096];
		sprintf_s( sCheevoProgressMangled, 4096, "%s%s%s%d", 
			RAUsers::LocalUser().Username().c_str(), cheevoProgressString, RAUsers::LocalUser().Username().c_str(), pAch->ID() );
		
		std::string sMD5Progress = RAGenerateMD5( std::string( sCheevoProgressMangled ) );
		std::string sMD5Achievement = RAGenerateMD5( pAch->CreateMemString() );
		
		fwrite( cheevoProgressString, sizeof(char), strlen(cheevoProgressString), pf );
		fwrite( sMD5Progress.c_str(), sizeof(char), sMD5Progress.length(), pf );
		fwrite( ":", sizeof(char), 1, pf );
		fwrite( sMD5Achievement.c_str(), sizeof(char), sMD5Achievement.length(), pf );
		fwrite( ":", sizeof(char), 1, pf );	//	Check!
	}

	fclose( pf );
}

void AchievementSet::LoadProgress( const char* sLoadStateFilename )
{
	char buffer[4096];
	long nFileSize = 0;
	unsigned int CondNumHits[50];	//	50 conditions per achievement
	unsigned int CondSourceVal[50];
	unsigned int CondSourceLastVal[50];
	unsigned int CondTargetVal[50];
	unsigned int CondTargetLastVal[50];
	unsigned int nID = 0;
	unsigned int nNumCond = 0;
	char cheevoProgressString[4096];
	unsigned int i = 0;
	unsigned int j = 0;
	char* pGivenProgressMD5 = NULL;
	char* pGivenCheevoMD5 = NULL;
	char cheevoMD5TestMangled[4096];
	int nMemStringLen = 0;

	if( !RAUsers::LocalUser().IsLoggedIn() )
		return;

	if( sLoadStateFilename == NULL )
		return;

	sprintf_s( buffer, 4096, "%s%s.rap", RA_DIR_DATA, sLoadStateFilename );

	char* pRawFile = _MallocAndBulkReadFileToBuffer( buffer, nFileSize );

	if( pRawFile != NULL )
	{
		unsigned int nOffs = 0;
		while( nOffs < (unsigned int)(nFileSize-2) && pRawFile[nOffs] != '\0' )
		{
			char* pIter = &pRawFile[nOffs];

			//	Parse achievement id and num conditions
			nID		 = strtol( pIter, &pIter, 10 ); pIter++;
			nNumCond = strtol( pIter, &pIter, 10 );	pIter++;

			//	Concurrently build the md5 checkstring
			sprintf_s( cheevoProgressString, 4096, "%d:%d:", nID, nNumCond );

			ZeroMemory( CondNumHits, 50*sizeof(unsigned int) );
			ZeroMemory( CondSourceVal, 50*sizeof(unsigned int) );
			ZeroMemory( CondSourceLastVal, 50*sizeof(unsigned int) );
			ZeroMemory( CondTargetVal, 50*sizeof(unsigned int) );
			ZeroMemory( CondTargetLastVal, 50*sizeof(unsigned int) );

			for( i = 0; i < nNumCond && i < 50; ++i )
			{
				//	Parse next condition state
				CondNumHits[i]		 = strtol( pIter, &pIter, 10 ); pIter++;
				CondSourceVal[i]	 = strtol( pIter, &pIter, 10 ); pIter++;
				CondSourceLastVal[i] = strtol( pIter, &pIter, 10 ); pIter++;
				CondTargetVal[i]	 = strtol( pIter, &pIter, 10 ); pIter++;
				CondTargetLastVal[i] = strtol( pIter, &pIter, 10 ); pIter++;
			
				//	Concurrently build the md5 checkstring
				sprintf_s( buffer, 4096, "%d:%d:%d:%d:%d:", 
					CondNumHits[i], 
					CondSourceVal[i],
					CondSourceLastVal[i],
					CondTargetVal[i],
					CondTargetLastVal[i] );

				strcat_s( cheevoProgressString, 4096, buffer );
			}

			//	Read the given md5:
			pGivenProgressMD5 = strtok_s( pIter, ":", &pIter );
			pGivenCheevoMD5 = strtok_s( pIter, ":", &pIter );
		
			//	Regenerate the md5 and see if it sticks:
			sprintf_s( cheevoMD5TestMangled, 4096, "%s%s%s%d", 
				RAUsers::LocalUser().Username().c_str(), cheevoProgressString, RAUsers::LocalUser().Username().c_str(), nID );

			std::string sRecalculatedProgressMD5 = RAGenerateMD5( cheevoMD5TestMangled );

			if( sRecalculatedProgressMD5.compare( pGivenProgressMD5 ) == 0 )
			{
				//	Embed in achievement:
				Achievement* pAch = Find( nID );
				if( pAch != NULL )
				{
					std::string sMemStr = pAch->CreateMemString();

					//	Recalculate the current achievement to see if it's compatible:
					std::string sMemMD5 = RAGenerateMD5( sMemStr );
					if( sMemMD5.compare( 0, 32, pGivenCheevoMD5 ) == 0 )
					{
						for( size_t nGrp = 0; nGrp < pAch->NumConditionGroups(); ++nGrp )
						{
							for( j = 0; j < pAch->NumConditions( nGrp ); ++j )
							{
								Condition& cond = pAch->GetCondition( nGrp, j );

								cond.OverrideCurrentHits( CondNumHits[ j ] );
								cond.CompSource().SetValues( CondSourceVal[ j ], CondSourceLastVal[ j ] );
								cond.CompTarget().SetValues( CondTargetVal[ j ], CondTargetLastVal[ j ] );

								pAch->SetDirtyFlag( Dirty_Conditions );
							}
						}
					}
					else
					{
						ASSERT( !"Achievement progress savestate incompatible (achievement has changed?)" );
						RA_LOG( "Achievement progress savestate incompatible (achievement has changed?)" );
					}
				}
				else
				{
					ASSERT( !"Achievement doesn't exist!" );
					RA_LOG( "Achievement doesn't exist!" );
				}
			}
			else
			{
				//assert(!"MD5 invalid... what to do... maybe they're trying to hack achievements?");
			}
		
			nOffs = (pIter - pRawFile);
		}
	
		free( pRawFile );
		pRawFile = NULL;
	}
}

Achievement& AchievementSet::Clone( unsigned int nIter )
{
	Achievement& newAch = AddAchievement();		//	Create a brand new achievement
	newAch.Set( m_Achievements[nIter] );		//	Copy in all values from the clone src
	newAch.SetID( 0 );
	newAch.SetModified( TRUE );
	newAch.SetActive( FALSE );

	return newAch;
}

BOOL AchievementSet::Unlock( AchievementID nAchID )
{
	for( size_t i = 0; i < NumAchievements(); ++i )
	{
		if( m_Achievements[ i ].ID() == nAchID )
		{
			m_Achievements[ i ].SetActive( FALSE );
			return TRUE;	//	Update Dlg? //TBD
		}
	}

	RA_LOG( "Attempted to unlock achievement %d but failed!\n", nAchID );
	return FALSE;//??
}

BOOL AchievementSet::HasUnsavedChanges()
{
	for( size_t i = 0; i < NumAchievements(); ++i )
	{
		if( m_Achievements[i].Modified() )
			return TRUE;
	}

	return FALSE;
}

