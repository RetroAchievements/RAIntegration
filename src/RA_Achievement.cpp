#include "RA_Achievement.h"

#include "RA_md5factory.h"

#ifndef RA_UTEST
#include "RA_Core.h"
#else
// duplicate of code in RA_Core, but RA_Core needs to be cleaned up before it can be pulled into the unit test build
void _ReadStringTil(std::string& value, char nChar, const char*& pSource)
{
    const char* pStartString = pSource;

    while (*pSource != '\0' && *pSource != nChar)
        pSource++;

    value.assign(pStartString, pSource - pStartString);
    pSource++;
}
#endif

//////////////////////////////////////////////////////////////////////////

Achievement::Achievement(AchievementSetType nType) :
    m_nSetType(nType), m_bPauseOnTrigger(FALSE), m_bPauseOnReset(FALSE)
{
    Clear();

    m_vConditions.AddGroup();
}

#ifndef RA_UTEST

void Achievement::Parse(const rapidjson::Value& element)
{
    //{"ID":"36","MemAddr":"0xfe20>=50","Title":"Fifty Rings","Description":"Collect 50 rings","Points":"0","Author":"Scott","Modified":"1351868953","Created":"1351814592","BadgeName":"00083","Flags":"5"},
    m_nAchievementID = element["ID"].GetUint();
    m_sTitle = element["Title"].GetString();
    m_sDescription = element["Description"].GetString();
    m_nPointValue = element["Points"].GetUint();
    m_sAuthor = element["Author"].GetString();
    m_nTimestampModified = element["Modified"].GetUint();
    m_nTimestampCreated = element["Created"].GetUint();
    //m_sBadgeImageURI = element["BadgeName"].GetString();
    SetBadgeImage(element["BadgeName"].GetString());
    //unsigned int nFlags = element["Flags"].GetUint();


    if (element["MemAddr"].IsString())
    {
        const char* sMem = element["MemAddr"].GetString();
        if (!m_vConditions.ParseFromString(sMem) || *sMem != '\0')
        {
            ASSERT(!"Invalid MemAddr");
            m_vConditions.Clear();
        }
    }

    SetActive(IsCoreAchievement());	//	Activate core by default
}

#endif

const char* Achievement::ParseLine(const char* pBuffer)
{
    std::string sTemp;

    if (pBuffer == nullptr || pBuffer[0] == '\0')
        return pBuffer;

    if (pBuffer[0] == '/' || pBuffer[0] == '\\')
        return pBuffer;

    //	Read ID of achievement:
    _ReadStringTil(sTemp, ':', pBuffer);
    SetID((unsigned int)atol(sTemp.c_str()));

    //	parse conditions
    m_vConditions.ParseFromString(pBuffer);

    // Skip any whitespace/colons
    while (*pBuffer == ' ' || *pBuffer == ':')
        pBuffer++;

    SetActive(false);

    //	buffer now contains TITLE : DESCRIPTION : PROGRESS(unused) : PROGRESS_MAX(unused) : PROGRESS_FMT(unused) :
    //                      AUTHOR : POINTS : CREATED : MODIFIED : UPVOTES : DOWNVOTES : BADGEFILENAME

    _ReadStringTil(sTemp, ':', pBuffer);
    SetTitle(sTemp);

    _ReadStringTil(sTemp, ':', pBuffer);
    SetDescription(sTemp);

    _ReadStringTil(sTemp, ':', pBuffer);
    SetProgressIndicator(sTemp);

    _ReadStringTil(sTemp, ':', pBuffer);
    SetProgressIndicatorMax(sTemp);

    _ReadStringTil(sTemp, ':', pBuffer);
    SetProgressIndicatorFormat(sTemp);

    _ReadStringTil(sTemp, ':', pBuffer);
    SetAuthor(sTemp);

    _ReadStringTil(sTemp, ':', pBuffer);
    SetPoints((unsigned int)atol(sTemp.c_str()));

    _ReadStringTil(sTemp, ':', pBuffer);
    SetCreatedDate((time_t)atol(sTemp.c_str()));

    _ReadStringTil(sTemp, ':', pBuffer);
    SetModifiedDate((time_t)atol(sTemp.c_str()));

    _ReadStringTil(sTemp, ':', pBuffer);
    // SetUpvotes( (unsigned short)atol( sTemp.c_str() ) );

    _ReadStringTil(sTemp, ':', pBuffer);
    // SetDownvotes( (unsigned short)atol( sTemp.c_str() ) );

    _ReadStringTil(sTemp, ':', pBuffer);
    SetBadgeImage(sTemp);

    return pBuffer;
}

static bool HasHitCounts(const ConditionSet& set)
{
    for (size_t i = 0; i < set.GroupCount(); i++)
    {
        const ConditionGroup& group = set.GetGroup(i);
        for (size_t j = 0; j < group.Count(); j++)
        {
            const Condition& condition = group.GetAt(j);
            if (condition.CurrentHits() > 0)
                return true;
        }
    }

    return false;
}

BOOL Achievement::Test()
{
    bool bDirtyConditions = FALSE;
    bool bResetConditions = FALSE;

    bool bNotifyOnReset = GetPauseOnReset() && HasHitCounts(m_vConditions);

    bool bRetVal = m_vConditions.Test(bDirtyConditions, bResetConditions);

    if (bDirtyConditions)
    {
        SetDirtyFlag(Dirty_Conditions);
    }

    if (bResetConditions && bNotifyOnReset)
    {
#ifndef RA_UTEST
        RA_CausePause();

        char buffer[256];
        sprintf_s(buffer, 256, "Pause on Reset: %s", Title().c_str());
        MessageBox(g_RAMainWnd, NativeStr(buffer).c_str(), TEXT("Paused"), MB_OK);
#endif
    }

    return bRetVal;
}

void Achievement::Clear()
{
    m_vConditions.Clear();

    m_nAchievementID = 0;

    m_sTitle.clear();
    m_sDescription.clear();
    m_sAuthor.clear();
    m_sBadgeImageURI.clear();

    m_nPointValue = 0;
    m_bActive = FALSE;
    m_bModified = FALSE;
    m_bPauseOnTrigger = FALSE;
    m_bPauseOnReset = FALSE;
    ClearDirtyFlag();

    m_bProgressEnabled = FALSE;
    m_sProgress[0] = '\0';
    m_sProgressMax[0] = '\0';
    m_sProgressFmt[0] = '\0';
    m_fProgressLastShown = 0.0f;

    m_nTimestampCreated = 0;
    m_nTimestampModified = 0;
    //m_nUpvotes = 0;
    //m_nDownvotes = 0;
}

void Achievement::AddConditionGroup()
{
    m_vConditions.AddGroup();
}

void Achievement::RemoveConditionGroup()
{
    m_vConditions.RemoveLastGroup();
}

void Achievement::SetID(ra::AchievementID nID)
{
    m_nAchievementID = nID;
    SetDirtyFlag(Dirty_ID);
}

void Achievement::SetActive(BOOL bActive)
{
    if (m_bActive != bActive)
    {
        m_bActive = bActive;
        SetDirtyFlag(Dirty__All);
    }
}

//void Achievement::SetUpvotes( unsigned short nVal )
//{
//	m_nUpvotes = nVal;
//	SetDirtyFlag( Dirty_Votes );
//}
//
//void Achievement::SetDownvotes( unsigned short nVal )
//{
//	m_nDownvotes = nVal;
//	SetDirtyFlag( Dirty_Votes );
//}

void Achievement::SetModified(BOOL bModified)
{
    if (m_bModified != bModified)
    {
        m_bModified = bModified;
        SetDirtyFlag(Dirty__All);	//	TBD? questionable...
    }
}

void Achievement::SetBadgeImage(const std::string& sBadgeURI)
{
    SetDirtyFlag(Dirty_Badge);

    if (sBadgeURI.length() > 5 && strcmp(&sBadgeURI[sBadgeURI.length() - 5], "_lock") == 0)
        m_sBadgeImageURI.assign(sBadgeURI.c_str(), sBadgeURI.length() - 5);
    else
        m_sBadgeImageURI = sBadgeURI;
}

void Achievement::Reset()
{
    if (m_vConditions.Reset())
        SetDirtyFlag(Dirty_Conditions);
}

size_t Achievement::AddCondition(size_t nConditionGroup, const Condition& rNewCond)
{
    while (NumConditionGroups() <= nConditionGroup)	//	ENSURE this is a legal op!
        m_vConditions.AddGroup();

    ConditionGroup& group = m_vConditions.GetGroup(nConditionGroup);
    group.Add(rNewCond);	//	NB. Copy by value	
    SetDirtyFlag(Dirty__All);

    return group.Count();
}

size_t Achievement::InsertCondition(size_t nConditionGroup, size_t nIndex, const Condition& rNewCond)
{
    while (NumConditionGroups() <= nConditionGroup)	//	ENSURE this is a legal op!
        m_vConditions.AddGroup();

    ConditionGroup& group = m_vConditions.GetGroup(nConditionGroup);
    group.Insert(nIndex, rNewCond);	//	NB. Copy by value	
    SetDirtyFlag(Dirty__All);

    return group.Count();
}

BOOL Achievement::RemoveCondition(size_t nConditionGroup, unsigned int nID)
{
    if (nConditionGroup < m_vConditions.GroupCount())
    {
        m_vConditions.GetGroup(nConditionGroup).RemoveAt(nID);
        SetDirtyFlag(Dirty__All);	//	Not Conditions: 
        return TRUE;
    }

    return FALSE;
}

void Achievement::RemoveAllConditions(size_t nConditionGroup)
{
    if (nConditionGroup < m_vConditions.GroupCount())
    {
        m_vConditions.GetGroup(nConditionGroup).Clear();
        SetDirtyFlag(Dirty__All);	//	All - not just conditions
    }
}

std::string Achievement::CreateMemString() const
{
    std::string buffer;
    m_vConditions.Serialize(buffer);
    return buffer;
}

void Achievement::Set(const Achievement& rRHS)
{
    SetID(rRHS.m_nAchievementID);
    SetActive(rRHS.m_bActive);
    SetAuthor(rRHS.m_sAuthor);
    SetDescription(rRHS.m_sDescription);
    SetPoints(rRHS.m_nPointValue);
    SetTitle(rRHS.m_sTitle);
    SetModified(rRHS.m_bModified);
    SetBadgeImage(rRHS.m_sBadgeImageURI);

    //	TBD: move to 'now'?
    SetModifiedDate(rRHS.m_nTimestampModified);
    SetCreatedDate(rRHS.m_nTimestampCreated);

    for (size_t i = 0; i < NumConditionGroups(); ++i)
        RemoveAllConditions(i);
    m_vConditions.Clear();

    for (size_t nGrp = 0; nGrp < rRHS.NumConditionGroups(); ++nGrp)
    {
        m_vConditions.AddGroup();

        const ConditionGroup& group = rRHS.m_vConditions.GetGroup(nGrp);
        for (size_t i = 0; i < group.Count(); ++i)
            AddCondition(nGrp, group.GetAt(i));
    }

    SetDirtyFlag(Dirty__All);
}

//int Achievement::StoreDynamicVar( char* pVarName, CompVariable nVar )
//{
//	ASSERT( m_nNumDynamicVars < 5 );
// 	
//	//m_nDynamicVars[m_nNumDynamicVars].m_nVar = nVar;
//	//strcpy_s( m_nDynamicVars[m_nNumDynamicVars].m_sName, 16, pVarName );
// 	
//	m_nNumDynamicVars++;
// 
//	return m_nNumDynamicVars;
//}
//
//
//float Achievement::ProgressGetNextStep( char* sFormat, float fLastKnownProgress )
//{
//	const float fStep = ( (float)strtol( sFormat, nullptr, 10 ) / 100.0f );
//	int nIter = 1;	//	Progress of 0% doesn't require a popup...
//	float fStepAt = fStep * nIter;
//	while( (fLastKnownProgress >= fStepAt) && (fStepAt < 1.0f) && (nIter < 20) )
//	{
//		nIter++;
//		fStepAt = fStep * nIter;
//	}
// 
//	if( fStepAt >= 1.0f )
//		fStepAt = 1.0f;
// 
//	return fStepAt;
//}
//
//float Achievement::ParseProgressExpression( char* pExp )
//{
//	if( pExp == nullptr )
//		return 0.0f;
// 
//	int nOperator = 0;	//	0=add, 1=mult, 2=sub
// 
//	while( *pExp == ' ' )
//		++pExp; //	Trim
// 
//	float fProgressValue = 0.0f;
//	char* pStrIter = &pExp[0];
// 
//	int nIterations = 0;
// 
//	while( *pStrIter != nullptr && nIterations < 20 )
//	{
//		float fNextVal = 0.0f;
// 
//		//	Parse operator
//		if( pStrIter == pExp )
//		{
//			//	Start of string: assume operator of 'add'
//			nOperator = 0;
//		}
//		else
//		{
//			if( *pStrIter == '+' )
//				nOperator = 0;
//			else if( *pStrIter == '*' )
//				nOperator = 1;
//			else if( *pStrIter == '-' )
//				nOperator = 2;
//			else
//			{
//				char buffer[256];
//				sprintf_s( buffer, 256, "Unrecognised operator character at %d",
//					(&pStrIter) - (&pExp) );
// 
//				ASSERT(!"Unrecognised operator in format expression!");
//				return 0.0f;
//			}
//			
//			pStrIter++;
//		}
// 
//		//	Parse value:
//		if( strncmp( pStrIter, "Cond:", 5 ) == 0 )
//		{
//			//	Get the specified condition, and the value from it.
//			unsigned int nCondIter = strtol( pStrIter+5, nullptr, 10 );
//			
//			if( nCondIter < NumConditions() )
//			{
//				Condition& Cond = GetCondition( nCondIter );
//				if( Cond.CompSource().m_nVarType != CMPTYPE_VALUE )
//				{
//					fNextVal = (float)Cond.CompSource().m_nVal;
//				}
//				else if( Cond.CompTarget().m_nVarType != CMPTYPE_VALUE )
//				{
//					//	Best guess?
//					fNextVal = (float)Cond.CompTarget().m_nVal;
//				}
//				else
//				{
//					//wtf? Both elements are 'value'?
//					ASSERT(!"Bolloxed");
//					fNextVal = 0.0f;
//				}
//			}
//			else
//			{
//				//	Report error?
//				ASSERT(!"Bolloxed2");
//				return 0.0f;
//			}
//		}
//		else if( pStrIter[0] == '0' && pStrIter[1] == 'x' )
//		{
//			CompVariable Var;							//	Memory location
//			Var.ParseVariable( pStrIter );				//	Automatically pushes pStrIter
//			fNextVal = (float)( Var.GetValue() );
//		}
//		else
//		{
//			//	Assume value, assume base 10
//			fNextVal = (float)( strtol( pStrIter, &pStrIter, 10 ) );
//		}
// 
//		switch( nOperator )
//		{
//		case 0:	//	Add
//			fProgressValue += fNextVal;
//			break;
//		case 1:	//	Mult
//			fProgressValue *= fNextVal;
//			break;
//		case 2:	//	Sub
//			fProgressValue -= fNextVal;
//			break;
//		default:
//			ASSERT(!"Unrecognised operator?!");
//			break;
//		}
// 
//		while( *pExp == ' ' )
//			++pExp; //	Trim any whitespace
// 
//		nIterations++;
//	}
// 
//	if( nIterations == 20 )
//	{
//		ASSERT(!"Bugger... can't parse this 'progress' thing. Too many iterations!");
//	}
// 
//	return fProgressValue;
//}

//void Achievement::UpdateProgress()
// {
//	//	Produce a float which represents the current progress.
//	//	Compare to m_sProgressFmt. If greater and 
//	//	Compare to m_fProgressLastShown
// 
//	//	TBD: Don't forget backslashes!
//	//sprintf_s( m_sProgress, 256,	"0xfe20", 10 );	//	every 10 percent
//	//sprintf_s( m_sProgressMax, 256, "200", 10 );	//	every 10 percent
//	//sprintf_s( m_sProgressFmt, 50,	"%d,%%01.f,%%01.f,,", 10 );	//	every 10 percent
//	
//	const float fProgressValue = ParseProgressExpression( m_sProgress );
//	const float fMaxValue = ParseProgressExpression( m_sProgressMax );
// 
//	if( fMaxValue == 0.0f )
//		return;
// 
//	const float fProgressPercent = ( fProgressValue / fMaxValue );
//	const float fNextStep = ProgressGetNextStep( m_sProgressFmt, fProgressPercent );
// 
//	if( fProgressPercent >= m_fProgressLastShown && m_fProgressLastShown < fNextStep )
//	{
//		if( m_fProgressLastShown == 0.0f )
//		{
//			m_fProgressLastShown = fNextStep;
//			return;	//	Don't show first indicator!
//		}
//		else if( fNextStep == 1.0f )
//		{
//			return;	//	Don't show final indicator!
//		}
// 
//		m_fProgressLastShown = fNextStep;
//		
//		char formatSpareBuffer[256];
//		strcpy_s( formatSpareBuffer, 256, m_sProgressFmt );
// 
//		//	DO NOT USE Sprintf!! this will interpret the format!
//		//	sprintf_s( formatSpareBuffer, 256, m_sProgressFmt ); 
// 
//		char* pUnused = &formatSpareBuffer[0];
//		char* pUnused2 = nullptr;
//		char* pFirstFmt = nullptr;
//		char* pSecondFmt = nullptr;
//		strtok_s( pUnused, ",", &pFirstFmt );
//		strtok_s( pFirstFmt, ",", &pSecondFmt );
//		strtok_s( pSecondFmt, ",\0\n:", &pUnused2 );	//	Adds a very useful '\0' to pSecondFmt 
//		
//		if( pFirstFmt==nullptr || pSecondFmt==nullptr )
//		{
//			ASSERT(!"FUCK. Format string is fucked");
//			return;
//		}
//		
//		//	Display progress message:
//		
//		char bufferVal1[64];
//		sprintf_s( bufferVal1, 64, pFirstFmt, fProgressValue );
//		
//		char bufferVal2[64];
//		sprintf_s( bufferVal2, 64, pSecondFmt, fMaxValue );
// 
//		char progressTitle[256];
//		sprintf_s( progressTitle, 256, " Progress: %s ", Title() );
// 
//		char progressDesc[256];
//		sprintf_s( progressDesc, 256, " %s of %s ", bufferVal1, bufferVal2 );
//		g_PopupWindows.ProgressPopups().AddMessage( progressTitle, progressDesc );
//	}
// }

std::string Achievement::CreateStateString(const std::string& sSalt) const
{
    // build the progress string
    std::ostringstream oss;

    for (unsigned int nGroup = 0; nGroup < NumConditionGroups(); ++nGroup)
    {
        const ConditionGroup& group = m_vConditions.GetGroup(nGroup);
        oss << ID() << ":" << group.Count() << ":";

        for (unsigned int j = 0; j < group.Count(); ++j)
        {
            const Condition& cond = group.GetAt(j);
            oss << cond.CurrentHits() << ":" <<
                cond.CompSource().RawValue() << ":" << cond.CompSource().RawPreviousValue() << ":" <<
                cond.CompTarget().RawValue() << ":" << cond.CompTarget().RawPreviousValue() << ":";
        }
    }

    std::string sProgressString = oss.str();

    // Checksum the progress string (including the salt value)
    std::string sModifiedProgressString;
    sModifiedProgressString.resize(sProgressString.length() + sSalt.length() * 2 + 10);
    size_t nNewSize = sprintf_s(sModifiedProgressString.data(), sModifiedProgressString.capacity(),
        "%s%s%s%u", sSalt.c_str(), sProgressString.c_str(), sSalt.c_str(), static_cast<unsigned int>(ID()));
    sModifiedProgressString.resize(nNewSize);
    std::string sMD5Progress = RAGenerateMD5(sModifiedProgressString);

    sProgressString.append(sMD5Progress);
    sProgressString.push_back(':');

    // Also checksum the achievement string itself
    std::string sMD5Achievement = RAGenerateMD5(CreateMemString());
    sProgressString.append(sMD5Achievement);
    sProgressString.push_back(':');

    return sProgressString;
}

const char* Achievement::ParseStateString(const char* sBuffer, const std::string& sSalt)
{
    std::vector<Condition> vConditions;
    const char* pIter = sBuffer;
    bool bSuccess = true;

    // recalculate the current achievement checksum
    std::string sMD5Achievement = RAGenerateMD5(CreateMemString());

    // parse achievement id and conditions
    while (*pIter)
    {
        char* pEnd;

        const char* pStart = pIter;
        unsigned int nID = strtoul(pIter, &pEnd, 10); pIter = pEnd + 1;
        if (nID != ID())
        {
            pIter = pStart;
            break;
        }

        unsigned int nNumCond = strtoul(pIter, &pEnd, 10); pIter = pEnd + 1;
        vConditions.reserve(vConditions.size() + nNumCond);

        for (size_t i = 0; i < nNumCond; ++i)
        {
            // Parse next condition state
            unsigned int nHits = strtoul(pIter, &pEnd, 10); pIter = pEnd + 1;
            unsigned int nSourceVal = strtoul(pIter, &pEnd, 10); pIter = pEnd + 1;
            unsigned int nSourcePrev = strtoul(pIter, &pEnd, 10); pIter = pEnd + 1;
            unsigned int nTargetVal = strtoul(pIter, &pEnd, 10); pIter = pEnd + 1;
            unsigned int nTargetPrev = strtoul(pIter, &pEnd, 10); pIter = pEnd + 1;

            Condition& cond = vConditions.emplace_back();
            cond.OverrideCurrentHits(nHits);
            cond.CompSource().SetValues(nSourceVal, nSourcePrev);
            cond.CompTarget().SetValues(nTargetVal, nTargetPrev);
        }
    }

    const char* pEnd = pIter;

    // read the given md5s
    std::string sGivenMD5Progress;
    _ReadStringTil(sGivenMD5Progress, ':', pIter);

    std::string sGivenMD5Achievement;
    _ReadStringTil(sGivenMD5Achievement, ':', pIter);

    // if the achievement is still compatible
    if (sGivenMD5Achievement == sMD5Achievement)
    {
        // regenerate the md5 and see if it sticks
        std::string sModifiedProgressString;
        sModifiedProgressString.resize(pEnd - sBuffer + sSalt.length() * 2 + 10);
        size_t nNewSize = sprintf_s(sModifiedProgressString.data(), sModifiedProgressString.capacity(),
            "%s%.*s%s%u", sSalt.c_str(), pEnd - sBuffer, sBuffer, sSalt.c_str(), ID());
        sModifiedProgressString.resize(nNewSize);
        std::string sMD5Progress = RAGenerateMD5(sModifiedProgressString);
        if (sMD5Progress == sGivenMD5Progress)
        {
            // compatible - merge
            size_t nCondition = 0;
            for (size_t nConditionGroup = 0; nConditionGroup < m_vConditions.GroupCount(); ++nConditionGroup)
            {
                ConditionGroup& group = m_vConditions.GetGroup(nConditionGroup);
                for (size_t i = 0; i < group.Count(); ++i)
                {
                    Condition& condSource = vConditions.at(nCondition++);
                    Condition& condTarget = group.GetAt(i);

                    condTarget.OverrideCurrentHits(condSource.CurrentHits());
                    condTarget.CompSource().SetValues(condSource.CompSource().RawValue(), condSource.CompSource().RawPreviousValue());
                    condTarget.CompTarget().SetValues(condSource.CompTarget().RawValue(), condSource.CompTarget().RawPreviousValue());
                }
            }
        }
        else
        {
            // state checksum fail
            bSuccess = false;
        }
    }
    else
    {
        // achievment checksum fail
        bSuccess = false;
    }

    if (bSuccess)
        SetDirtyFlag(Dirty_Conditions);
    else
        Reset();

    return pIter;
}
