#include "MSDLLHeaders.h"
#include "Stats/Stats.h"
#include "Stats/Races.h"
#include "SVGlobals.h"
#include "logfile.h"
#include "MSNetcode.h"
#include "Global.h"
#include "Weapons/GenericItem.h"
#include "gamerules/gamerules.h"
#include "Store.h"
#include "MSCentral.h"
#include "versioncontrol.h"

ofstream modelout;
int HighestPrecache = -1;
int TotalModelPrecaches = 1;
int PreCount = 0; //Thothie OCT2007a Precache map verification

bool CSVGlobals::LogScripts = true; 
mslist<CSVGlobals::scriptlistitem_t> CSVGlobals::ScriptList[SCRIPT_TYPES];

//Thothie MAR2012_27 - duplicate precache trackers
int gModelPrecacheCount = 0;
int gSoundPrecacheCount = 0;
mslist<modelprecachelist_t> gModelPrecacheList;
mslist<modelprecachelist_t> gSoundPrecacheList;

//Master Sword CVARs
/*
cvar_t	debug			= {"ms_debug","0", FCVAR_SERVER };
cvar_t	betakey			= {"ms_key","", FCVAR_PROTECTED|FCVAR_EXTDLL };*/
cvar_t	ms_dev_mode	= {"ms_dev_mode","0",FCVAR_SERVER}; //MiB JUL2010_13 - Dev mode..
cvar_t	ms_dynamicnpc	= {"ms_dynamicnpc","", 0 };
cvar_t	msallowkickvote	= {"ms_allowkickvote","1", FCVAR_SERVER };
cvar_t	msallowtimevote	= {"ms_allowtimevote","1", FCVAR_SERVER };
cvar_t	timelimit		= {"ms_timelimit","0", FCVAR_SERVER };
cvar_t	ms_version		= {"ms_version", MS_VERSION, FCVAR_EXTDLL };
cvar_t	ms_pklevel		= {"ms_pklevel","0", FCVAR_SERVER };
//cvar_t	ms_trans_req	= {"ms_trans_req","0", FCVAR_SERVER }; //Thothie JUN2007 - max players required to activate a transition (0 = all on server) - nvm, changed method - nvm, changed method
cvar_t	ms_fxlimit		= {"ms_fxlimit","0", FCVAR_SERVER };
//cvar_t	ms_currentfx	= {"ms_currentfx","0", 0 }; //Thothie - want to make FX control total ms.dll, but can't figure how
cvar_t	ms_serverchar	= {"ms_serverchar","1", FCVAR_SERVER };
cvar_t	ms_joinreset	= {"ms_joinreset","1", FCVAR_SERVER };
cvar_t	ms_reset_if_empty	= {"ms_reset_if_empty","0", FCVAR_SERVER }; //Thothie FEB2008a - game_master restarts server, 60 seconds after last player leaves, if no new players join
cvar_t	ms_hp_limit	= {"ms_hp_limit","0", FCVAR_SERVER };
cvar_t	msvote_farm_all_day	= {"msvote_farm_all_day","0", FCVAR_SERVER }; //Thothie FEB2008a - Allow voting for the map the players are on
cvar_t	msvote_map_type	= {"msvote_map_type","all", FCVAR_SERVER }; //Thothie FEB2008a - Map vote type (current: all, and root)
cvar_t	msvote_map_enable	= {"msvote_map_enable","1", FCVAR_SERVER }; //Thothie FEB2008a - Map vote enable
cvar_t	msvote_kick_enable	= {"msvote_kick_enable","1", FCVAR_SERVER }; //Thothie FEB2008a - Vote kick at chat/console enable
cvar_t	msvote_ban_enable	= {"msvote_ban_enable","1", FCVAR_SERVER }; //Thothie FEB2008a - Vote ban at chat/console enable
cvar_t	msvote_ban_time	= {"msvote_ban_time","60", FCVAR_SERVER }; //Thothie FEB2008a - Time to ban a person voted banned
cvar_t	msvote_pvp_enable	= {"msvote_pvp_enable","60", FCVAR_SERVER }; //Thothie FEB2008a - Allow voting for PVP
cvar_t	msvote_lock_enable	= {"msvote_lock_enable","1", FCVAR_SERVER }; //Thothie JAN2011_04 - Allow voting to password lock server
cvar_t	ms_chatlog	= {"ms_chatlog","0", FCVAR_SERVER }; //Thothie FEB2008a - write chatlog
cvar_t	ms_admin_contact = {"ms_admin_contact","[none provided]", FCVAR_SERVER }; //Thothie FEB2008a - server host contact info
cvar_t	ms_ban_to_cfg = {"ms_ban_to_cfg","1", FCVAR_SERVER }; //Thothie FEB2008a - write scriptside perm bans to server/listenserver.cfg
cvar_t	ms_central_enabled	= {"ms_central_enabled","0", FCVAR_SERVER };
cvar_t	ms_fake_hp	= {"ms_fake_hp","0", FCVAR_SERVER }; //Thothie AUG2011_17 - moving Fakehp to cvar for use with triggers
cvar_t	ms_fake_players	= {"ms_fake_players","0", FCVAR_SERVER }; //Thothie DEC2013_07 - for returning false # of players for some functions during testing
cvar_t	ms_central_addr	= {"ms_central_addr","0", FCVAR_PROTECTED };
cvar_t	ms_central_pass	= {"ms_central_pass","0", FCVAR_PROTECTED };
cvar_t	ms_central_pulse= {"ms_central_pulse","6", 0 };
cvar_t	ms_central_tag  = {"ms_central_tag","[OFFLINE] ", 0 };
cvar_t	ms_central_online  = {"ms_central_online","0", 0 };
cvar_t	ms_debug_mem  = {"ms_debug_mem","0", 0 };


#ifdef DEV_BUILD
cvar_t	ms_devlog		= {"ms_devlog","1", 0 };
cvar_t	ms_allowdev		= {"ms_allowdev","1", 0 };
#endif

//------------


//SOCKET g_PingSock;
void UnBanAll( );
void InitializeGMs( );
//map checksums:
bool FileCheckSumMatch( const char *FileName, unsigned char *CheckSum );
struct mapchecksum_t
{
	const char *pszMapName;
	byte CheckSum[17];
} g_MapCheckSum[] =
{ 
	"edana", "\xb2\x50\x1a\xa0\x0\x72\x69\x52\x38\xac\x60\x4e\x37\x31\x91\x3",
	"thornlands", "\xd5\xb4\xa2\x6e\xbf\x1b\x54\x1e\x55\x94\x3d\x1d\x95\xa3\x11\x3c",
	"helena", "\x3c\xb\x85\xd1\x5b\x80\x40\xc6\x7e\x0\xb5\xfb\x9f\x4f\xf7\x5c",
	"sfor", "\xf6\x86\xda\x19\xcb\xb9\x35\xe9\xf8\x6b\x72\xf7\x97\x3f\x5a\x1",
	"calruin", "\x81\x3e\x22\x88\x16\x2a\xd9\x5d\x1d\x1f\x2d\x89\xfa\x7e\x82\x31",
	"mscave", "\xa5\xb7\x6e\x6e\x7c\xff\x4e\x54\x98\xc0\xb8\xf9\xf2\x2b\x79\x8e",
};


bool MSGlobalInit( ) //Called upon DLL Initialization
{
	//Master Sword CVARs
	CVAR_REGISTER (&ms_version);
	CVAR_REGISTER (&ms_pklevel);
	//CVAR_REGISTER (&ms_trans_req); //Thothie JUN2007 - max players required to activate a transition (0 = all on server)
	CVAR_REGISTER (&ms_fxlimit);
	//CVAR_REGISTER (&debug);
	//CVAR_REGISTER (&betakey);
	CVAR_REGISTER (&ms_dev_mode); //MiB "Mapper Scripts.rtf"
	CVAR_REGISTER (&ms_dynamicnpc);
	CVAR_REGISTER (&msallowkickvote);
	CVAR_REGISTER (&msallowtimevote);
	CVAR_REGISTER (&ms_serverchar);
	CVAR_REGISTER (&ms_joinreset);
	CVAR_REGISTER (&ms_reset_if_empty);
	CVAR_REGISTER (&ms_hp_limit);
	CVAR_REGISTER (&msvote_farm_all_day);
	CVAR_REGISTER (&msvote_map_type);
	CVAR_REGISTER (&msvote_map_enable);
	CVAR_REGISTER (&msvote_kick_enable);
	CVAR_REGISTER (&msvote_ban_enable);
	CVAR_REGISTER (&msvote_ban_time);
	CVAR_REGISTER (&msvote_pvp_enable);
	CVAR_REGISTER (&msvote_lock_enable);
	CVAR_REGISTER (&ms_chatlog);
	CVAR_REGISTER (&ms_admin_contact);
	CVAR_REGISTER (&ms_ban_to_cfg);
	CVAR_REGISTER (&ms_central_enabled);
	CVAR_REGISTER (&ms_central_addr);
	CVAR_REGISTER (&ms_central_pass);
	CVAR_REGISTER (&ms_central_pulse);
	CVAR_REGISTER (&ms_central_tag);
	CVAR_REGISTER (&ms_central_online);
	CVAR_REGISTER (&ms_debug_mem);
	CVAR_REGISTER (&ms_fake_hp); //AUG2011_17 Thothie - moving fakehp functions to cvar
	CVAR_REGISTER (&ms_fake_players); //DEC2013_07 Thothie - fake players cvar

	#ifdef DEV_BUILD
		CVAR_REGISTER( &ms_devlog );
		CVAR_REGISTER( &ms_allowdev );
	#endif

	//------------

//	GMs
	InitializeGMs( );
	g_log_initialized = true;

//	-- Initialize network for receiving characters
	logfile << "Initialize network... ";
		
	CNetCode::InitNetCode( );

	MSCentral::Startup( );

	logfile << "DONE\r\n";

	SERVER_COMMAND( "exec msstartup.cfg\n" );

	return true;
}

//Called from CWorld::Spawn() each map change
void MSWorldSpawn( )
{ 
	//Setup global variables that can't be changed during a game
	MSGlobals::PKAllowed = ms_pklevel.value > 0 ? true : false;
	//Thothie attemptitng to remove FN upload sploit (Thanx to MiB)
	MSGlobals::CentralEnabled = ms_central_enabled.value > 0 ? true  : false;
	MSGlobals::DevModeEnabled= ms_dev_mode.value > 0 && !MSGlobals::CentralEnabled ? true : false;
	//return MSGlobals::CentralEnabled && !MSGlobals::IsLanGame && MSGlobals::ServerSideChar;
	//MSGlobals::FXLimit = CVAR_GET_FLOAT("ms_fxlimit");
	MSGlobals::PKAllowedinTown = ms_pklevel.value > 1 ? true : false;
	MSGlobals::IsLanGame = CVAR_GET_FLOAT("sv_lan") ? true : false;
	MSGlobals::CanCreateCharOnMap = false;
	MSGlobals::ServerSideChar = ms_serverchar.value ? true : false;
	MSGlobals::MapName = STRING(gpGlobals->mapname);

	//Get the Central Server network name and MOTD
	MSCentral::NewLevel( );
	MSCentral::RetrieveInfo( );

	#ifndef RELEASE_LOCKDOWN
		UTIL_LogPrintf( "***************************************\n" );
		UTIL_LogPrintf( "***************************************\n" );
		UTIL_LogPrintf( "***************************************\n" );
		UTIL_LogPrintf( "********                 \n" );
		UTIL_LogPrintf( "********  DEVELOPER BUILD\n" );
		UTIL_LogPrintf( "********                 \n" );
		UTIL_LogPrintf( "***************************************\n" );
		UTIL_LogPrintf( "***************************************\n" );
		UTIL_LogPrintf( "***************************************\n" );
	#endif
	//g_SummonedMonsters = 0;

	//Map verification
	/*char cMapName[MAX_PATH];
	GET_GAME_DIR( cMapName );
	strcat( cMapName, "/maps/" );
	strcat( cMapName, STRING(gpGlobals->mapname) );
	strcat( cMapName, ".bsp" );
	//ALERT( at_console, "Check Map %s\n", cMapName );
	int MapIdx = -1;
	for( int i = 0; i < ARRAYSIZE(g_MapCheckSum); i++ )
		if( !stricmp(g_MapCheckSum[i].pszMapName,STRING(gpGlobals->mapname)) )
			{ MapIdx = i; break; }
	
	if( MapIdx < 0 || !FileCheckSumMatch( cMapName, g_MapCheckSum[MapIdx].CheckSum) )
	{
		//ALERT( at_console, "File Check Failed!!\n" );
		//exit( 0 );
		MSGlobals::LegalMap = false;
	}
	else
		MSGlobals::LegalMap = true;*/

	//UnBanAll( ); //Unban all players //Thothie FEB2008a - commenting, seeing if this breaks

	//Force items.txt to be unmodified --- Undone, servers need to be updated
	//PRECACHE_GENERIC("dlls/sc.dll");
	//ENGINE_FORCE_UNMODIFIED(force_exactfile,NULL,NULL,"dlls/sc.dll");
}

//Called every frame
void MSGameThink( )
{
	startdbg;
	dbg( "Call MSCentral::Think" );

	MSCentral::Think( );

	enddbg;
}


//Called when the map changes or server is shutdown from ServerDeactivate
//Note that ClientDisconnect is called after this, and the player is deallocated again!
#define WORLD_MAX 6000
void MSGameEnd( )
{
	startdbg;
    MSCentral::GameEnd( );

	//Thothie MAR2012_27 - clear duplicate precaches for next map
	gSoundPrecacheList.clearitems();
	gModelPrecacheList.clearitems();
	gModelPrecacheCount = 0;
	gSoundPrecacheCount = 0;

	dbg( "Call Deactivate on all Entities" );
	//Deallocate any 'extra' memory the mod allocated for any entity
	edict_t		*pEdict = g_engfuncs.pfnPEntityOfEntIndex( 0 );
	if( pEdict )
		for( int i = 0; i < gpGlobals->maxEntities; i++, pEdict++ )
		{
			if( pEdict->free ) continue;
			
			CBaseEntity *pEntity = MSInstance( pEdict );
			if( !pEntity ) continue;

			msstring dbgstr_classname = STRING(pEntity->pev->classname);

			dbg( msstring("Call Deactivate on Entity: ") + pEntity->DisplayName() + " | " + STRING(pEntity->pev->classname) );
			pEntity->Deactivate( );
			REMOVE_ENTITY( pEntity->edict() );
		}

	//Delete global items
	dbg( "Call CGenericItemMgr::DeleteItems( )" );
	CGenericItemMgr::DeleteItems( );

	//Delete global stores
	CStore::RemoveAllStores( );

	//Delete global script commands -- UNDONE: Keep these through level changes
	//CScript::Globals.Deactivate();

	dbg( "Call CRaceManager::DeleteAllRaces( )" );
	CRaceManager::DeleteAllRaces( );

	//Delete gamerules
	dbg( "delete g_pGameRules" );
	if( g_pGameRules ) { delete g_pGameRules; g_pGameRules = NULL; }

	//Thothie - I've not added anything here but there's a game error that generates here
	//MSGameEnd --> Call MSGlobals::EndMap
	dbg( "Call MSGlobals::EndMap" );
	MSGlobals::EndMap( );

	//Model precache dumpfile
	if( modelout.is_open( ) ) modelout.close( );
	HighestPrecache = -1; TotalModelPrecaches = 1;
	CSVGlobals::LogScripts = true; 

	enddbg;
}

void SendHUDMsgAll( msstring_ref Title, msstring_ref Text )
{
	foreach( p, gpGlobals->maxClients )
		if( MSInstance( INDEXENT(p+1) ) )
			((CBasePlayer *)MSInstance( INDEXENT(p+1) ))->SendHUDMsg( Title, Text );
}

//Server versions of these functions
const char *EngineFunc::GetGameDir( )
{
	static char cGameDir[MAX_PATH];
	cGameDir[0] = 0;
	GET_GAME_DIR( cGameDir );
	return cGameDir;
}


extern "C" void LogText( char *szFmt, ... )
{
	va_list		argptr;
	static char	string[1024];
	va_start (argptr, szFmt);
	vsprintf (string, szFmt,argptr);
	va_end (argptr);

	logfile << string;
}

void WRITE_FLOAT( float Float )
{
	byte *pData = (byte *)&Float;
	foreach( i, sizeof(float) )
		WRITE_BYTE( pData[i] );
}

int PRECACHE_SOUND( const char *pszSound )
{
	//Thothie tracking model precaches, avoiding duplicates
	bool thoth_nolog=false;
	foreach( i, gSoundPrecacheList.size() )
	{
		msstring thoth_yarglbarblb = pszSound;
		if ( strcmp(thoth_yarglbarblb.c_str(),gSoundPrecacheList[i].PrecacheName.c_str()) == 0 )
		{
			//logfile << "(Precache Duplicate Avoided)" << "\n"; //temporary
			//return 0;
			thoth_nolog = true;
			break;
		}
	}
	if ( !thoth_nolog )
	{
		gSoundPrecacheList.add_blank();
		gSoundPrecacheList[gSoundPrecacheCount].PrecacheName = pszSound;
		gSoundPrecacheCount++;
		logfile << "Precache_Sound(" << gSoundPrecacheCount << "):" << pszSound << "\n";
	}
	return (*g_engfuncs.pfnPrecacheSound)( (char *)pszSound );
}

int PRECACHE_MODEL( const char *pszModelname )
{
	//Thothie tracking model precaches, avoiding duplicates
	bool thoth_nolog=false;
	foreach( i, gModelPrecacheList.size() )
	{
		msstring thoth_yarglbarbl = pszModelname;
		if ( strcmp(thoth_yarglbarbl.c_str(),gModelPrecacheList[i].PrecacheName.c_str()) == 0 )
		{
			//logfile << "(Precache Duplicate Avoided)" << "\n"; //temporary
			//return 0;
			thoth_nolog = true;
			break;
		}
	}
	if ( !thoth_nolog )
	{
		gModelPrecacheList.add_blank();
		gModelPrecacheList[gModelPrecacheCount].PrecacheName = pszModelname;
		gModelPrecacheCount++;
		logfile << "Precache_Model(" << gModelPrecacheCount << "):" << pszModelname << "\n";
	}

#ifdef DEV_BUILD
	if( !ms_devlog.value )
		return (*g_engfuncs.pfnPrecacheModel)( (char *)pszModelname );

	if( !modelout.is_open() )
		modelout.open( msstring(EngineFunc::GetGameDir( )) + "/log_models.txt" );
#endif

	int LastModel = (*g_engfuncs.pfnPrecacheModel)( (char *)pszModelname );

#ifdef DEV_BUILD
	if( LastModel > HighestPrecache )
	{
		if( TotalModelPrecaches == 1 )
		{
			modelout << "Brush entities: " <<  LastModel << endl;
			modelout << "Num\tIndex in Engine" <<  endl;
		}
		char NumStr[512];
		sprintf( NumStr, "%.3i\t#%.3i - %s", TotalModelPrecaches, LastModel, pszModelname );
		modelout << NumStr << endl;
		HighestPrecache = LastModel;
		TotalModelPrecaches++;
	}
#endif

	return LastModel;
}
int ALLOC_STRING( const char *szValue )					//Master Sword - Keep track of all string allocations
{
	return (*g_engfuncs.pfnAllocString)( szValue );
}



void CSVGlobals::LogScript( msstring_ref ScriptName, CBaseEntity *pOwner, int includelevel, bool PrecacheOnly, bool Sucess )
{
#ifdef DEV_BUILD
	if( !LogScripts ||
		!ms_devlog.value )
		return;

	int									idx = 0;
	if( PrecacheOnly )					idx = 3;
	else if( !pOwner )					idx = 0;
	else if( pOwner->IsMSMonster( ) )	idx = 1;
	else								idx = 2;

	scriptlistitem_t Item;
	Item.FileName = ScriptName;
	Item.Included = (includelevel > 1) ? true : false;

	ScriptList[idx].add( Item );
#endif
}
void CSVGlobals::WriteScriptLog( )
{
#ifdef DEV_BUILD
	if( !ms_devlog.value )
		return;

	LogScripts = false;				//Stop logging scripts

	ofstream scriptout;

	scriptout.open( msstring(EngineFunc::GetGameDir( )) + "/log_scripts.txt" );
	scriptout << "Scripts loaded for " << STRING(gpGlobals->mapname) << endl;

	int Total = 0;
	foreach( i, SCRIPT_TYPES )
		Total += ScriptList[i].size();

	scriptout << "Total: " << Total << endl;

	foreach( i, SCRIPT_TYPES )
	{
		scriptout << endl;

		msstring_ref Name = "Global:";
		if( i == 1 ) Name = "Monsters:";
		else if( i == 2 ) Name = "Items:";
		else if( i == 3 ) Name = "Precache only:";

		scriptout << Name << endl;
		scriptout << "------------" << endl;
		scriptout << "Total: " << ScriptList[i].size() << endl;
		
		foreach( s, ScriptList[i].size() )
		{
			CSVGlobals::scriptlistitem_t &ScriptListItem = ScriptList[i][s];
			if( ScriptListItem.Included  )
				if( ms_devlog.value > 1 )
					scriptout << "   ";
				else
					continue;  // Admin chose not to log #included scripts

			scriptout << ScriptListItem.FileName.c_str() << endl;
		}

		ScriptList[i].clear( ); //save ourselves some memory
	}
#endif
}
