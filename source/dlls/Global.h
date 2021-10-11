#include "votedefs.h"

//The client calls this once, on DLL load
//The server calls this every map change, at CWorld::Precache
void MSGlobalItemInit( );

#ifndef _WINDEF_
	typedef void *HANDLE;
	typedef HANDLE HINSTANCE;
	typedef HINSTANCE HMODULE;      /* HMODULEs can be used in place of HINSTANCEs */
#endif

enum gametype_e 
{
	GAMETYPE_ADVENTURE,		//Normal map, saving allowed
	GAMETYPE_CHALLENGE,		//Special scenario.  Can be a custom map. Only played for fun... character is never saved.
	GAMETYPE_NUM,
};

enum clperments_e
{
	CLPERMENT_INSET,
	CLPERMENT_LEFTVIEW,
	CLPERMENT_RIGHTVIEW,
	CLPERMENT_TOTAL,
};

class MSGlobals
{
public:
	static bool PKAllowed,			//Pk and stealing allowed outside of town
				PKAllowedinTown,	//PK and stealing allowed anywhere
				IsLanGame,			//Lan game
				CentralEnabled,		//MS Central Enabled -Thothie
				DevModeEnabled,		//MiB JUL2010_13 - Making it so dev-mode makes it so you can't save.
				CanCreateCharOnMap,	//Can create character on this map
				ServerSideChar,		//Whether characters are saved/loaded server-side
				InvertTownAreaPKFlag, //Town areas work opposite - PK is only allowed _in_ the townarea
				IsServer,			//This is the server (not client)
				InPrecache,			//Dll is loading and precaching (The client doesn't precache, but this prevents spawn from being called on items)
				LegalMap;			//This is an official MS map

	static gametype_e GameType;		//Regular game (allows saving) or custom map (no saving allowed)
	static vote_t CurrentVote;		//Current vote
	static msstring ServerName;		//Client - Server name
	static msstring MapName;		//Client - Map name
	static msstring DllFileName;	//Dll filename
	static msstring DllPath;		//Dll path
	static int DefaultGold;			//Starting gold
	static msstringlist DefaultWeapons;		//Default starting weapons
	static msstringlist DefaultFreeItems;	//Free items that come with a new character
	static string_i DefaultSpawnBoxModel;	//Model surrounding the player when he first spawns in
	static class IScripted *GameScript;	//Controls global events, like weather, etc
	static char Buffer[32768];		//A huge buffer for text or anything else
	static int ClEntities[CLPERMENT_TOTAL];	//Index of the permanent client entities.  Must be allocated on server & client

	static void DLLAttach( HINSTANCE hinstDLL );	//Client & Server - Called on dll load
	static void DLLDetach( );						//Client & Server - Called on new unload
	static void NewMap( );			//Client & Server - Called on new level
	static void SharedThink( );		//Client & Server - Called each frame
	static void EndMap( );			//Client & Server - Called when map ends
	static int gSoundPrecacheCount;
};

class MemMgr
{
public:
	static int m_TotalAllocations;
	static int m_HighestAllocations;
	static float m_TimePrintAllocations;
	
	static void NewAllocation( void *pAddr, size_t size );
	static void NewDeallocation( void *pAddr );
	static void Think( );
	static void EndMap( );
};