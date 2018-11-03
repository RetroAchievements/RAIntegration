#include "RA_Achievement.h"

#include "RA_MemManager.h"
#include "RA_md5factory.h"

#include "RA_Defs.h"

#ifndef RA_UTEST
#include "RA_ImageFactory.h"
#endif


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

Achievement::Achievement() noexcept
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
        ParseTrigger(sMem);
    }
}
#endif

void Achievement::RebuildTrigger()
{
    std::string sTrigger;
    m_vConditions.Serialize(sTrigger);

    ParseTrigger(sTrigger.c_str());
    SetDirtyFlag(DirtyFlags::Conditions);
}

static MemSize GetCompVariableSize(char nOperandSize)
{
    switch (nOperandSize)
    {
        case RC_OPERAND_BIT_0: return MemSize::Bit_0;
        case RC_OPERAND_BIT_1: return MemSize::Bit_1;
        case RC_OPERAND_BIT_2: return MemSize::Bit_2;
        case RC_OPERAND_BIT_3: return MemSize::Bit_3;
        case RC_OPERAND_BIT_4: return MemSize::Bit_4;
        case RC_OPERAND_BIT_5: return MemSize::Bit_5;
        case RC_OPERAND_BIT_6: return MemSize::Bit_6;
        case RC_OPERAND_BIT_7: return MemSize::Bit_7;
        case RC_OPERAND_LOW: return MemSize::Nibble_Lower;
        case RC_OPERAND_HIGH: return MemSize::Nibble_Upper;
        case RC_OPERAND_8_BITS: return MemSize::EightBit;
        case RC_OPERAND_16_BITS: return MemSize::SixteenBit;
        case RC_OPERAND_32_BITS: return MemSize::ThirtyTwoBit;
        default:
            ASSERT(!"Unsupported operand size");
            return MemSize::EightBit;
    }
}

static void SetOperand(CompVariable& var, rc_operand_t& operand)
{
    switch (operand.type)
    {
        case RC_OPERAND_ADDRESS:
            var.Set(GetCompVariableSize(operand.size), CompVariable::Type::Address, operand.value);
            break;
            
        case RC_OPERAND_DELTA:
            var.Set(GetCompVariableSize(operand.size), CompVariable::Type::DeltaMem, operand.value);
            break;

        case RC_OPERAND_CONST:
            var.Set(MemSize::ThirtyTwoBit, CompVariable::Type::ValueComparison, operand.value);
            break;

        case RC_OPERAND_FP:
            ASSERT(!"Floating point operand not supported");
            var.Set(MemSize::ThirtyTwoBit, CompVariable::Type::ValueComparison, 0U);
            break;

        case RC_OPERAND_LUA:
            ASSERT(!"Lua operand not supported");
            var.Set(MemSize::ThirtyTwoBit, CompVariable::Type::ValueComparison, 0U);
            break;
    }
}

static void MakeConditionGroup(ConditionSet& vConditions, rc_condset_t* pCondSet)
{
    vConditions.AddGroup();
    ConditionGroup& group = vConditions.GetGroup(vConditions.GroupCount() - 1);

    rc_condition_t* pCondition = pCondSet->conditions;
    while (pCondition != nullptr)
    {
        Condition cond;
        SetOperand(cond.CompSource(), pCondition->operand1);
        SetOperand(cond.CompTarget(), pCondition->operand2);

        switch (pCondition->oper)
        {
            default:
                ASSERT(!"Unsupported operator");
                _FALLTHROUGH;
            case RC_CONDITION_EQ: cond.SetCompareType(ComparisonType::Equals); break;
            case RC_CONDITION_NE: cond.SetCompareType(ComparisonType::NotEqualTo); break;
            case RC_CONDITION_LT: cond.SetCompareType(ComparisonType::LessThan); break;
            case RC_CONDITION_LE: cond.SetCompareType(ComparisonType::LessThanOrEqual); break;
            case RC_CONDITION_GT: cond.SetCompareType(ComparisonType::GreaterThan); break;
            case RC_CONDITION_GE: cond.SetCompareType(ComparisonType::GreaterThanOrEqual); break;
        }

        switch (pCondition->type)
        {
            default:
                ASSERT(!"Unsupported condition type");
                _FALLTHROUGH;
            case RC_CONDITION_STANDARD: cond.SetConditionType(Condition::ConditionType::Standard); break;
            case RC_CONDITION_RESET_IF: cond.SetConditionType(Condition::ConditionType::ResetIf); break;
            case RC_CONDITION_PAUSE_IF: cond.SetConditionType(Condition::ConditionType::PauseIf); break;
            case RC_CONDITION_ADD_SOURCE: cond.SetConditionType(Condition::ConditionType::AddSource); break;
            case RC_CONDITION_SUB_SOURCE: cond.SetConditionType(Condition::ConditionType::SubSource); break;
            case RC_CONDITION_ADD_HITS: cond.SetConditionType(Condition::ConditionType::AddHits); break;
        }

        cond.SetRequiredHits(pCondition->required_hits);

        group.Add(cond);
        pCondition = pCondition->next;
    }
}

void Achievement::ParseTrigger(const char* sTrigger)
{
    m_vConditions.Clear();

    int nSize = rc_trigger_size(sTrigger);
    if (nSize < 0)
    {
        // parse error occurred
        RA_LOG("rc_parse_trigger returned %d", nSize);
        m_pTrigger = nullptr;
    }
    else
    {
        // allocate space and parse again
        m_pTriggerBuffer.reset(new unsigned char[nSize]);
        auto* pTrigger = rc_parse_trigger(static_cast<void*>(m_pTriggerBuffer.get()), sTrigger, nullptr, 0);
        m_pTrigger = pTrigger;

        // wrap rc_trigger_t in a ConditionSet for the UI
        MakeConditionGroup(m_vConditions, pTrigger->requirement);
        rc_condset_t* alternative = pTrigger->alternative;
        while (alternative != nullptr)
        {
            MakeConditionGroup(m_vConditions, alternative);
            alternative = alternative->next;
        }
    }
}

const char* Achievement::ParseLine(const char* pBuffer)
{
#ifndef RA_UTEST
    std::string sTemp;

    if (pBuffer == nullptr || pBuffer[0] == '\0')
        return pBuffer;

    if (pBuffer[0] == '/' || pBuffer[0] == '\\')
        return pBuffer;

    //	Read ID of achievement:
    _ReadStringTil(sTemp, ':', pBuffer);
    SetID((unsigned int)atol(sTemp.c_str()));

    //	parse conditions
    const char* pTrigger = pBuffer;
    while (*pBuffer && (pBuffer[0] != ':' || strchr("ABCPRabcpr", pBuffer[-1]) != nullptr))
        pBuffer++;
    ParseTrigger(pTrigger);

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
#endif
    return pBuffer;
}

static bool HasHitCounts(const rc_condset_t* pCondSet)
{
    rc_condition_t* condition = pCondSet->conditions;
    while (condition != nullptr)
    {
        if (condition->current_hits)
            return true;

        condition = condition->next;
    }

    return false;
}

static bool HasHitCounts(const rc_trigger_t* pTrigger)
{
    if (HasHitCounts(pTrigger->requirement))
        return true;

    rc_condset_t* pAlternate = pTrigger->alternative;
    while (pAlternate != nullptr)
    {
        if (HasHitCounts(pAlternate))
            return true;

        pAlternate = pAlternate->next;
    }

    return false;
}

bool Achievement::Test()
{
    if (m_pTrigger == nullptr)
        return false;

    rc_trigger_t* pTrigger = static_cast<rc_trigger_t*>(m_pTrigger);

    bool bNotifyOnReset = GetPauseOnReset() && HasHitCounts(pTrigger);

    bool bRetVal = rc_test_trigger(pTrigger, rc_peek_callback, nullptr, nullptr);

    if (bNotifyOnReset && !HasHitCounts(pTrigger))
    {
#ifndef RA_UTEST
        RA_CausePause();

        char buffer[256];
        sprintf_s(buffer, 256, "Pause on Reset: %s", Title().c_str());
        MessageBox(g_RAMainWnd, NativeStr(buffer).c_str(), TEXT("Paused"), MB_OK);
#endif
    }

    SetDirtyFlag(DirtyFlags::Conditions);
    return bRetVal;
}

static rc_condition_t* GetTriggerCondition(rc_trigger_t* pTrigger, size_t nGroup, size_t nIndex)
{
    rc_condset_t* pGroup = pTrigger->requirement;
    if (nGroup > 0)
    {
        pGroup = pTrigger->alternative;
        --nGroup;

        while (pGroup && nGroup > 0)
        {
            pGroup = pGroup->next;
            --nGroup;
        }
    }

    if (!pGroup)
        return nullptr;

    rc_condition_t* pCondition = pGroup->conditions;
    while (pCondition && nIndex > 0)
    {
        --nIndex;
        pCondition = pCondition->next;
    }

    return pCondition;
}

unsigned int Achievement::GetConditionHitCount(size_t nGroup, size_t nIndex) const
{
    if (m_pTrigger == nullptr)
        return 0U;

    rc_trigger_t* pTrigger = static_cast<rc_trigger_t*>(m_pTrigger);
    rc_condition_t* pCondition = GetTriggerCondition(pTrigger, nGroup, nIndex);
    return pCondition ? pCondition->current_hits : 0U;
}

int Achievement::StoreConditionState(size_t nGroup, size_t nIndex, char *pBuffer) const
{
    if (m_pTrigger != nullptr)
    {
        rc_trigger_t* pTrigger = static_cast<rc_trigger_t*>(m_pTrigger);
        rc_condition_t* pCondition = GetTriggerCondition(pTrigger, nGroup, nIndex);
        if (pCondition)
        {
            return snprintf(pBuffer, 128, "%u:%u:%u:%u:%u:", pCondition->current_hits, pCondition->operand1.value, pCondition->operand1.previous,
                pCondition->operand2.value, pCondition->operand2.previous);
        }
    }

    return snprintf(pBuffer, 128, "0:0:0:0:0:");
}

void Achievement::RestoreConditionState(size_t nGroup, size_t nIndex, unsigned int nCurrentHits, unsigned int nValue, unsigned int nPreviousValue)
{
    if (m_pTrigger == nullptr)
        return;

    rc_trigger_t* pTrigger = static_cast<rc_trigger_t*>(m_pTrigger);
    rc_condition_t* pCondition = GetTriggerCondition(pTrigger, nGroup, nIndex);
    if (!pCondition)
        return;

    pCondition->current_hits = nCurrentHits;
    pCondition->operand2.value = nValue;
    pCondition->operand2.previous = nPreviousValue;
}

void Achievement::Clear()
{
    m_vConditions.Clear();


    m_nAchievementID = 0;
    m_pTriggerBuffer.reset();
    m_pTrigger = nullptr;

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
    SetDirtyFlag(DirtyFlags::ID);
}

void Achievement::SetActive(BOOL bActive)
{
    if (m_bActive != bActive)
    {
        m_bActive = bActive;
        SetDirtyFlag(DirtyFlags::All);
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
        SetDirtyFlag(DirtyFlags::All);	//	TBD? questionable...
    }
}

void Achievement::SetBadgeImage(const std::string& sBadgeURI)
{
    SetDirtyFlag(DirtyFlags::Badge);

    if (sBadgeURI.length() > 5 && strcmp(&sBadgeURI[sBadgeURI.length() - 5], "_lock") == 0)
        m_sBadgeImageURI.assign(sBadgeURI.c_str(), sBadgeURI.length() - 5);
    else
        m_sBadgeImageURI = sBadgeURI;
}

void Achievement::Reset()
{
    if (m_pTrigger)
    {
        rc_trigger_t* pTrigger = static_cast<rc_trigger_t*>(m_pTrigger);
        rc_reset_trigger(pTrigger);

        SetDirtyFlag(DirtyFlags::Conditions);
    }
}

size_t Achievement::AddCondition(size_t nConditionGroup, const Condition& rNewCond)
{
    while (NumConditionGroups() <= nConditionGroup)	//	ENSURE this is a legal op!
        m_vConditions.AddGroup();

    ConditionGroup& group = m_vConditions.GetGroup(nConditionGroup);
    group.Add(rNewCond);	//	NB. Copy by value	
    SetDirtyFlag(DirtyFlags::All);

    return group.Count();
}

size_t Achievement::InsertCondition(size_t nConditionGroup, size_t nIndex, const Condition& rNewCond)
{
    while (NumConditionGroups() <= nConditionGroup)	//	ENSURE this is a legal op!
        m_vConditions.AddGroup();

    ConditionGroup& group = m_vConditions.GetGroup(nConditionGroup);
    group.Insert(nIndex, rNewCond);	//	NB. Copy by value	
    SetDirtyFlag(DirtyFlags::All);

    return group.Count();
}

BOOL Achievement::RemoveCondition(size_t nConditionGroup, unsigned int nID)
{
    if (nConditionGroup < m_vConditions.GroupCount())
    {
        m_vConditions.GetGroup(nConditionGroup).RemoveAt(nID);
        SetDirtyFlag(DirtyFlags::All);	//	Not Conditions: 
        return TRUE;
    }

    return FALSE;
}

void Achievement::RemoveAllConditions(size_t nConditionGroup)
{
    if (nConditionGroup < m_vConditions.GroupCount())
    {
        m_vConditions.GetGroup(nConditionGroup).Clear();
        SetDirtyFlag(DirtyFlags::All);	//	All - not just conditions
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

    SetDirtyFlag(DirtyFlags::All);
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

    auto* pTrigger = static_cast<rc_trigger_t*>(m_pTrigger);
    if (pTrigger == nullptr)
    {
        oss << ID() << ":0:";
    }
    else
    {
        rc_condset_t* pGroup = pTrigger->requirement;
        while (pGroup != nullptr)
        {
            size_t nGroups = 0;
            rc_condition_t* pCondition = pGroup->conditions;
            while (pCondition )
            {
                ++nGroups;
                pCondition = pCondition->next;
            }
            oss << ID() << ':' << nGroups << ':';

            pCondition = pGroup->conditions;
            while (pCondition != nullptr)
            {
                oss << pCondition->current_hits << ':'
                    << pCondition->operand1.value << ':' << pCondition->operand1.previous << ':'
                    << pCondition->operand2.value << ':' << pCondition->operand2.previous << ':';

                pCondition = pCondition->next;
            }

            if (pGroup == pTrigger->requirement)
                pGroup = pTrigger->alternative;
            else
                pGroup = pGroup->next;
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
    std::vector<rc_condition_t> vConditions;
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

            rc_condition_t& cond = vConditions.emplace_back();
            cond.current_hits = nHits;
            cond.operand1.value = nSourceVal;
            cond.operand1.previous = nSourcePrev;
            cond.operand2.value = nTargetVal;
            cond.operand2.previous = nTargetPrev;
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

            auto* pTrigger = static_cast<rc_trigger_t*>(m_pTrigger);
            rc_condset_t* pGroup = pTrigger->requirement;
            while (pGroup != nullptr)
            {
                rc_condition_t* pCondition = pGroup->conditions;
                while (pCondition != nullptr)
                {
                    rc_condition_t& condSource = vConditions.at(nCondition++);
                    pCondition->current_hits = condSource.current_hits;
                    pCondition->operand1.value = condSource.operand1.value;
                    pCondition->operand1.previous = condSource.operand1.previous;
                    pCondition->operand2.value = condSource.operand2.value;
                    pCondition->operand2.previous = condSource.operand2.previous;

                    pCondition = pCondition->next;
                }

                if (pGroup == pTrigger->requirement)
                    pGroup = pTrigger->alternative;
                else
                    pGroup = pGroup->next;
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
        // achievement checksum fail
        bSuccess = false;
    }

    if (bSuccess)
        SetDirtyFlag(DirtyFlags::Conditions);
    else
        Reset();

    return pIter;
}
