#include "statdefs.h"
#include "Races.h"
#include <stdio.h>
#include <string.h>

//Races

mslist<race_t> CRaceManager::Races;

void CRaceManager::AddRace( race_t &Race )
{
	Races.push_back( Race );
}
void CRaceManager::DeleteAllRaces( )
{
	Races.clear( );
}
race_t *CRaceManager::GetRacePtr( msstring_ref pszName )
{
	if( !pszName || strlen(pszName) < 1 )		//Null or empty string
		return NULL;

	foreach( r, Races.size() )
	{
		race_t &Race = Races[r];
		if( !stricmp(Race.Name,pszName) )
			return &Race;						//Valid race
	}
	return NULL;								//Race not found
}
bool CRaceManager::RelationshipContains( msstringlist &RaceList, msstring_ref pszTargetRace )
{
	foreach( i, RaceList.size() )
	{
		msstring_ref pszRaceName = RaceList[i];

		if( !stricmp(pszRaceName,"all") )
			return true;

		if( !stricmp(pszRaceName,pszTargetRace) )
			return true;
	}

	return false;
}
relationship_e CRaceManager::Relationship( msstring_ref pszSourceRace, msstring_ref pszTargetRace )
{
	race_t *pSourceRace = GetRacePtr( pszSourceRace );							//Source race doesn't exist
	if( !pSourceRace ) return RELATIONSHIP_NO;

	race_t *pTargetRace = GetRacePtr( pszTargetRace );							//Target race doesn't exist
 	if( !pTargetRace ) return RELATIONSHIP_NO;
	
	//if( !stricmp(pszSourceRace,pszTargetRace) )									//Same race, assume we're friends
	//	return RELATIONSHIP_AL;

	if( RelationshipContains(pSourceRace->Allies,pTargetRace->Name ) )	//Found Friend
		return RELATIONSHIP_AL;								
	if( RelationshipContains(pSourceRace->Enemies,pTargetRace->Name) )	//Found Enemy
 		return RELATIONSHIP_HT;								

	return RELATIONSHIP_NO;														//No relationship
}