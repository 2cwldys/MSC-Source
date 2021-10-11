/*
	MSCharacter.cpp - Shared character definitions
*/

#include "inc_weapondefs.h"
#include "Stats/Stats.h"
#ifdef VALVE_DLL
	#include "../MSShared/Global.h"
	#include "MSCentral.h"
#else
	#include "../cl_dll/inc_huditem.h"
	#include "../cl_dll/MasterSword/CLGlobal.h"
	#include "../cl_dll/vgui_ScorePanel.h"
#endif
#include "logfile.h"
#include "MSCharacter.h"
#include "../MSShared/Magic.h"
#include "../MSShared/Script.h"

#ifndef _WIN32
	#include "sys/io.h"
#endif
#include <direct.h> //for mkdir()

//Vector	MSChar_Interface::LastGoodPos, 
//		MSChar_Interface::LastGoodAng;

void ReplaceChar( char *pString, char org, char dest ); 


bool IsValidCharVersion( int Version )
{
	//logfile << "Check char version: " << Version << "\r\n";
	if( Version == SAVECHAR_VERSION || Version == SAVECHAR_LASTVERSION )
		return true;

	if( Version == SAVECHAR_DEV_VERSION || Version == SAVECHAR_REL_VERSION )	//hack to get somebody's char back
		return true;
		
	#ifndef RELEASE_LOCKDOWN
		if( Version == SAVECHAR_REL_VERSION )
			return true;
	#endif
	return false;
}

const char *GetSaveFileName( int iCharacter, CBasePlayer *pPlayer ) 
{
	static char cFileName[MAX_PATH];

	#ifdef VALVE_DLL
		//Server
		msstring FileID;
		//if( MSGlobals::IsLanGame ) pszFileID = LanID = msstring("LAN_") + STRING(pPlayer->DisplayName) + g_engfuncs.pfnInfoKeyValue( g_engfuncs.pfnGetInfoKeyBuffer( pPlayer->edict() ), "ms_id" );
		if( MSGlobals::IsLanGame ) FileID = msstring("LAN_") + g_engfuncs.pfnInfoKeyValue( g_engfuncs.pfnGetInfoKeyBuffer( pPlayer->edict() ), "name" );
		else FileID = GETPLAYERAUTHID( pPlayer->edict() );
		msstring Prefix = MSCentral::Enabled( ) ? CENTRAL_FILEPREFIX : "";

		//Thothie MAR2010_08 emergency work around
		//iCharacter = pPlayer->m_CharacterNum;
		//Print("CHAR_SAVE_DEBUG [MSCharacter]: %i vs %i\n",iCharacter+1,pPlayer->m_CharacterNum);
        
		sprintf( cFileName, "%s/save/%s%s_%i.char", EngineFunc::GetGameDir(), Prefix.c_str(), FileID.c_str(), iCharacter+1 );
		ReplaceChar( cFileName, ':', '-' );
	#else
		//Client
		//Print("CHAR_CLIENT [MSCharacter]: %i\n",iCharacter+1); //MAR2010_08
		msstring Prefix = !MSGlobals::ServerSideChar ? "cl_" : "";
		sprintf( cFileName, "%s/save/%ssave%i.char", EngineFunc::GetGameDir(), Prefix.c_str(), iCharacter+1 );
	#endif

	return cFileName;
}

#ifdef VALVE_DLL
const char *GetSaveFileName( int iCharacter, const char *AuthID ) 
{
	static char cFileName[MAX_PATH];

	//Server
	Print("CHAR_SAVE_DEBUG [GetSaveFileName]: %s %#i\n",cFileName,iCharacter+1);
	sprintf( cFileName, "%s/save/%s_%i.char", EngineFunc::GetGameDir(), AuthID, iCharacter+1 );
	ReplaceChar( cFileName, ':', '-' );

	return cFileName;
}
#endif
bool DeleteChar( int iCharacter )
{
	const char *pszCharFileName = GetSaveFileName( iCharacter );
	int ret = remove( pszCharFileName );		//Delete savefile
	remove( BACKUP_NAME(pszCharFileName) );		//Delete backup
	return (!ret) ? true : false;
}


savedata_t *GetCharInfo( const char *pszFileName, msstringlist &VisitedMaps )
{
	CPlayer_DataBuffer gFile;
	static savedata_t Data;
	bool fCharLoaded = gFile.ReadFromFile( pszFileName, "rb" , true );
	if( fCharLoaded )
	{
		fCharLoaded = gFile.Decrypt( ENCRYPTION_TYPE );
		if( fCharLoaded )
		{
			memset( &Data, 0, sizeof(savedata_t) );
			gFile.Read( &Data, sizeof(savedata_t) );

			if( IsValidCharVersion(Data.Version) )
			{
				//Also read the visited maps -- This is used to determine whether you can spawn on this map
				//The visited maps must come DIRECTLY after the main data
				int Maps = 0;
				gFile.ReadInt( Maps );												//[INT]

				char cTemp[256];
				VisitedMaps.clear( );
				foreach( m, Maps )
				{
					gFile.ReadString( cTemp );										//[STRING]
					VisitedMaps.add( cTemp );
				}
			}
			else fCharLoaded = false;
		
			gFile.Close( );
		}
	}

	if( fCharLoaded ) return &Data;
	return NULL;
}

bool MSChar_Interface::ReadCharData( void *pData, ulong Size, chardata_t *CharData )
{
	return CharData->ReadData( pData, Size );
}

bool chardata_t::ReadData( void *pData, ulong Size )
{
	bool ValidVersion = false;

	startdbg;
	dbg( "Begin" );


	CPlayer_DataBuffer m_File( Size );
	m_File.Write( pData, Size );
	if( !m_File.Decrypt( ENCRYPTION_TYPE ) )
		return false;

	byte DataID = CHARDATA_UNKNOWN;

	do
	{
		m_File.ReadByte( DataID );
		if( DataID >= CHARDATA_UNKNOWN )
		{
			ValidVersion = false;
			break;
		}

		if( ReadHeader1( DataID, m_File ) ) ValidVersion = true;
		#ifdef VALVE_DLL
			ReadMaps1( DataID, m_File );
			ReadSkills1( DataID, m_File );
			ReadSpells1( DataID, m_File );
			ReadItems1( DataID, m_File );
			ReadStorageItems1( DataID, m_File );
			ReadCompanions1( DataID, m_File );
			ReadHelpTips1( DataID, m_File );
			ReadQuests1( DataID, m_File );
			ReadQuickSlots1( DataID, m_File );
		#else
			//Only read the header data from the client
			//Just enough to get the version, character's name and maps
			break;
		#endif
	}
	while( !m_File.Eof( ) );

	m_File.Close( );
	enddbg;

	return ValidVersion;
}
bool chardata_t::ReadHeader1( byte DataID, CPlayer_DataBuffer &m_File )
{
	if( DataID == CHARDATA_HEADER1 )
	{
		m_File.Read( this, sizeof(savedata_t) );		//[HEADER}

		if( !IsValidCharVersion(Version) )
			return false;

		return true;
	}
	return false;
}


jointype_e MSChar_Interface::CanJoinThisMap( savedata_t &Data, msstringlist &VisitedMaps )
{
	//phase this function out.  Use the one below
	jointype_e JoinType = JN_NOTALLOWED;
	if( MSGlobals::CanCreateCharOnMap ) JoinType = JN_STARTMAP;			//Can create a character on this map
	else if( !stricmp(Data.MapName,MSGlobals::MapName) ||				//Already in this map Or trying to 
		!stricmp(Data.NextMap,MSGlobals::MapName) )						//transition to this map
			JoinType = JN_TRAVEL;
	else if( HasVisited( MSGlobals::MapName, VisitedMaps ) &&
			 GetOtherPlayerTransition( NULL ) ) JoinType = JN_STARTMAP;	// Already visited this map before and at least 
																		//1 other player is currently playing it
	else if( Data.IsElite )	JoinType = JN_ELITE;						//GM.  You can always join any may
	
	if( strcmp( Data.Name, "LOAD_ERROR-RECONNECT" ) == 0) {JoinType = JN_NOTALLOWED;}
	if( strcmp( Data.Name, "LOAD_FAILED-RECONNECT" ) == 0) {JoinType = JN_NOTALLOWED;}
	//If this character is one that shouldn't be loaded
	//then don't allow it to be loaded.  ---MiB---

	return JoinType;
}
jointype_e MSChar_Interface::CanJoinThisMap( charinfo_t &CharData, msstringlist &VisitedMaps )
{
	jointype_e JoinType = JN_NOTALLOWED;
	if( MSGlobals::CanCreateCharOnMap ) JoinType = JN_STARTMAP;			//Can create a character on this map
	else if( !stricmp(CharData.MapName,MSGlobals::MapName) ||			//Already in this map Or trying to 
		!stricmp(CharData.NextMap,MSGlobals::MapName) )					//transition to this map
			JoinType = JN_TRAVEL;
	else if( HasVisited( MSGlobals::MapName, VisitedMaps ) &&
			 GetOtherPlayerTransition( NULL ) ) JoinType = JN_STARTMAP;	// Already visited this map before and at least 
																		//1 other player is currently playing it
	else if( CharData.IsElite )	JoinType = JN_ELITE;					//GM.  You can always join any may
	
	return JoinType;
}

bool MSChar_Interface::HasVisited( msstring_ref MapName, msstringlist &VisitedMaps )
{
	foreach( m, VisitedMaps.size() )
		if( VisitedMaps[m] == MSGlobals::MapName )
			return true;
	return false;
}

void MSChar_Interface::CreateSaveDir( )
{
	mkdir( MSGlobals::DllPath + "/../save" );
}

#define RWVar Write
#define RWByte WriteByte
#define RWString WriteString
#define RWShort WriteShort

/*void WriteItem( CDataBuffer &gFile, CGenericItem *pItem )
{
	if( !pItem || FBitSet(pItem->MSProperties(), ITEM_SPELL) )
	{ 
		//No item or item is a spell
		gFile.RWByte( 0 );
		return;
	}

	gFile.RWString( (string_t)pItem->ItemName );			//[STRING] Item Type

	gFile.RWShort( pItem->MSProperties( ) );	//[SHORT] Item Properties
	gFile.RWShort( pItem->m_Location );			//[SHORT] Item Location
	gFile.RWByte( pItem->m_Hand );				//[BYTE] Item Hand

	if( FBitSet( pItem->MSProperties( ), ITEM_PERISHABLE ) ||
		FBitSet( pItem->MSProperties( ), ITEM_DRINKABLE ) )
	{
		gFile.RWShort( pItem->Quality );		//[SHORT] Current quality
		gFile.RWShort( pItem->MaxQuality );		//[SHORT] Max quality
		//Print( "Write Shield Quality: %i\n", pItem->Quality );
	}

	if(FBitSet( pItem->MSProperties( ), ITEM_GROUPABLE ))
		gFile.RWShort( pItem->iQuantity );		//[SHORT] Grouped amount

	//Writing contained items should be the *LAST* thing you do
	if(FBitSet( pItem->MSProperties( ), ITEM_CONTAINER ))
	{
		gFile.RWShort( pItem->Container_ItemCount( ) ); //[SHORT] Container Item Total

		foreach( i, pItem->Container_ItemCount( ) )
			WriteItem( gFile, pItem->Container_GetItem( i ) );
	}
}*/

void WriteItem( CPlayer_DataBuffer &gFile, genericitem_full_t &Item )
{
	if( FBitSet(Item.Properties, ITEM_SPELL) )
	{ 
		//No item or item is a spell
		gFile.RWByte( 0 );
		return;
	}

	gFile.RWString( Item.Name );				//[STRING] Item Type

	gFile.RWShort( Item.Properties );			//[SHORT] Item Properties
	gFile.RWShort( Item.Location );				//[SHORT] Item Location
	gFile.RWByte( Item.Hand );					//[BYTE] Item Hand
	gFile.WriteInt( Item.ID );					//[INT] Item ID at last save (used by quickslots to identify this item)

	if( FBitSet( Item.Properties, ITEM_PERISHABLE ) ||
		FBitSet( Item.Properties, ITEM_DRINKABLE ) )
	{
		gFile.RWShort( Item.Quality );			//[SHORT] Current quality
		gFile.RWShort( Item.MaxQuality );		//[SHORT] Max quality
		//Print( "Write Shield Quality: %i\n", pItem->Quality );
	}

	if(FBitSet( Item.Properties, ITEM_GROUPABLE ))
		gFile.RWShort( Item.Quantity );			//[SHORT] Grouped amount

	//Writing contained items should be the *LAST* thing you do
	if(FBitSet( Item.Properties, ITEM_CONTAINER ))
	{
		gFile.RWShort( Item.ContainerItems.size() ); //[SHORT] Container Item Total

		foreach( i, Item.ContainerItems.size() )
			WriteItem( gFile,Item.ContainerItems[i] );
	}
}