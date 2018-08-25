#include "RA_Achievement.h"

#include "RA_Core.h"
#include "RA_ImageFactory.h"

//	No game-specific code here please!

//////////////////////////////////////////////////////////////////////////

_Use_decl_annotations_
Achievement::Achievement(ra::AchievementSetType nType) noexcept :
    m_nSetType{ nType }
{
    Clear();

    m_vConditions.AddGroup();
}

void Achievement::Parse(const Value& element)
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
        SetDirtyFlag(ra::Achievement_DirtyFlags::Conditions);
    }

    if (bResetConditions && bNotifyOnReset)
    {
        RA_CausePause();

        char buffer[256];
        sprintf_s(buffer, 256, "Pause on Reset: %s", Title().c_str());
        MessageBox(g_RAMainWnd, NativeStr(buffer).c_str(), TEXT("Paused"), MB_OK);
    }

    return bRetVal;
}

void Achievement::Clear() noexcept
{
    m_vConditions.Clear();

    m_nAchievementID = ra::AchievementID{};

    m_sTitle.clear();
    m_sDescription.clear();
    m_sAuthor.clear();
    m_sBadgeImageURI.clear();

    m_nPointValue     = 0U;
    m_bActive         = FALSE;
    m_bModified       = FALSE;
    m_bPauseOnTrigger = FALSE;
    m_bPauseOnReset   = FALSE;
    ClearDirtyFlag();

    m_bProgressEnabled   = FALSE;
    m_sProgress          = "";
    m_sProgressMax       = "";
    m_sProgressFmt       = "";
    m_fProgressLastShown = 0.0F;

    m_nTimestampCreated  = std::time_t{};
    m_nTimestampModified = std::time_t{};
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
    SetDirtyFlag(ra::Achievement_DirtyFlags::ID);
}

void Achievement::SetActive(BOOL bActive)
{
    if (m_bActive != bActive)
    {
        m_bActive = bActive;
        SetDirtyFlag(ra::Achievement_DirtyFlags::All);
    }
}

//void Achievement::SetUpvotes( unsigned short nVal )
//{
//	m_nUpvotes = nVal;
//	SetDirtyFlag( ra::Achievement_DirtyFlags::Votes );
//}
//
//void Achievement::SetDownvotes( unsigned short nVal )
//{
//	m_nDownvotes = nVal;
//	SetDirtyFlag( ra::Achievement_DirtyFlags::Votes );
//}

void Achievement::SetModified(BOOL bModified)
{
    if (m_bModified != bModified)
    {
        m_bModified = bModified;
        SetDirtyFlag(ra::Achievement_DirtyFlags::All);	//	TBD? questionable...
    }
}

void Achievement::SetBadgeImage(const std::string& sBadgeURI)
{
    SetDirtyFlag(ra::Achievement_DirtyFlags::Badge);

    if (sBadgeURI.length() > 5 && strcmp(&sBadgeURI[sBadgeURI.length() - 5], "_lock") == 0)
        m_sBadgeImageURI.assign(sBadgeURI.c_str(), sBadgeURI.length() - 5);
    else
        m_sBadgeImageURI = sBadgeURI;
}

void Achievement::Reset()
{
    if (m_vConditions.Reset())
        SetDirtyFlag(ra::Achievement_DirtyFlags::Conditions);
}

size_t Achievement::AddCondition(size_t nConditionGroup, const Condition& rNewCond)
{
    while (NumConditionGroups() <= nConditionGroup)	//	ENSURE this is a legal op!
        m_vConditions.AddGroup();

    ConditionGroup& group = m_vConditions.GetGroup(nConditionGroup);
    group.Add(rNewCond);	//	NB. Copy by value	
    SetDirtyFlag(ra::Achievement_DirtyFlags::All);

    return group.Count();
}

size_t Achievement::InsertCondition(size_t nConditionGroup, size_t nIndex, const Condition& rNewCond)
{
    while (NumConditionGroups() <= nConditionGroup)	//	ENSURE this is a legal op!
        m_vConditions.AddGroup();

    ConditionGroup& group = m_vConditions.GetGroup(nConditionGroup);
    group.Insert(nIndex, rNewCond);	//	NB. Copy by value	
    SetDirtyFlag(ra::Achievement_DirtyFlags::All);

    return group.Count();
}

BOOL Achievement::RemoveCondition(size_t nConditionGroup, unsigned int nID)
{
    if (nConditionGroup < m_vConditions.GroupCount())
    {
        m_vConditions.GetGroup(nConditionGroup).RemoveAt(nID);
        SetDirtyFlag(ra::Achievement_DirtyFlags::All);	//	Not Conditions: 
        return TRUE;
    }

    return FALSE;
}

void Achievement::RemoveAllConditions(size_t nConditionGroup)
{
    if (nConditionGroup < m_vConditions.GroupCount())
    {
        m_vConditions.GetGroup(nConditionGroup).Clear();
        SetDirtyFlag(ra::Achievement_DirtyFlags::All);	//	All - not just conditions
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

    SetDirtyFlag(ra::Achievement_DirtyFlags::All);
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
