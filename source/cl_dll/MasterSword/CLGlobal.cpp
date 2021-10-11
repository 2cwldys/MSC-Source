#include "../hud.h"
#include "../cl_util.h"
#include "../com_weapons.h"

#include "MSDLLHeaders.h"
#include "MSNetcodeClient.h"
//#include "CLRender.h"
#include "CLGlobal.h"
#include "CLEnv.h"
#include "../MSShared/MSCharacter.h"
#include "../MSShared/vgui_MenuDefsShared.h"
#include "Player/player.h"
#include "HUDMisc.h"
#include "HUDScript.h"
#include "vgui_HUD.h"
#include "logfile.h"

void VGUI_Think( );

msstring				MSCLGlobals::AuthID;					//My steamID
bool					MSCLGlobals::OnMyOwnListenServer;		//Did I join my own listen server?
bool					MSCLGlobals::CreatingCharacter;			//Am in the process of creating a new character?
bool					MSCLGlobals::CharPanelActive;			//Choosing a character to play with?
bool					MSCLGlobals::CamThirdPerson;			//Camera is in thirdperson?
bool					MSCLGlobals::OtherPlayers = false;		//Other players who can legally play this map are on the server
mslist<char *>			MSCLGlobals::m_Strings;					//All client-side globally allocated strings
mslist<CBaseEntity *>	MSCLGlobals::m_ClEntites;				//All client-side entities
mslist<cl_entity_t>		MSCLGlobals::m_ClModels;				//Extra models to be updated/animated client-side
cl_entity_t				MSCLGlobals::CLViewEntities[MAX_PLAYER_HANDITEMS];	//All View entity models
mslist<mstexture_t>		MSCLGlobals::Textures;					//Custom textures, to be rendered in a unique way (reflective, blended, etc)
hudcharanims_t			MSCLGlobals::DefaultHUDCharAnims;		//Anims for the char select VGUI
hudsounds_t				MSCLGlobals::DefaultHUDSounds;			//HUD sounds
hudcoords_t				MSCLGlobals::DefaultHUDCoords;			//HUD placement coordinates
int						MSCLGlobals::mapDarkenLevel = 0;		//Map's custom bloom darkening MiB 31_DEC2010

// Local version of game .dll global variables ( time, etc. )
globalvars_t	Globals; 

void MSCLGlobals::AddEnt( CBaseEntity *pEntity )
{
	pEntity->pev = msnew( entvars_t );
	pEntity->pev->pContainingEntity = (edict_t *)pEntity;
	pEntity->pev->nextthink = -1;

	m_ClEntites.add( pEntity );
}
void MSCLGlobals::RemoveEnt( CBaseEntity *pEntity, bool fDelete )
{
	foreach( e, m_ClEntites.size() )
		if( m_ClEntites[e] == pEntity )
			{ m_ClEntites.erase( e ); break; }
	if( fDelete )
	{
		::delete pEntity->pev;
		::delete pEntity;
	}
}

//Global one-time Initialization - called from CHud :: Init()
void MSCLGlobals::Initialize( )
{
	startdbg;

	dbg( "Begin" );
	//Set up g_engfuncs re-directs
	gpGlobals = &Globals;
	SetupGlobalEngFuncRedirects( );

	// Set up pointer 
	// Fill in current time
	gpGlobals->time = gEngfuncs.GetClientTime();

	//Client CVARs
	CVAR_CREATE( "ms_status_icons", "1", FCVAR_CLIENTDLL );	// Drigien MAY2008 - Shows/Hides The HUD Status Icons
	//JAN2010_11 - not optional - used to blind player sometimes
	//CVAR_CREATE( "ms_showhudimgs", "1", FCVAR_CLIENTDLL );		// Drigien MAY2008 - Shows/Hides The HUD Images
	CVAR_CREATE( "hud_classautokill", "1", FCVAR_ARCHIVE );		// controls whether or not to suicide immediately on TF class switch
	CVAR_CREATE( "hud_takesshots", "0", FCVAR_ARCHIVE );		// controls whether or not to automatically take screenshots at the end of a round
	CVAR_CREATE( "ms_hidehud", "0", FCVAR_ARCHIVE );			// Hides the HUD and viewmodel completely
	CVAR_CREATE( "ms_lildude", "1", FCVAR_ARCHIVE );			// Thothie MAR2007a - Hides the 3d Guy if set 0
	CVAR_CREATE( "ms_clgender", "0", FCVAR_CLIENTDLL );			// Thothie FEB2011_22 - Adding a cvar to store client gender
	CVAR_CREATE( "ms_xpdisplay", "0", FCVAR_ARCHIVE );		// Thothie AUG2007a - XP Display Options
	CVAR_CREATE( "ms_developer", "0", FCVAR_CLIENTDLL );		// Thothie MAR2007b - Hides client side developer messages when set to 0
	CVAR_CREATE( "ms_evthud_decaytime", "5", FCVAR_ARCHIVE );	// Time each line in the Event Console lasts before it shrinks
	CVAR_CREATE( "ms_evthud_history", "10", FCVAR_ARCHIVE );	// Max number of text lines to keep in the Event Console history
	CVAR_CREATE( "ms_evthud_size", "5", FCVAR_ARCHIVE );		// Max number of text lines shown at once
	CVAR_CREATE( "ms_evthud_bgtrans", "0", FCVAR_CLIENTDLL );	// Transparency of the background
	CVAR_CREATE( "ms_txthud_decaytime", "9", FCVAR_ARCHIVE );	// Time each line in the Event Console lasts before it shrinks
	CVAR_CREATE( "ms_txthud_history", "50", FCVAR_ARCHIVE );	// Max number of text lines to keep in the Event Console history
	CVAR_CREATE( "ms_txthud_size", "8", FCVAR_ARCHIVE );		// Max number of text lines shown at once
	CVAR_CREATE( "ms_txthud_bgtrans", "0", FCVAR_ARCHIVE );		// Transparency of the background
	CVAR_CREATE( "ms_txthud_width", "640", FCVAR_ARCHIVE );		// Width of console
	CVAR_CREATE( CVAR_HELPTIPS, "1", FCVAR_ARCHIVE/*|FCVAR_USERINFO*/ ); // Whether help tips are shown
	CVAR_CREATE( "ms_reflect", "1", FCVAR_ARCHIVE );			// Allow reflective surfaces
	CVAR_CREATE( "ms_reflect_dbg", "0", FCVAR_ARCHIVE );		// Debug reflective surfaces
	CVAR_CREATE( "ms_bloom_darken", "-1" , FCVAR_ARCHIVE );   // MiB DEC2010 - Darken bloom
	CVAR_CREATE( "ms_bloom_level", "0" , FCVAR_ARCHIVE );   // Thothie DEC2010_30 - Fix ms_bloom_level stickiness
	CVAR_CREATE( "ms_reconnect_delay", "5" , FCVAR_ARCHIVE ); // Thothie AUG2017 - Make reconnect delay adjustable client side
	CVAR_CREATE( MSCVAR_QUICKSLOT_TIMEOUT, "2.5", FCVAR_ARCHIVE ); // Timeout for the quickslots
	CVAR_CREATE( "ms_autocharge", "1", FCVAR_ARCHIVE );      // MiB MAR2012_05 - Let's you auto-charge your attack
	CVAR_CREATE( "ms_doubletapdodge", "1", FCVAR_ARCHIVE );      // MiB MAR2012_05 -Enable/Disable double tapping to dodge
	CVAR_CREATE( "ms_invtype", "1", FCVAR_ARCHIVE );      // MiB FEB2012_12 - Inventory types (added post-doc by Thothie)
	CVAR_CREATE( "ms_didbloomcheck", "0", FCVAR_ARCHIVE );      // Thothie FEB2012_12 - Boom check tracker
	//CVAR_CREATE( "r_decals", "300" , FCVAR_CLIENTDLL );   // Thothie SEP2011_07 - allow tracking/reasonable default setting of r_decals (fail, doesn't override engine setting)

	/*msstring DefaultLANID = "this_must_be_unique";
	gEngfuncs.pfnRegisterVariable( "ms_id", DefaultID, FCVAR_USERINFO|FCVAR_ARCHIVE );

	if( DefaultLANID == (const char *)gEngfuncs.pfnGetCvarString("ms_id") )
	{
		//If I'm still using the default ID, generate a random one
		char ID[8];
		foreach( i, 6 )
		{
			int CharType = RANDOM_LONG(0,2);
			int Low, High;
			switch( CharType ) {
				case 0: Low = '0'; High = '9'; break;
				case 1: Low = 'a'; High = 'z'; break;
				case 2: Low = 'A'; High = 'Z'; break;
			}
			ID[i] = RANDOM_LONG( Low, High ); ID[i+1] = 0;
		}
		gEngfuncs.pfnClientCmd( msstring("ms_id ") + ID + "\n" );
	}*/
	
	dbg( "Call InitializePlayer" );
	InitializePlayer( );

	dbg( "Call MSGlobalItemInit" );
	MSGlobalItemInit( );

	//Initialize Network
	dbg( "Call CNetCode::InitNetCode" );
	CNetCode::InitNetCode( );

	enddbg;
}
//Player initialization that happens every map
void MSCLGlobals::InitializePlayer( )
{
	if( !player.pev )
		AddEnt( &player );

	player.CreateStats( );
	player.m_CharacterState = CHARSTATE_UNLOADED;
	player.m_DisplayName = "Adventurer";
	player.PlayerHands = NULL;
	player.ClearConditions( MONSTER_OPENCONTAINER );
	player.Gear.clear( );
	player.m_HP = player.m_MP = 0;

	player.m_fGameHUDInitialized = TRUE;
	player.pbs.fMaxForwardPressTime = 0;
	player.m_Initialized = false;

	foreach( i, MAX_PLAYER_HANDITEMS )
	{
		MSCLGlobals::CLViewEntities[i].index = MSGlobals::ClEntities[ (i != 2) ? CLPERMENT_LEFTVIEW + i : CLPERMENT_LEFTVIEW];
		MSCLGlobals::CLViewEntities[i].curstate.number = MSGlobals::ClEntities[ (i != 2) ? CLPERMENT_LEFTVIEW + i : CLPERMENT_LEFTVIEW];
	}
}
//Global think - sure to be called every frame
void MSCLGlobals::Think( )
{
	startdbg;
	dbg( "Begin" );

	static float flLastThinkTime = 0;
	gpGlobals->time = gEngfuncs.GetClientTime();
	gpGlobals->frametime = gpGlobals->time - flLastThinkTime;

	//if( player.m_CharacterState == CHARSTATE_UNLOADED ) return;

	//Delete entities with |FL_KILLME
	//Count backward
	dbg( "Delete entities with |FL_KILLME" );
	for( int e = m_ClEntites.size()-1; e >= 0; e-- )
		if( FBitSet(m_ClEntites[e]->pev->flags,FL_KILLME) )
			RemoveEnt( m_ClEntites[e] );

	//Call entity Think() functions
	dbg( "Call entity Think() functions" );
	foreach( e, m_ClEntites.size() )
		if( flLastThinkTime <= m_ClEntites[e]->pev->nextthink && m_ClEntites[e]->pev->nextthink < gpGlobals->time )
			m_ClEntites[e]->Think( );

	dbg( "Update GUI" );
	VGUI_Think( );

	//Last task
	flLastThinkTime = gpGlobals->time;
	enddbg;
}
void MSCLGlobals::PrintAllEntites( )
{
	Print( "Global Items...\n" );

	int items = 0;
	foreach( e, m_ClEntites.size() )
		Print( "Item %i: %s", items++, m_ClEntites[e]->DisplayName( ) );
}
void MSCLGlobals::RemoveAllEntities( )
{
	//Delete all entites
	if( player.m_CharacterState == CHARSTATE_LOADED )
	{
		player.Killed( NULL, 0 );
		player.RemoveAllItems( false, true );
	}

	//There shouldn't be anything left here, but the player... which is skipped.
	//If any items get deleted below, conside it an error and make sure it gets
	//deleted beforehand 
	//(presumably within RemoveAllItems() somewhere)
	int killed = 0;
	foreach( e, m_ClEntites.size() )
	{
		CBaseEntity *pEntity = m_ClEntites[e];

		//Skip player
		if( pEntity == &player )
			continue;

		//unset player hands (should happen in CBasePlayer::RemoveAllItems() first...)
		if( pEntity == (CBaseEntity *)player.PlayerHands )
			player.PlayerHands = NULL;

		Print( "Cleanup Item %i: %s\n", killed++, pEntity->DisplayName( ) );
		SetBits( pEntity->pev->flags, FL_KILLME );
	}
	//logfile << "Global Cleanup: " << killed << " unreferenced entites.\r\n";
	MSCLGlobals::Think( );

	m_ClModels.clear( );	//Cleanup client-side models/sprites

	//Re-initialize player
	InitializePlayer( );
}
void MSCLGlobals::EndMap( )
{
	ChooseChar_Interface::CentralServer = false;	//Reset
}
void MSCLGlobals::DLLDetach( )
{
	player.Deactivate( );
}
int MSCLGlobals::GetLocalPlayerIndex()
{
	cl_entity_t *clPlayer = gEngfuncs.GetLocalPlayer();
	return clPlayer ? clPlayer->index : 1;
}

//Client versions of these functions
const char *EngineFunc::GetGameDir( )
{
	return gEngfuncs.pfnGetGameDirectory( );
}
void AlertMessage( ALERT_TYPE atype, char *szFmt, ... )
{
	va_list		argptr;
	static char	string[1024];
	
	va_start (argptr, szFmt);
	vsprintf (string, szFmt,argptr);
	va_end (argptr);

	ConsolePrint( "cl: " );
	ConsolePrint( string );
}
char *UTIL_VarArgs( char *format, ... ) 
{ 
	va_list		argptr;
	static char		string[1024];
	
	va_start (argptr, format);
	vsprintf (string, format,argptr);
	va_end (argptr);

	return (char *)string;	
}
Vector UTIL_VecToAngles( const Vector &vec )
{
	float rgflVecOut[3];
	AngleVectors(vec, rgflVecOut, NULL, NULL);
	return Vector(rgflVecOut);
}

byte *MSCLGlobals::LoadFile( char *pFileName, int *pLength )
{
	return gEngfuncs.COM_LoadFile( pFileName, 5, pLength );
}
string_t MSCLGlobals::AllocString( const char *pszString )
{
	if( !pszString ) return 0;

	//Does the string already exist?
	foreach( s, m_Strings.size() )
		if( FStrEq(m_Strings[s],pszString) )
			return m_Strings[s] - gpGlobals->pStringBase;	//Return existing string

	//Create new string
	uint len = strlen( pszString ) + 1;
	char *pszNewString = msnew( char[len] );
	strcpy( pszNewString, pszString );

	m_Strings.push_back( (char *)pszNewString );

	return pszNewString - gpGlobals->pStringBase;
}
/*char *MSCLGlobals::GetString( string_t sString )
{
	if( !sString ) return "(NULL)";
	return (char *)STRING( sString );
}*/
void MSCLGlobals::SetupGlobalEngFuncRedirects( void ) 
{
	// Fake functions
	g_engfuncs.pfnPrecacheModel		= stub_PrecacheModel;
	g_engfuncs.pfnPrecacheSound		= stub_PrecacheSound;
	g_engfuncs.pfnPrecacheEvent		= stub_PrecacheEvent;
	g_engfuncs.pfnNameForFunction	= stub_NameForFunction;
	g_engfuncs.pfnSetModel			= stub_SetModel;
	g_engfuncs.pfnSetClientMaxspeed = HUD_SetMaxSpeed;

	// Handled locally
	g_engfuncs.pfnPlaybackEvent		= HUD_PlaybackEvent;
	g_engfuncs.pfnAllocString		= AllocString;
	g_engfuncs.pfnAlertMessage		= AlertMessage;
	gpGlobals->pStringBase			= "(NULL)";

	// Pass through to engine
	g_engfuncs.pfnPrecacheEvent		= gEngfuncs.pfnPrecacheEvent;
	g_engfuncs.pfnRandomFloat		= gEngfuncs.pfnRandomFloat;
	g_engfuncs.pfnRandomLong		= gEngfuncs.pfnRandomLong;
	g_engfuncs.pfnLoadFileForMe		= LoadFile;
	g_engfuncs.pfnFreeFile			= gEngfuncs.COM_FreeFile;
}
//I've recieved all script files, I can now spawn
void CreateStoreMenus( );
void ShowVGUIMenu( int iMenu );

void MSCLGlobals::SpawnIntoServer( )
{
	startdbg;

	logfile << "SpawnIntoServer...";
	
	Cleanup( );	//Clean up stuff from the previous map

	dbg( "Call player.InitialSpawn" );
	player.InitialSpawn( );
	player.BeginRender( );

	dbg( "Call CreateStoreMenus" );
	CreateStoreMenus( );

	dbg( "Call MSChar_Interface::CLInit" );
	MSChar_Interface::CLInit( );

	dbg( "ShowVGUIMenu( MENU_NEWCHARACTER )" );
	ShowVGUIMenu( MENU_NEWCHARACTER );


	logfile << "DONE" << endl;

	enddbg;
}

//Cleans up stuff from the previous map
void MSCLGlobals::Cleanup( )
{
	startdbg;

	//Remove spell list
	player.m_SpellList.clear();

	//Kill the client-side entity list
	dbg( "Call CBasePlayeR::RenderCleanup" );
	player.RenderCleanup( );

	// Delete all client-side entities
	dbg( "Call MSCLGlobals::RemoveAllEntities( )" );
	RemoveAllEntities( );

	//Remove Environment Special Effects
	CRender::Cleanup( );

	enddbg;
}


BOOL WINAPI DllMain(
   HINSTANCE hinstDLL,
   DWORD fdwReason,
   LPVOID lpvReserved)
{
	if( fdwReason == DLL_PROCESS_ATTACH )
	{
		MSGlobals::DLLAttach( hinstDLL );
		logfile.DebugOpen( );
		/*#ifdef KEEP_LOG
			char cLogfile[MAX_PATH], *pGameDir;

			#ifdef DEV_BUILD
				pGameDir = "MSDev";
			#else
				pGameDir = "MS";
			#endif
			
			//MessageBox( NULL, gEngfuncs.pfnGetGameDirectory(), "", MB_OK );
			sprintf( cLogfile, "%s/log_cldll.txt", pGameDir );
			logfile.open( cLogfile );
			g_log_initialized = true;
		#endif*/
	}
	else if (fdwReason == DLL_PROCESS_DETACH) 
	{
		//if( logfile.is_open() ) logfile << __FILE__ << ":" << ((int)__LINE__) << " client.dll being unloaded" << endl;
		if( logfile.is_open() ) (((logfile << __FILE__) << " client.dll being unloaded") << endl);
		MSGlobals::EndMap( );
		MSCLGlobals::DLLDetach( );
		MSGlobals::DLLDetach( );
	}
	return TRUE;
}
