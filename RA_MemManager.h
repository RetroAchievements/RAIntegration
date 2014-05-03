#ifndef _IGMEMORYDEBUG_H
#define _IGMEMORYDEBUG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <wtypes.h>

typedef struct
{
	//MemCandidate() : m_nAddr(0), m_nLastKnownValue(0) {}

	DWORD m_nAddr;
	unsigned int m_nLastKnownValue;		//	A Candidate MAY be a 32-bit candidate!
	BOOL m_bUpperNibble;				//	Used only for 4-bit comparisons
} MemCandidate;

//extern enum ComparisonSize;
static unsigned char RAMByteStandard( unsigned int nOffs );
static unsigned char RAMByteGBC( unsigned int nOffs );
static void RAMByteWriteStandard( unsigned int nOffs, unsigned int nVal );
static void RAMByteWriteGBC( unsigned int nOffs, unsigned int nVal );

typedef unsigned char (_RAMByteReadFn)(size_t nOffset);
typedef void (_RAMByteWriteFn)(unsigned int nOffs, unsigned int nVal);

class MemManager
{
public:
	MemManager();
	~MemManager();

public:
	void Init();
	void InstallRAM( unsigned char* pRAM, unsigned int nRAMBytes, unsigned char* pRAMExtra = NULL, unsigned int nRAMExtraBytes = 0 );
	void Reset( int comparisonSize );

	unsigned int Compare( unsigned int nCompareType, unsigned int nTestValue, BOOL& bResultsFound );
	
	inline DWORD ValidMemAddrFound( int iter ) const			{ return m_Candidates[ iter ].m_nAddr; }
	inline DWORD RAMTotalSize() const							{ return ( m_nCoreRAMSize + m_nRAMExtraSize ); }

	inline unsigned char RAMByte(unsigned int nOffs) const					{ return (*m_pfnRAMByteRead)(nOffs); }
	inline void RAMByteWrite(unsigned int nOffs, unsigned int nVal) const	{ (*m_pfnRAMByteWrite)(nOffs, nVal); }

	void PokeByte( unsigned int nAddr, unsigned int nValue );

public:	
	_RAMByteReadFn*  m_pfnRAMByteRead;		//	Installable function ptr.
	_RAMByteWriteFn* m_pfnRAMByteWrite;		//	Installable function ptr.

	MemCandidate* m_Candidates;	//	Pointer to an array
	unsigned char* m_pCoreRAM;
	unsigned int m_nCoreRAMSize;

	unsigned char* m_pRAMExtra;	//	Used for VBA's Working Memory at the moment
	unsigned int m_nRAMExtraSize;
	
	unsigned int m_NumCandidates;

	/*ComparisonSize*/int m_nComparisonSizeMode;
	BOOL m_bUseLastKnownValue;
	HWND m_hMemDialog;
};

extern MemManager g_MemManager;


static unsigned char RAMByteStandard( unsigned int nOffs )
{
	if( nOffs >= g_MemManager.RAMTotalSize() )
		return 0;

	return (nOffs<g_MemManager.m_nCoreRAMSize) ? *(g_MemManager.m_pCoreRAM+nOffs) : (nOffs<g_MemManager.RAMTotalSize()) ? *(g_MemManager.m_pRAMExtra+(nOffs-g_MemManager.m_nCoreRAMSize)) : 0;
}

//	Alternative for GBC
static unsigned char RAMByteGBC( unsigned int nOffs )
{
	if( nOffs >= g_MemManager.RAMTotalSize() )
		return 0;

	return ( (unsigned char**)g_MemManager.m_pCoreRAM)[(nOffs) >> 12][(nOffs) & 0xfff];
}

static void RAMByteStandardWrite( unsigned int nOffs, unsigned int nVal )
{
	if( nOffs >= g_MemManager.RAMTotalSize() )
		return;

	if( nOffs < g_MemManager.m_nCoreRAMSize )
	{
		*( g_MemManager.m_pCoreRAM+nOffs ) = nVal;
	}
	else
	{
		nOffs -= g_MemManager.m_nCoreRAMSize;
		*( g_MemManager.m_pRAMExtra+nOffs ) = nVal;
	}
}

//	Alternative for GBC
static void RAMByteGBCWrite( unsigned int nOffs, unsigned int nVal )
{
	if( nOffs >= g_MemManager.RAMTotalSize() )
		return;

	( (unsigned char**)g_MemManager.m_pCoreRAM)[(nOffs) >> 12][(nOffs) & 0xfff] = nVal;
}

typedef void (*writefunc)( unsigned int A, unsigned char V );
typedef unsigned char (*readfunc)( unsigned int A );

static unsigned char RAMByteNES( unsigned int nOffs )
{
	//	g_MemManager.m_pCoreRAM is an array of readfunc function ptrs which we cast to, THEN dereference, then call
	return ( ( (readfunc*)( g_MemManager.m_pCoreRAM ) )[nOffs] )(nOffs);
}

static void RAMByteNESWrite( unsigned int nOffs, unsigned int nVal )
{
	//	g_MemManager.m_pRAMExtra is an array of writefunc function ptrs which we cast to, THEN dereference, then call
	( (writefunc*)( g_MemManager.m_pRAMExtra ) )[nOffs]( nOffs, nVal );
}

//unsigned short MemManager_ValidMemAddrFound( int iter );

#ifdef __cplusplus
};
#endif

#endif //_IGMEMORYDEBUG_H