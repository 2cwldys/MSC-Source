#include "MSDLLHeaders.h"
#include "Player/player.h"
#include "Teams.h"

extern int gmsgTeamInfo;
mslist<CTeam *> CTeam::Teams;

CTeam *CTeam::CreateTeam( const char *pszName, ulong ID )
{
	if( !pszName || !pszName[0] ) return NULL;

	CTeam *pNewTeam = GetTeam( ID );

	//Team exists, use it
	if( pNewTeam ) return pNewTeam;
	
	//Create new team
	CTeam &NewTeam = *msnew CTeam;
	strncpy( NewTeam.m_TeamName, pszName, MAX_TEAMNAME_LEN );
	NewTeam.m_TeamName[MAX_TEAMNAME_LEN] = 0;
	NewTeam.m_ID = RANDOM_LONG(0,MAXLONG);			//Assign Unique ID

	Teams.add( &NewTeam );

	return &NewTeam;
}

CTeam *CTeam::GetTeam( const char *pszName )
{
	if( !pszName || !pszName[0] ) return NULL;
	foreach( i, Teams.size() )
		if( FStrEq(Teams[i]->TeamName(),pszName) ) return Teams[i];

	return NULL;
}
CTeam *CTeam::GetTeam( ulong ID )
{
	foreach( i, Teams.size() )
		if( Teams[i]->m_ID == ID ) return Teams[i];

	return NULL;
}

void CTeam :: ValidateUnits( )
{
	foreach( i, MemberList.size() )
	{
		teamunit_t &Unit = MemberList[i];
		if( (ulong)UTIL_PlayerByIndex( Unit.idx ) != Unit.ID )
		{
			//Member doesn't exist.  Remove from team
			MemberList.erase( i );
			i--;
		}
	}
}
CBasePlayer *CTeam :: GetPlayer( int idx )
{
	if( idx >= (signed)MemberList.size() ) return NULL;
	CBasePlayer *pPlayer = (CBasePlayer *)UTIL_PlayerByIndex( MemberList[idx].idx );
	if( !pPlayer || (ulong)pPlayer != MemberList[idx].ID ) return NULL;
	return pPlayer;
}


void CTeam ::AddToTeam( CBasePlayer *pPlayer )
{
	if( ExistsInList( pPlayer ) ) return;

	//pPlayer->SetTeam( this );
	teamunit_t Unit;
	Unit.idx = pPlayer->entindex();
	Unit.ID = (ulong)pPlayer;

	MemberList.add( Unit );

	//Update everyone
	for( int n = 1; n <= gpGlobals->maxClients; n++ )
	{
		CBasePlayer *pSendPlayer = (CBasePlayer *)UTIL_PlayerByIndex( n );
		if( !pSendPlayer ) continue;
	
		MSGSend_PlayerTeam( pSendPlayer, pPlayer );	
	}
}
void CTeam :: RemoveFromTeam( CBasePlayer *pPlayer )
{
	if( !pPlayer || !ExistsInList( pPlayer ) )
		return;

	foreach( i, MemberList.size() )
		if( GetPlayer( i ) == pPlayer )
			{ MemberList.erase( i ); break; }

	//Update everyone
	for( int n = 1; n <= gpGlobals->maxClients; n++ )
	{
		CBasePlayer *pSendPlayer = (CBasePlayer *)UTIL_PlayerByIndex( n );
		if( !pSendPlayer ) continue;
	
		MSGSend_PlayerTeam( pSendPlayer, pPlayer );	
	}

	//If no members are left, this team will be deleted in CHalfLifeMultiplay::Think()
}

BOOL CTeam :: ExistsInList( CBasePlayer *pPlayer )
{
	if( !pPlayer ) return FALSE;
	ValidateUnits( );
	foreach( i, MemberList.size() )
		if( MemberList[i].idx == pPlayer->entindex() && MemberList[i].ID == (ulong)pPlayer )
			return TRUE;
	return FALSE;
}
CTeam::~CTeam( )
{
	ValidateUnits( );
	foreach( i, MemberList.size() )
	{
		CBasePlayer *pPlayer = (CBasePlayer *)UTIL_PlayerByIndex( MemberList[i].idx );
		if( !pPlayer || (ulong)pPlayer != MemberList[i].ID ) continue;

		pPlayer->SetTeam( NULL );
	}	
}

void MSGSend_PlayerTeam( CBasePlayer *pSendToPlayer, CBasePlayer *pPlayer )
{
	MESSAGE_BEGIN( MSG_ONE, gmsgTeamInfo, NULL, pSendToPlayer->edict() );
		WRITE_BYTE( pPlayer->entindex() );
		WRITE_STRING( pPlayer->TeamID() );
		WRITE_LONG( pPlayer->m_pTeam ? pPlayer->m_pTeam->m_ID : 0 );
	MESSAGE_END();
}

void CTeam :: UpdateClients( )
{
}
void CTeam :: SetSolidMembers( BOOL fSolid )
{
}


BOOL SameTeam( CBaseEntity *pObject1, CBaseEntity *pObject2 )
{
	//Original:
	/*if( !pObject1->TeamID()[0] || !pObject2->TeamID()[0] ) return FALSE;
	if( pObject1->TeamID() == pObject2->TeamID() ) return TRUE;
	return FALSE;*/

	if( !pObject1->TeamID()[0] || !pObject2->TeamID()[0] ) return FALSE; 
	if( strcmp( pObject1->TeamID(), pObject2->TeamID() ) == 0) return TRUE; //MiB AUG2007a: Comparing with == is bad. Bad, Dogg. Bad. 
	return FALSE; 
}
/*int CBaseEntity::TeamID( void ) { return m_pTeam ? m_pTeam->TeamID() : -1; }
void CBaseEntity::SetTeam( CTeam *pTeam ) {
	if( !pTeam && m_pTeam ) m_pTeam->RemoveFromTeam( edict() );
	m_pTeam = pTeam; 
	if( m_pTeam ) m_pTeam->AddToTeam( edict() ); 
}*/

