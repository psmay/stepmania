#include "global.h"
#include "SongUtil.h"
#include "Song.h"
#include "Steps.h"
#include "GameState.h"
#include "Style.h"
#include "ProfileManager.h"
#include "Profile.h"
#include "PrefsManager.h"
#include "RageString.hpp"
#include "SongManager.h"
#include "XmlFile.h"
#include "UnlockManager.h"
#include "ThemeMetric.h"
#include "LocalizedString.h"
#include "RageLog.h"
#include "GameManager.h"
#include "CommonMetrics.h"
#include "LuaBinding.h"
#include "EnumHelper.h"
#include "RageFmtWrap.h"

#include <unordered_map>

using std::vector;

ThemeMetric<int> SORT_BPM_DIVISION ( "MusicWheel", "SortBPMDivision" );
ThemeMetric<int> SORT_LENGTH_DIVISION ( "MusicWheel", "SortLengthDivision" );
ThemeMetric<bool> SHOW_SECTIONS_IN_BPM_SORT ( "MusicWheel", "ShowSectionsInBPMSort" );
ThemeMetric<bool> SHOW_SECTIONS_IN_LENGTH_SORT ( "MusicWheel", "ShowSectionsInLengthSort" );

bool SongCriteria::Matches( const Song *pSong ) const
{
	if( !m_sGroupName.empty()  &&  m_sGroupName != pSong->m_sGroupName )
		return false;

	if( UNLOCKMAN->SongIsLocked(pSong) & LOCKED_DISABLED )
		return false;

	if( m_bUseSongGenreAllowedList )
	{
		if( find(m_vsSongGenreAllowedList.begin(),m_vsSongGenreAllowedList.end(),pSong->m_sGenre) == m_vsSongGenreAllowedList.end() )
			return false;
	}

	switch( m_Selectable )
	{
	DEFAULT_FAIL(m_Selectable);
	case Selectable_Yes:
		if( UNLOCKMAN  &&  UNLOCKMAN->SongIsLocked(pSong) & LOCKED_SELECTABLE )
			return false;
		break;
	case Selectable_No:
		if( UNLOCKMAN  &&  !(UNLOCKMAN->SongIsLocked(pSong) & LOCKED_SELECTABLE) )
			return false;
		break;
	case Selectable_DontCare:
		break;
	}

	if( m_bUseSongAllowedList )
	{
		if( find(m_vpSongAllowedList.begin(),m_vpSongAllowedList.end(),pSong) == m_vpSongAllowedList.end() )
			return false;
	}

	if( m_iMaxStagesForSong != -1  &&  GAMESTATE->GetNumStagesMultiplierForSong(pSong) > m_iMaxStagesForSong )
		return false;

	switch( m_Tutorial )
	{
	DEFAULT_FAIL(m_Tutorial);
	case Tutorial_Yes:
		if( !pSong->IsTutorial() )
			return false;
		break;
	case Tutorial_No:
		if( pSong->IsTutorial() )
			return false;
		break;
	case Tutorial_DontCare:
		break;
	}

	switch( m_Locked )
	{
	DEFAULT_FAIL(m_Locked);
	case Locked_Locked:
		if( UNLOCKMAN  &&  !(UNLOCKMAN->SongIsLocked(pSong) & LOCKED_LOCK) )
			return false;
		break;
	case Locked_Unlocked:
		if( UNLOCKMAN  &&  UNLOCKMAN->SongIsLocked(pSong) & LOCKED_LOCK )
			return false;
		break;
	case Locked_DontCare:
		break;
	}

	return true;
}

void SongUtil::GetSteps(
	const Song *pSong,
	vector<Steps*>& arrayAddTo,
	StepsType st,
	Difficulty dc,
	int iMeterLow,
	int iMeterHigh,
	const std::string &sDescription,
	const std::string &sCredit,
	bool bIncludeAutoGen,
	unsigned uHash,
	int iMaxToGet
	)
{
	if( !iMaxToGet )
		return;

	const vector<Steps*> &vpSteps = st == StepsType_Invalid ? pSong->GetAllSteps() : pSong->GetStepsByStepsType(st);
	for (auto *pSteps: vpSteps)
	{
		if( dc != Difficulty_Invalid && dc != pSteps->GetDifficulty() )
			continue;
		if( iMeterLow != -1 && iMeterLow > pSteps->GetMeter() )
			continue;
		if( iMeterHigh != -1 && iMeterHigh < pSteps->GetMeter() )
			continue;
		if( sDescription.size() && sDescription != pSteps->GetDescription() )
			continue;
		if( sCredit.size() && sCredit != pSteps->GetCredit() )
			continue;
		if( uHash != 0 && uHash != pSteps->GetHash() )
			continue;
		if( !bIncludeAutoGen && pSteps->IsAutogen() )
			continue;

		arrayAddTo.push_back( pSteps );

		if( iMaxToGet != -1 )
		{
			--iMaxToGet;
			if( !iMaxToGet )
				break;
		}
	}
}

Steps* SongUtil::GetOneSteps(
	const Song *pSong,
	StepsType st,
	Difficulty dc,
	int iMeterLow,
	int iMeterHigh,
	const std::string &sDescription,
	const std::string &sCredit,
	unsigned uHash,
	bool bIncludeAutoGen
	)
{
	vector<Steps*> vpSteps;
	GetSteps( pSong, vpSteps, st, dc, iMeterLow, iMeterHigh, sDescription, sCredit, bIncludeAutoGen, uHash, 1 );	// get max 1
	if( vpSteps.empty() )
		return nullptr;
	else
		return vpSteps[0];
}

Steps* SongUtil::GetStepsByDifficulty( const Song *pSong, StepsType st, Difficulty dc, bool bIncludeAutoGen )
{
	const vector<Steps*>& vpSteps = (st == StepsType_Invalid)? pSong->GetAllSteps() : pSong->GetStepsByStepsType(st);
	for (auto *pSteps: vpSteps)
	{
		if( dc != Difficulty_Invalid && dc != pSteps->GetDifficulty() )
			continue;
		if( !bIncludeAutoGen && pSteps->IsAutogen() )
			continue;

		return pSteps;
	}

	return nullptr;
}

Steps* SongUtil::GetStepsByMeter( const Song *pSong, StepsType st, int iMeterLow, int iMeterHigh )
{
	const vector<Steps*>& vpSteps = (st == StepsType_Invalid)? pSong->GetAllSteps() : pSong->GetStepsByStepsType(st);
	for (auto *pSteps: vpSteps)
	{
		if( iMeterLow > pSteps->GetMeter() )
			continue;
		if( iMeterHigh < pSteps->GetMeter() )
			continue;

		return pSteps;
	}

	return nullptr;
}

Steps* SongUtil::GetStepsByDescription( const Song *pSong, StepsType st, std::string sDescription )
{
	vector<Steps*> vNotes;
	GetSteps( pSong, vNotes, st, Difficulty_Invalid, -1, -1, sDescription, "" );
	if( vNotes.size() == 0 )
		return nullptr;
	else
		return vNotes[0];
}

Steps* SongUtil::GetStepsByCredit( const Song *pSong, StepsType st, std::string sCredit )
{
	vector<Steps*> vNotes;
	GetSteps(pSong, vNotes, st, Difficulty_Invalid, -1, -1, "", sCredit );
	if( vNotes.size() == 0 )
		return nullptr;
	else
		return vNotes[0];
}


Steps* SongUtil::GetClosestNotes( const Song *pSong, StepsType st, Difficulty dc, bool bIgnoreLocked )
{
	ASSERT( dc != Difficulty_Invalid );

	const vector<Steps*>& vpSteps = (st == StepsType_Invalid)? pSong->GetAllSteps() : pSong->GetStepsByStepsType(st);
	Steps *pClosest = nullptr;
	int iClosestDistance = 999;
	for (auto *pSteps: vpSteps)
	{
		if( pSteps->GetDifficulty() == Difficulty_Edit && dc != Difficulty_Edit )
			continue;
		if( bIgnoreLocked && UNLOCKMAN->StepsIsLocked(pSong,pSteps) )
			continue;

		int iDistance = abs(dc - pSteps->GetDifficulty());
		if( iDistance < iClosestDistance )
		{
			pClosest = pSteps;
			iClosestDistance = iDistance;
		}
	}

	return pClosest;
}


/* Make any duplicate difficulties edits.  (Note that BMS files do a first pass
 * on this; see BMSLoader::SlideDuplicateDifficulties.) */
void SongUtil::AdjustDuplicateSteps( Song *pSong )
{
	FOREACH_ENUM( StepsType, st )
	{
		FOREACH_ENUM( Difficulty, dc )
		{
			if( dc == Difficulty_Edit )
				continue;

			vector<Steps*> vSteps;
			SongUtil::GetSteps( pSong, vSteps, st, dc );

			/* Delete steps that are completely identical.  This happened due to a
			 * bug in an earlier version. */
			DeleteDuplicateSteps( pSong, vSteps );

			char const *songTitle = pSong->GetDisplayFullTitle().c_str();
			CHECKPOINT_M(fmt::sprintf("Duplicate steps from %s removed.", songTitle));
			StepsUtil::SortNotesArrayByDifficulty( vSteps );
			CHECKPOINT_M(fmt::sprintf("Charts from %s sorted.", songTitle));
			for( unsigned k=1; k<vSteps.size(); k++ )
			{
				vSteps[k]->SetDifficulty( Difficulty_Edit );
				if( vSteps[k]->GetDescription() == "" )
				{
					/* "Hard Edit" */
					std::string EditName = Capitalize( DifficultyToString(dc) ) + " Edit";
					vSteps[k]->SetDescription( EditName );
				}
			}
		}

		/* XXX: Don't allow edits to have descriptions that look like regular difficulties.
		 * These are confusing, and they're ambiguous when passed to GetStepsByID. */
	}
}
/**
 * @brief Remove the initial whitespace characters.
 * @param s the string to left trim.
 * @return the trimmed string.
 */
static std::string RemoveInitialWhitespace( std::string s )
{
	size_t i = s.find_first_not_of(" \t\r\n");
	if( i != s.npos )
		s.erase( 0, i );
	return s;
}

/* This is called within TidyUpData, before autogen notes are added. */
void SongUtil::DeleteDuplicateSteps( Song *pSong, vector<Steps*> &vSteps )
{
	/* vSteps have the same StepsType and Difficulty.  Delete them if they have the
	 * same m_sDescription, m_sCredit, m_iMeter and SMNoteData. */
	for( unsigned i=0; i<vSteps.size(); i++ )
	{
		const Steps *s1 = vSteps[i];

		for( unsigned j=i+1; j<vSteps.size(); j++ )
		{
			const Steps *s2 = vSteps[j];

			if( s1->GetDescription() != s2->GetDescription() )
				continue;
			if( s1->GetCredit() != s2->GetCredit() )
				continue;
			if( s1->GetMeter() != s2->GetMeter() )
				continue;
			/* Compare, ignoring whitespace. */
			std::string sSMNoteData1;
			s1->GetSMNoteData( sSMNoteData1 );
			std::string sSMNoteData2;
			s2->GetSMNoteData( sSMNoteData2 );
			if( RemoveInitialWhitespace(sSMNoteData1) != RemoveInitialWhitespace(sSMNoteData2) )
				continue;

			auto diffString = DifficultyToString(s2->GetDifficulty());
			auto logStatement = fmt::format("Removed {0}: duplicate steps in song \"{1}\" with description \"{2}\", step author \"{3}\", and meter \"{4}\"",
				diffString, pSong->GetSongDir(), s1->GetDescription(), s1->GetCredit(), s1->GetMeter()
				);
			LOG->Trace(logStatement );

			pSong->DeleteSteps( s2, false );

			vSteps.erase(vSteps.begin()+j);
			--j;
		}
	}
}


/////////////////////////////////////
// Sorting
/////////////////////////////////////

static LocalizedString SORT_NOT_AVAILABLE( "Sort", "NotAvailable" );
static LocalizedString SORT_OTHER        ( "Sort", "Other" );

/* Just calculating GetNumTimesPlayed within the sort is pretty slow, so let's precompute
 * it.  (This could be generalized with a template.) */
static std::unordered_map<const Song*, std::string> g_mapSongSortVal;

static bool CompareSongPointersBySortValueAscending( const Song *pSong1, const Song *pSong2 )
{
	return g_mapSongSortVal[pSong1] < g_mapSongSortVal[pSong2];
}

static bool CompareSongPointersBySortValueDescending( const Song *pSong1, const Song *pSong2 )
{
	return g_mapSongSortVal[pSong1] > g_mapSongSortVal[pSong2];
}


std::string SongUtil::MakeSortString( std::string s )
{
	s = Rage::make_upper(s);

	// Make sure that non-alphanumeric strings are placed at the very end.
	if( s.size() > 0 )
	{
		if( s[0] == '.' )	// like the song ".59"
			s.erase(s.begin());
		if( (s[0] < 'A' || s[0] > 'Z') && (s[0] < '0' || s[0] > '9') )
			s = char(126) + s;
	}

	return s;
}

static bool CompareSongPointersByTitle( const Song *pSong1, const Song *pSong2 )
{
	// Prefer transliterations to full titles
	std::string s1 = pSong1->GetTranslitMainTitle();
	std::string s2 = pSong2->GetTranslitMainTitle();
	if( s1 == s2 )
	{
		s1 = pSong1->GetTranslitSubTitle();
		s2 = pSong2->GetTranslitSubTitle();
	}

	s1 = SongUtil::MakeSortString(s1);
	s2 = SongUtil::MakeSortString(s2);

	// TODO: Use a std::string comparison.
	int ret = strcmp( s1.c_str(), s2.c_str() );
	if(ret < 0) return true;
	if(ret > 0) return false;

	/* The titles are the same.  Ensure we get a consistent ordering
	 * by comparing the unique SongFilePaths. */
	Rage::ci_ascii_string a{ pSong1->GetSongFilePath().c_str() };
	Rage::ci_ascii_string b{ pSong2->GetSongFilePath().c_str() };
	return a < b;
}

void SongUtil::SortSongPointerArrayByTitle( vector<Song*> &vpSongsInOut )
{
	sort( vpSongsInOut.begin(), vpSongsInOut.end(), CompareSongPointersByTitle );
}

static bool CompareSongPointersByBPM( const Song *pSong1, const Song *pSong2 )
{
	DisplayBpms bpms1, bpms2;
	pSong1->GetDisplayBpms( bpms1 );
	pSong2->GetDisplayBpms( bpms2 );

	if( bpms1.GetMax() < bpms2.GetMax() )
		return true;
	if( bpms1.GetMax() > bpms2.GetMax() )
		return false;

	return CompareStringsAsc( pSong1->GetSongFilePath(), pSong2->GetSongFilePath() );
}

void SongUtil::SortSongPointerArrayByBPM( vector<Song*> &vpSongsInOut )
{
	sort( vpSongsInOut.begin(), vpSongsInOut.end(), CompareSongPointersByBPM );
}

static bool CompareSongPointersByLength( const Song *pSong1, const Song *pSong2 )
{
	float length1, length2;
	length1 = pSong1->m_fMusicLengthSeconds;
	length2 = pSong2->m_fMusicLengthSeconds;

	if( length1 < length2 )
		return true;
	if( length1 > length2 )
		return false;

	return CompareStringsAsc( pSong1->GetSongFilePath(), pSong2->GetSongFilePath() );
}

void SongUtil::SortSongPointerArrayByLength( vector<Song*> &vpSongsInOut )
{
	sort( vpSongsInOut.begin(), vpSongsInOut.end(), CompareSongPointersByLength );
}

void AppendOctal( int n, int digits, std::string &out )
{
	for( int p = digits-1; p >= 0; --p )
	{
		const int shift = p*3;
		int n2 = (n >> shift) & 0x7;
		out.insert( out.end(), static_cast<char>(n2+'0') );
	}
}

static bool CompDescending( const std::pair<Song *, std::string> &a, const std::pair<Song *, std::string> &b )
{
	return a.second > b.second;
}
static bool CompAscending( const std::pair<Song *, std::string> &a, const std::pair<Song *, std::string> &b )
{
	return a.second < b.second;
}

void SongUtil::SortSongPointerArrayByGrades( vector<Song*> &vpSongsInOut, bool bDescending )
{
	/* Optimize by pre-writing a string to compare, since doing
	 * GetNumNotesWithGrade inside the sort is too slow. */
	typedef std::pair< Song *, std::string > val;
	vector<val> vals;
	vals.reserve( vpSongsInOut.size() );

	for (auto *pSong: vpSongsInOut)
	{
		int iCounts[NUM_Grade];
		const Profile *pProfile = PROFILEMAN->GetMachineProfile();
		ASSERT( pProfile != nullptr );
		pProfile->GetGrades( pSong, GAMESTATE->GetCurrentStyle(GAMESTATE->GetMasterPlayerNumber())->m_StepsType, iCounts );

		std::string foo;
		foo.reserve(256);
		for( int g=Grade_Tier01; g<NUM_Grade; ++g )
			AppendOctal( iCounts[g], 3, foo );
		vals.push_back( val(pSong, foo) );
	}

	sort( vals.begin(), vals.end(), bDescending ? CompDescending : CompAscending );

	for( unsigned i = 0; i < vpSongsInOut.size(); ++i )
	{
		vpSongsInOut[i] = vals[i].first;
	}
}


void SongUtil::SortSongPointerArrayByArtist( vector<Song*> &vpSongsInOut )
{
	for (auto *song: vpSongsInOut)
	{
		g_mapSongSortVal[song] = MakeSortString( song->GetTranslitArtist() );
	}
	stable_sort( vpSongsInOut.begin(), vpSongsInOut.end(), CompareSongPointersBySortValueAscending );
}

/* This is for internal use, not display; sorting by Unicode codepoints isn't very
 * interesting for display. */
void SongUtil::SortSongPointerArrayByDisplayArtist( vector<Song*> &vpSongsInOut )
{
	for (auto *song: vpSongsInOut)
	{
		g_mapSongSortVal[song] = MakeSortString( song->GetDisplayArtist() );
	}
	stable_sort( vpSongsInOut.begin(), vpSongsInOut.end(), CompareSongPointersBySortValueAscending );
}

static int CompareSongPointersByGenre(const Song *pSong1, const Song *pSong2)
{
	return pSong1->m_sGenre < pSong2->m_sGenre;
}

void SongUtil::SortSongPointerArrayByGenre( vector<Song*> &vpSongsInOut )
{
	stable_sort( vpSongsInOut.begin(), vpSongsInOut.end(), CompareSongPointersByGenre );
}

int SongUtil::CompareSongPointersByGroup(const Song *pSong1, const Song *pSong2)
{
	return pSong1->m_sGroupName < pSong2->m_sGroupName;
}

static int CompareSongPointersByGroupAndTitle( const Song *pSong1, const Song *pSong2 )
{
	const std::string &sGroup1 = pSong1->m_sGroupName;
	const std::string &sGroup2 = pSong2->m_sGroupName;

	if( sGroup1 < sGroup2 )
		return true;
	if( sGroup1 > sGroup2 )
		return false;

	/* Same group; compare by name. */
	return CompareSongPointersByTitle( pSong1, pSong2 );
}

void SongUtil::SortSongPointerArrayByGroupAndTitle( vector<Song*> &vpSongsInOut )
{
	sort( vpSongsInOut.begin(), vpSongsInOut.end(), CompareSongPointersByGroupAndTitle );
}

void SongUtil::SortSongPointerArrayByNumPlays( vector<Song*> &vpSongsInOut, ProfileSlot slot, bool bDescending )
{
	if( !PROFILEMAN->IsPersistentProfile(slot) )
		return;	// nothing to do since we don't have data
	Profile* pProfile = PROFILEMAN->GetProfile(slot);
	SortSongPointerArrayByNumPlays( vpSongsInOut, pProfile, bDescending );
}

void SongUtil::SortSongPointerArrayByNumPlays( vector<Song*> &vpSongsInOut, const Profile* pProfile, bool bDescending )
{
	ASSERT( pProfile != nullptr );
	for (auto *song: vpSongsInOut)
	{
		g_mapSongSortVal[song] = fmt::sprintf("%9i", pProfile->GetSongNumTimesPlayed(song));
	}
	stable_sort( vpSongsInOut.begin(), vpSongsInOut.end(), bDescending ? CompareSongPointersBySortValueDescending : CompareSongPointersBySortValueAscending );
	g_mapSongSortVal.clear();
}

std::string SongUtil::GetSectionNameFromSongAndSort( const Song* pSong, SortOrder so )
{
	if( pSong == nullptr )
		return std::string();

	switch( so )
	{
	case SORT_PREFERRED:
		return SONGMAN->SongToPreferredSortSectionName( pSong );
	case SORT_GROUP:
		// guaranteed not empty
		return pSong->m_sGroupName;
	case SORT_TITLE:
	case SORT_ARTIST:
		{
			std::string s;
			switch( so )
			{
			case SORT_TITLE:	s = pSong->GetTranslitMainTitle();	break;
			case SORT_ARTIST:	s = pSong->GetTranslitArtist();		break;
			default:
				FAIL_M(fmt::sprintf("Unexpected SortOrder: %i", so));
			}
			s = MakeSortString(s);	// resulting string will be uppercase

			if (s.empty())
			{
				return "";
			}
			if (s[0] >= '0' && s[0] <= '9')
			{
				return "0-9";
			}
			if (s[0] < 'A' || s[0] > 'Z')
			{
				return SORT_OTHER.GetValue();
			}
			return Rage::head(s, 1);
		}
	case SORT_GENRE:
		if( !pSong->m_sGenre.empty() )
			return pSong->m_sGenre;
		return SORT_NOT_AVAILABLE.GetValue();
	case SORT_BPM:
		{
			if( SHOW_SECTIONS_IN_BPM_SORT )
			{
				const int iBPMGroupSize = SORT_BPM_DIVISION;
				DisplayBpms bpms;
				pSong->GetDisplayBpms( bpms );
				int iMaxBPM = (int)bpms.GetMax();
				iMaxBPM += iBPMGroupSize - (iMaxBPM%iBPMGroupSize) - 1;
				return fmt::sprintf("%03d-%03d",iMaxBPM-(iBPMGroupSize-1), iMaxBPM);
			}
			else
				return std::string();
		}
	case SORT_LENGTH:
		{
			if( SHOW_SECTIONS_IN_LENGTH_SORT )
			{
				const int iSortLengthSize = SORT_LENGTH_DIVISION;
				int iMaxLength = static_cast<int>(pSong->m_fMusicLengthSeconds);
				iMaxLength += (iSortLengthSize - (iMaxLength%iSortLengthSize) - 1);
				int iMinLength = iMaxLength - (iSortLengthSize-1);
				return fmt::sprintf("%s-%s",
					SecondsToMMSS(static_cast<float>(iMinLength)).c_str(),
					SecondsToMMSS(static_cast<float>(iMaxLength)).c_str());
			}
			else
				return std::string();
		}
	case SORT_POPULARITY:
	case SORT_RECENT:
		return std::string();
	case SORT_TOP_GRADES:
		{
			int iCounts[NUM_Grade];
			PROFILEMAN->GetMachineProfile()->GetGrades( pSong, GAMESTATE->GetCurrentStyle(GAMESTATE->GetMasterPlayerNumber())->m_StepsType, iCounts );

			for( int i=Grade_Tier01; i<NUM_Grade; ++i )
			{
				Grade g = (Grade)i;
				if( iCounts[i] > 0 )
					return fmt::sprintf( "%4s x %d", GradeToLocalizedString(g).c_str(), iCounts[i] );
			}
			return GradeToLocalizedString( Grade_NoData );
		}
	case SORT_BEGINNER_METER:
	case SORT_EASY_METER:
	case SORT_MEDIUM_METER:
	case SORT_HARD_METER:
	case SORT_CHALLENGE_METER:
	case SORT_DOUBLE_EASY_METER:
	case SORT_DOUBLE_MEDIUM_METER:
	case SORT_DOUBLE_HARD_METER:
	case SORT_DOUBLE_CHALLENGE_METER:
		{
			StepsType st;
			Difficulty dc;
			SongUtil::GetStepsTypeAndDifficultyFromSortOrder( so, st, dc );

			Steps* pSteps = GetStepsByDifficulty(pSong,st,dc);
			if( pSteps && !UNLOCKMAN->StepsIsLocked(pSong,pSteps) )
				return fmt::sprintf("%02d", pSteps->GetMeter() );
			return SORT_NOT_AVAILABLE.GetValue();
		}
	case SORT_MODE_MENU:
		return std::string();
	case SORT_ALL_COURSES:
	case SORT_NONSTOP_COURSES:
	case SORT_ONI_COURSES:
	case SORT_ENDLESS_COURSES:
	default:
		FAIL_M(fmt::sprintf("Invalid SortOrder: %i", so));
	}
}

void SongUtil::SortSongPointerArrayBySectionName( vector<Song*> &vpSongsInOut, SortOrder so )
{
	std::string sOther = SORT_OTHER.GetValue();
	for (auto *song: vpSongsInOut)
	{
		std::string val = GetSectionNameFromSongAndSort( song, so );

		// Make sure 0-9 comes first and OTHER comes last.
		if( val == "0-9" )			val = "0";
		else if( val == sOther )	val = "2";
		else						val = "1" + MakeSortString(val);

		g_mapSongSortVal[song] = val;
	}

	stable_sort( vpSongsInOut.begin(), vpSongsInOut.end(), CompareSongPointersBySortValueAscending );
	g_mapSongSortVal.clear();
}

void SongUtil::SortSongPointerArrayByStepsTypeAndMeter( vector<Song*> &vpSongsInOut, StepsType st, Difficulty dc )
{
	g_mapSongSortVal.clear();
	for (auto *song: vpSongsInOut)
	{
		// Ignore locked steps.
		const Steps* pSteps = GetClosestNotes( song, st, dc, true );
		std::string &s = g_mapSongSortVal[song];
		s = fmt::sprintf("%03d", pSteps ? pSteps->GetMeter() : 0);

		/* pSteps may not be exactly the difficulty we want; for example, we
		 * may be sorting by Hard difficulty and a song may have no Hard steps.
		 * In this case, we can end up with unintuitive ties; for example, pSteps
		 * may be Medium with a meter of 5, which will sort it among the 5-meter
		 * Hard songs.  Break the tie, by adding the difficulty to the sort as
		 * well. That way, we'll always put Medium 5s before Hard 5s. If all
		 * songs are using the preferred difficulty (dc), this will be a no-op. */
		s += fmt::sprintf( "%c", (pSteps? pSteps->GetDifficulty():0) + '0' );

		if( PREFSMAN->m_bSubSortByNumSteps )
			s += fmt::sprintf("%06.0f",pSteps ? pSteps->GetRadarValues(PLAYER_1)[RadarCategory_TapsAndHolds] : 0);
	}
	stable_sort( vpSongsInOut.begin(), vpSongsInOut.end(), CompareSongPointersBySortValueAscending );
}

void SongUtil::SortByMostRecentlyPlayedForMachine( vector<Song*> &vpSongsInOut )
{
	Profile *pProfile = PROFILEMAN->GetMachineProfile();

	for (auto *s: vpSongsInOut)
	{
		int iNumTimesPlayed = pProfile->GetSongNumTimesPlayed( s );
		std::string val = iNumTimesPlayed ? pProfile->GetSongLastPlayedDateTime(s).GetString() : "0";
		g_mapSongSortVal[s] = val;
	}

	stable_sort( vpSongsInOut.begin(), vpSongsInOut.end(), CompareSongPointersBySortValueDescending );
	g_mapSongSortVal.clear();
}

bool SongUtil::IsEditDescriptionUnique( const Song* pSong, StepsType st, const std::string &sPreferredDescription, const Steps *pExclude )
{
	for (auto *pSteps: pSong->GetAllSteps())
	{
		if( pSteps->GetDifficulty() != Difficulty_Edit )
			continue;
		if( pSteps->m_StepsType != st )
			continue;
		if( pSteps == pExclude )
			continue;
		if( pSteps->GetDescription() == sPreferredDescription )
			return false;
	}
	return true;
}

bool SongUtil::IsChartNameUnique( const Song* pSong, StepsType st, const std::string &name, const Steps *pExclude )
{
	for (auto *pSteps: pSong->GetAllSteps())
	{
		if( pSteps->m_StepsType != st )
			continue;
		if( pSteps == pExclude )
			continue;
		if( pSteps->GetChartName() == name )
			return false;
	}
	return true;
}

std::string SongUtil::MakeUniqueEditDescription( const Song *pSong, StepsType st, const std::string &sPreferredDescription )
{
	if( IsEditDescriptionUnique( pSong, st, sPreferredDescription, nullptr ) )
		return sPreferredDescription;

	std::string sTemp;

	for( int i=0; i<1000; i++ )
	{
		// make name "My Edit" -> "My Edit2"
		std::string sNum = fmt::sprintf("%d", i+1);
		sTemp = Rage::head(sPreferredDescription, MAX_STEPS_DESCRIPTION_LENGTH - sNum.size()) + sNum;

		if (IsEditDescriptionUnique(pSong, st, sTemp, nullptr))
		{
			return sTemp;
		}
	}

	// Edit limit guards should prevent this
	FAIL_M("Exceeded limit of 1000 edits per song");
}

static LocalizedString YOU_MUST_SUPPLY_NAME	( "SongUtil", "You must supply a name for your new edit." );
static LocalizedString CHART_NAME_CONFLICTS ("SongUtil", "The name you chose conflicts with another chart. Please use a different name.");
static LocalizedString EDIT_NAME_CONFLICTS	( "SongUtil", "The name you chose conflicts with another edit. Please use a different name." );
static LocalizedString CHART_NAME_CANNOT_CONTAIN ("SongUtil", "The chart name cannot contain any of the following characters: %s" );
static LocalizedString EDIT_NAME_CANNOT_CONTAIN	( "SongUtil", "The edit name cannot contain any of the following characters: %s" );

bool SongUtil::ValidateCurrentEditStepsDescription( const std::string &sAnswer, std::string &sErrorOut )
{
	Steps *pSteps = GAMESTATE->m_pCurSteps[PLAYER_1];
	Song *pSong = pSteps->m_pSong;

	ASSERT( pSteps->IsAnEdit() );

	if( sAnswer.empty() )
	{
		sErrorOut = YOU_MUST_SUPPLY_NAME.GetValue();
		return false;
	}

	static const std::string sInvalidChars = "\\/:*?\"<>|";
	if( strpbrk(sAnswer.c_str(), sInvalidChars.c_str()) != nullptr )
	{
		sErrorOut = rage_fmt_wrapper(EDIT_NAME_CANNOT_CONTAIN, sInvalidChars.c_str() );
		return false;
	}

	// Steps name must be unique for this song.
	vector<Steps*> v;
	GetSteps( pSong, v, StepsType_Invalid, Difficulty_Edit );
	for (auto *s: v)
	{
		if( pSteps == s )
			continue; // don't compare name against ourself

		if( s->GetDescription() == sAnswer )
		{
			sErrorOut = EDIT_NAME_CONFLICTS.GetValue();
			return false;
		}
	}

	return true;
}

bool SongUtil::ValidateCurrentStepsDescription( const std::string &sAnswer, std::string &sErrorOut )
{
	if( sAnswer.empty() )
		return true;

	/* Don't allow duplicate edit names within the same StepsType; edit names
	 * uniquely identify the edit. */
	Steps *pSteps = GAMESTATE->m_pCurSteps[PLAYER_1];

	// If unchanged:
	if( pSteps->GetDescription() == sAnswer )
		return true;

	if( pSteps->IsAnEdit() )
	{
		return SongUtil::ValidateCurrentEditStepsDescription( sAnswer, sErrorOut );
	}

	return true;
}

bool SongUtil::ValidateCurrentStepsChartName(const std::string &answer, std::string &error)
{
	if (answer.empty()) return true;

	static const std::string sInvalidChars = "\\/:*?\"<>|";
	if( strpbrk(answer.c_str(), sInvalidChars.c_str()) != nullptr )
	{
		error = rage_fmt_wrapper(CHART_NAME_CANNOT_CONTAIN, sInvalidChars.c_str() );
		return false;
	}

	/* Don't allow duplicate title names within the same StepsType.
	 * We need some way of identifying the unique charts. */
	Steps *pSteps = GAMESTATE->m_pCurSteps[PLAYER_1];

	if (pSteps->GetChartName() == answer) return true;

	// TODO next commit: borrow code from EditStepsDescription.
	bool result = SongUtil::IsChartNameUnique(GAMESTATE->get_curr_song(), pSteps->m_StepsType,
											  answer, pSteps);
	if (!result)
	{
		error = CHART_NAME_CONFLICTS.GetValue();
	}
	return result;
}

static LocalizedString AUTHOR_NAME_CANNOT_CONTAIN( "SongUtil", "The step author's name cannot contain any of the following characters: %s" );

bool SongUtil::ValidateCurrentStepsCredit( const std::string &sAnswer, std::string &sErrorOut )
{
	if( sAnswer.empty() )
		return true;

	Steps *pSteps = GAMESTATE->m_pCurSteps[PLAYER_1];
	// If unchanged:
	if( pSteps->GetCredit() == sAnswer )
		return true;

	// Borrow from EditDescription testing. Perhaps this should be abstracted? -Wolfman2000
	static const std::string sInvalidChars = "\\/:*?\"<>|";
	if( strpbrk(sAnswer.c_str(), sInvalidChars.c_str()) != nullptr )
	{
		sErrorOut = rage_fmt_wrapper(AUTHOR_NAME_CANNOT_CONTAIN, sInvalidChars.c_str() );
		return false;
	}

	return true;
}

static LocalizedString PREVIEW_DOES_NOT_EXIST("SongUtil", "The preview file '%s' does not exist.");
bool SongUtil::ValidateCurrentSongPreview(const std::string& answer, std::string& error)
{
	if(answer.empty())
	{ return true; }
	Song* song= GAMESTATE->get_curr_song();
	std::string real_file= song->m_PreviewFile;
	song->m_PreviewFile= answer;
	std::string path= song->GetPreviewMusicPath();
	bool valid= DoesFileExist(path);
	song->m_PreviewFile= real_file;
	if(!valid)
	{
		error= rage_fmt_wrapper(PREVIEW_DOES_NOT_EXIST, answer.c_str());
	}
	return valid;
}

static LocalizedString MUSIC_DOES_NOT_EXIST("SongUtil", "The music file '%s' does not exist.");
bool SongUtil::ValidateCurrentStepsMusic(const std::string &answer, std::string &error)
{
	if(answer.empty())
		return true;
	Steps *pSteps = GAMESTATE->m_pCurSteps[PLAYER_1];
	std::string real_file= pSteps->GetMusicFile();
	pSteps->SetMusicFile(answer);
	std::string path= pSteps->GetMusicPath();
	bool valid= DoesFileExist(path);
	pSteps->SetMusicFile(real_file);
	if(!valid)
	{
		error= rage_fmt_wrapper(MUSIC_DOES_NOT_EXIST, answer.c_str());
	}
	return valid;
}

void SongUtil::GetAllSongGenres( vector<std::string> &vsOut )
{
	std::set<std::string> genres;
	for (auto *song: SONGMAN->GetAllSongs())
	{
		if( !song->m_sGenre.empty() )
		{
			genres.insert( song->m_sGenre );
		}
	}

	for (auto const &s: genres)
	{
		vsOut.push_back( s );
	}
}

void SongUtil::FilterSongs( const SongCriteria &sc, const vector<Song*> &in,
			   vector<Song*> &out, bool doCareAboutGame )
{
	out.reserve( in.size() );
	for (auto *s: in)
	{
		if( sc.Matches( s ) && (!doCareAboutGame || IsSongPlayable(s) ) )
		{
			out.push_back( s );
		}
	}
}

void SongUtil::GetPlayableStepsTypes( const Song *pSong, std::set<StepsType> &vOut )
{
	vector<const Style*> vpPossibleStyles;
	// If AutoSetStyle, or a Style hasn't been chosen, check StepsTypes for all Styles.
	if( CommonMetrics::AUTO_SET_STYLE || GAMESTATE->GetCurrentStyle(PLAYER_INVALID) == nullptr )
		GAMEMAN->GetCompatibleStyles( GAMESTATE->m_pCurGame, GAMESTATE->GetNumPlayersEnabled(), vpPossibleStyles );
	else
		vpPossibleStyles.push_back( GAMESTATE->GetCurrentStyle(PLAYER_INVALID) );

	// Only allow OneSide Styles in Workout
	if( GAMESTATE->m_bMultiplayer )
	{
		for( int i=vpPossibleStyles.size()-1; i>=0; i-- )
		{
			const Style *pStyle = vpPossibleStyles[i];
			switch( pStyle->m_StyleType )
			{
			DEFAULT_FAIL( pStyle->m_StyleType );
			case StyleType_OnePlayerOneSide:
				continue;
			case StyleType_TwoPlayersTwoSides:
			case StyleType_OnePlayerTwoSides:
			case StyleType_TwoPlayersSharedSides:
				vpPossibleStyles.erase( vpPossibleStyles.begin() + i );
				break;
			}
		}
	}

	std::set<StepsType> vStepsTypes;
	for (auto *s: vpPossibleStyles)
	{
		vStepsTypes.insert( s->m_StepsType );
	}
	/* filter out hidden StepsTypes, and remove steps that we don't have enough
	 * stages left to play. */
	// this being const may have caused some problems... -aj
	const vector<StepsType> &vstToShow = CommonMetrics::STEPS_TYPES_TO_SHOW.GetValue();
	for (auto &st: vStepsTypes)
	{
		bool bShowThisStepsType = find( vstToShow.begin(), vstToShow.end(), st ) != vstToShow.end();

		int iNumPlayers = GAMESTATE->GetNumPlayersEnabled();
		iNumPlayers = std::max( iNumPlayers, 1 );

		bool bEnoughStages = GAMESTATE->IsAnExtraStage() ||
			GAMESTATE->GetSmallestNumStagesLeftForAnyHumanPlayer() >=
			GAMESTATE->GetNumStagesMultiplierForSong(pSong);

		if( bShowThisStepsType && bEnoughStages )
		{
			vOut.insert( st );
		}
	}
}

void SongUtil::GetPlayableSteps( const Song *pSong, vector<Steps*> &vOut )
{
	std::set<StepsType> vStepsType;
	GetPlayableStepsTypes( pSong, vStepsType );

	for (auto const &st: vStepsType)
	{
		SongUtil::GetSteps( pSong, vOut, st );
	}

	StepsUtil::RemoveLockedSteps( pSong, vOut );
	StepsUtil::SortNotesArrayByDifficulty( vOut );
	StepsUtil::SortStepsByTypeAndDifficulty( vOut );
}

bool SongUtil::IsStepsTypePlayable( Song *pSong, StepsType st )
{
	std::set<StepsType> vStepsType;
	GetPlayableStepsTypes( pSong, vStepsType );
	return vStepsType.find( st ) != vStepsType.end();
}

bool SongUtil::IsStepsPlayable( Song *pSong, Steps *pSteps )
{
	vector<Steps*> vpSteps;
	GetPlayableSteps( pSong, vpSteps );
	return find( vpSteps.begin(), vpSteps.end(), pSteps ) != vpSteps.end();
}

bool SongUtil::IsSongPlayable( Song *s )
{
	const vector<Steps*> & steps = s->GetAllSteps();
	auto isPlayable = [s](Steps *step) {
		return IsStepsPlayable(s, step);
	};
	return std::any_of(steps.begin(), steps.end(), isPlayable);
}

bool SongUtil::GetStepsTypeAndDifficultyFromSortOrder( SortOrder so, StepsType &stOut, Difficulty &dcOut )
{
	switch( so )
	{
	case SORT_BEGINNER_METER:			dcOut = Difficulty_Beginner;	break;
	case SORT_EASY_METER:				dcOut = Difficulty_Easy;		break;
	case SORT_MEDIUM_METER:			dcOut = Difficulty_Medium;		break;
	case SORT_HARD_METER:				dcOut = Difficulty_Hard;		break;
	case SORT_CHALLENGE_METER:			dcOut = Difficulty_Challenge;	break;
	case SORT_DOUBLE_EASY_METER:		dcOut = Difficulty_Easy;		break;
	case SORT_DOUBLE_MEDIUM_METER:		dcOut = Difficulty_Medium;		break;
	case SORT_DOUBLE_HARD_METER:		dcOut = Difficulty_Hard;		break;
	case SORT_DOUBLE_CHALLENGE_METER:	dcOut = Difficulty_Challenge;	break;
	default:
		return false;
	}

	switch( so )
	{
	DEFAULT_FAIL( so );
	case SORT_BEGINNER_METER:
	case SORT_EASY_METER:
	case SORT_MEDIUM_METER:
	case SORT_HARD_METER:
	case SORT_CHALLENGE_METER:
		stOut = GAMESTATE->GetCurrentStyle(GAMESTATE->GetMasterPlayerNumber())->m_StepsType;
		break;
	case SORT_DOUBLE_EASY_METER:
	case SORT_DOUBLE_MEDIUM_METER:
	case SORT_DOUBLE_HARD_METER:
	case SORT_DOUBLE_CHALLENGE_METER:
		stOut = GAMESTATE->GetCurrentStyle(GAMESTATE->GetMasterPlayerNumber())->m_StepsType;	// in case we don't find any matches below
		vector<const Style*> vpStyles;
		GAMEMAN->GetStylesForGame(GAMESTATE->m_pCurGame,vpStyles);
		for (auto const *i: vpStyles)
		{
			if( i->m_StyleType == StyleType_OnePlayerTwoSides )
			{
				// Ugly hack to ignore pump's half-double.
				bool bContainsHalf = ((std::string)i->m_szName).find("half") != std::string::npos;
				if( bContainsHalf )
					continue;
				stOut = i->m_StepsType;
				break;
			}
		}
	}

	return true;
}

//////////////////////////////////
// SongID
//////////////////////////////////

void SongID::FromSong( const Song *p )
{
	if( p )
		sDir = p->GetSongDir();
	else
		sDir = "";

	// HACK for backwards compatibility:
	// Strip off leading "/".  2005/05/21 file layer changes added a leading slash.
	if (Rage::starts_with(sDir, "/"))
	{
		sDir.erase(sDir.begin());
	}
	m_Cache.Unset();
}

Song *SongID::ToSong() const
{
	Song *pRet = nullptr;
	if( !m_Cache.Get(&pRet) )
	{
		if (!sDir.empty())
		{
			// HACK for backwards compatibility: Re-add the leading "/".
			// 2005/05/21 file layer changes added a leading slash.
			std::string sDir2 = sDir;
			if (!Rage::starts_with(sDir2, "/"))
			{
				sDir2 = "/" + sDir2;
			}
			pRet = SONGMAN->GetSongFromDir( sDir2 );
		}
		m_Cache.Set( pRet );
	}
	return pRet;
}

XNode* SongID::CreateNode() const
{
	XNode* pNode = new XNode( "Song" );
	pNode->AppendAttr( "Dir", sDir );
	return pNode;
}

void SongID::LoadFromNode( const XNode* pNode )
{
	ASSERT( pNode->GetName() == "Song" );
	pNode->GetAttrValue("Dir", sDir);
	m_Cache.Unset();
}

std::string SongID::ToString() const
{
	return sDir;
}

bool SongID::IsValid() const
{
	return ToSong() != nullptr;
}

// lua start
#include "LuaBinding.h"

namespace
{
	int GetPlayableSteps( lua_State *L )
	{
		const Song *pSong = Luna<Song>::check( L, 1, true );
		vector<Steps*> vSteps;
		SongUtil::GetPlayableSteps(pSong,vSteps);
		LuaHelpers::CreateTableFromArray<Steps*>( vSteps, L );
		return 1;
	}
	int IsStepsTypePlayable( lua_State *L )
	{
		Song *pSong = Luna<Song>::check( L, 1, true );
		StepsType st = Enum::Check<StepsType>(L, 2);
		bool b = SongUtil::IsStepsTypePlayable( pSong, st );
		LuaHelpers::Push( L, b );
		return 1;
	}
	int IsStepsPlayable( lua_State *L )
	{
		Song *pSong = Luna<Song>::check( L, 1, true );
		Steps *pSteps = Luna<Steps>::check( L, 2, true );
		bool b = SongUtil::IsStepsPlayable( pSong, pSteps );
		LuaHelpers::Push( L, b );
		return 1;
	}
	int validate_edit_description(lua_State* L)
	{
		Song* check_song= Luna<Song>::check(L, 1, true);
		StepsType stype= Enum::Check<StepsType>(L, 2);
		std::string desc= SArg(3);
		lua_pushboolean(L, SongUtil::IsEditDescriptionUnique(check_song, stype, desc, nullptr));
		return 1;
	}

	const luaL_Reg SongUtilTable[] =
	{
		LIST_METHOD( GetPlayableSteps ),
		LIST_METHOD( IsStepsTypePlayable ),
		LIST_METHOD( IsStepsPlayable ),
		LIST_METHOD(validate_edit_description),
		{ nullptr, nullptr }
	};
}

LUA_REGISTER_NAMESPACE( SongUtil )

/*
 * (c) 2001-2004 Chris Danford, Glenn Maynard
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
