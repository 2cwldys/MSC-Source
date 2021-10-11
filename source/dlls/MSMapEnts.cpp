#include "MSDLLHeaders.h"
#include "Player.h"
#include "SVGlobals.h"
#include "../MSShared/Global.h"
#include "../MSShared/MSCharacter.h"
#include "logfile.h"

class CCycler : public CBaseMonster
{
public:
	void GenericCyclerSpawn(char *szModel, Vector vecMin, Vector vecMax);
	virtual int	ObjectCaps( void ) { return (CBaseEntity :: ObjectCaps() | FCAP_IMPULSE_USE); }
	int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType );
	void Spawn( void );
	void Think( void );
	//void Pain( float flDamage );
	void Use ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	// Don't treat as a live target
	virtual BOOL IsAlive( void ) { return FALSE; }

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	int			m_animate;
};

TYPEDESCRIPTION	CCycler::m_SaveData[] = 
{
	DEFINE_FIELD( CCycler, m_animate, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CCycler, CBaseMonster );


//
// we should get rid of all the other cyclers and replace them with this.
//
class CGenericCycler : public CCycler
{
public:
	void Spawn( void ) { GenericCyclerSpawn( (char *)STRING(pev->model), Vector(-16, -16, 0), Vector(16, 16, 72) ); }
};
LINK_ENTITY_TO_CLASS( cycler, CGenericCycler );



// Cycler member functions

void CCycler :: GenericCyclerSpawn(char *szModel, Vector vecMin, Vector vecMax)
{
	if (!szModel || !*szModel)
	{
		ALERT( at_error, "Cycler (%.0f %.0f %0.f) Missing model name!\n", pev->origin.x, pev->origin.y, pev->origin.z );
		REMOVE_ENTITY(ENT(pev));
		return;
	}

	byte *pMemFile;
	int iFileSize;

	//hack to fix maps that wont load because they have old cyclers pointing to missing models
	char c[256];
	GET_GAME_DIR( c );
	pMemFile = LOAD_FILE_FOR_ME( UTIL_VarArgs("%s/%s", c, szModel), &iFileSize );
	if( pMemFile )
		FREE_FILE( pMemFile );
	else
	{
		ALERT( at_error, "Cycler (%.0f %.0f %0.f) Model: \'%s\' NOT FOUND!\n", pev->origin.x, pev->origin.y, pev->origin.z, szModel );
		REMOVE_ENTITY(ENT(pev));
		return;
	}


	pev->classname		= MAKE_STRING("cycler");
	PRECACHE_MODEL( szModel );
	SET_MODEL(ENT(pev),	szModel);

	CCycler::Spawn( );

	UTIL_SetSize(pev, vecMin, vecMax);
}


void CCycler :: Spawn( )
{
	InitBoneControllers();
	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_NONE;
	pev->takedamage		= DAMAGE_YES;
	pev->effects		= 0;
	pev->health			= 80000;// no cycler should die
	pev->yaw_speed		= 5;
	pev->ideal_yaw		= pev->angles.y;
	ChangeYaw( 360 );
	
	m_flFrameRate		= 75;
	m_flGroundSpeed		= 0;

	pev->nextthink		+= 1.0;

	ResetSequenceInfo( );

	if (pev->sequence != 0 || pev->frame != 0)
	{
		m_animate = 0;
		pev->framerate = 0;
	}
	else
	{
		m_animate = 1;
	}
}




//
// cycler think
//
void CCycler :: Think( void )
{
	pev->nextthink = gpGlobals->time + 0.1;

	if (m_animate)
	{
		StudioFrameAdvance ( );
	}
	if (m_fSequenceFinished && !m_fSequenceLoops)
	{
		// ResetSequenceInfo();
		// hack to avoid reloading model every frame
		pev->animtime = gpGlobals->time;
		pev->framerate = 1.0;
		m_fSequenceFinished = FALSE;
		m_flLastEventCheck = gpGlobals->time;
		pev->frame = 0;
		if (!m_animate)
			pev->framerate = 0.0;	// FIX: don't reset framerate
	}
}

//
// CyclerUse - starts a rotation trend
//
void CCycler :: Use ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	m_animate = !m_animate;
	if (m_animate)
		pev->framerate = 1.0;
	else
		pev->framerate = 0.0;
}

//
// CyclerPain , changes sequences when shot
//
//void CCycler :: Pain( float flDamage )
int CCycler :: TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType )
{
	if (m_animate)
	{
		pev->sequence++;

		ResetSequenceInfo( );

		if (m_flFrameRate == 0.0)
		{
			pev->sequence = 0;
			ResetSequenceInfo( );
		}
		pev->frame = 0;
	}
	else
	{
		pev->framerate = 1.0;
		StudioFrameAdvance ( 0.1 );
		pev->framerate = 0;
		ALERT( at_console, "sequence: %d, frame %.0f\n", pev->sequence, pev->frame );
	}

	return 0;
}



class CCyclerSprite : public CBaseEntity
{
public:
	void Spawn( void );
	void Think( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	virtual int	ObjectCaps( void ) { return (CBaseEntity :: ObjectCaps() | FCAP_DONT_SAVE | FCAP_IMPULSE_USE); }
	virtual int	TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType );
	void	Animate( float frames );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	inline int		ShouldAnimate( void ) { return m_animate && m_maxFrame > 1.0; }
	int			m_animate;
	float		m_lastTime;
	float		m_maxFrame;
};

LINK_ENTITY_TO_CLASS( cycler_sprite, CCyclerSprite );

TYPEDESCRIPTION	CCyclerSprite::m_SaveData[] = 
{
	DEFINE_FIELD( CCyclerSprite, m_animate, FIELD_INTEGER ),
	DEFINE_FIELD( CCyclerSprite, m_lastTime, FIELD_TIME ),
	DEFINE_FIELD( CCyclerSprite, m_maxFrame, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( CCyclerSprite, CBaseEntity );


void CCyclerSprite::Spawn( void )
{
	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_NONE;
	pev->takedamage		= DAMAGE_YES;
	pev->effects		= 0;

	pev->frame			= 0;
	pev->nextthink		= gpGlobals->time + 0.1;
	m_animate			= 1;
	m_lastTime			= gpGlobals->time;

	PRECACHE_MODEL( (char *)STRING(pev->model) );
	SET_MODEL( ENT(pev), STRING(pev->model) );

	m_maxFrame = (float) MODEL_FRAMES( pev->modelindex ) - 1;
}


void CCyclerSprite::Think( void )
{
	if ( ShouldAnimate() )
		Animate( pev->framerate * (gpGlobals->time - m_lastTime) );

	pev->nextthink		= gpGlobals->time + 0.1;
	m_lastTime = gpGlobals->time;
}


void CCyclerSprite::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	m_animate = !m_animate;
	ALERT( at_console, "Sprite: %s\n", STRING(pev->model) );
}


int	CCyclerSprite::TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType )
{
	if ( m_maxFrame > 1.0 )
	{
		Animate( 1.0 );
	}
	return 1;
}

void CCyclerSprite::Animate( float frames )
{ 
	pev->frame += frames;
	if ( m_maxFrame > 0 )
		pev->frame = fmod( pev->frame, m_maxFrame );
}

//Thothie OCT2007a - generate random events
//Wait - multi_manager has a random property? :|
/*class CRandEvent : public CPointEntity
{
public:
	msstring thoth_event_prefix;
	void Spawn( void )
	{

	}
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
	{
		SUB_UseTargets( this, triggerType, 0 );

		if ( pev->spawnflags & SF_RELAY_FIREONCE )
			UTIL_Remove( this );
	}
	void KeyValue( KeyValueData *pkvd )
	{
		if (FStrEq(pkvd->szKeyName, "prefix"))
		{
			thoth_event_prefix = pkvd->szValue;
			pkvd->fHandled = TRUE;
		}
		else if (FStrEq(pkvd->szKeyName, "nevents")) 
		{
			//Thothie - AUG2007a - adding option to only spawn monster if # players present
			thoth_nevents = (atoi(pkvd->szValue));
			pkvd->fHandled = TRUE;
		}
	}
}*/


//
// For Ewok.
//
class CStaticModel : public CBaseEntity
{
public:
	void Spawn( void ) 
	{ 
		//Save these, since they change when the model is loaded
		Vector vMins = pev->mins, vMaxs = pev->maxs;
		int body = pev->body; //Thothie attempting to fix env_model body not settings
		int skin = pev->skin; //" " skin
		float scale = pev->scale; //FEB2010_23 - scale for env_model

		//Set model
		char *pszModel = (char *)STRING(pev->model);
		if( !pszModel || !pszModel[0] )
		{
			ALERT( at_console, "Error: env_model - no model specified" );
			return;
		}
		PRECACHE_MODEL( pszModel );
		SET_MODEL( edict(), pszModel );
		//Set solidity
		if( pev->dmg )
		{
			pev->solid = SOLID_SLIDEBOX;
			UTIL_SetSize( pev, vMins, vMaxs );
		}
		pev->body = body; //Thothie (see above)
		pev->skin = skin; //Thothie (see above)
	}
};
LINK_ENTITY_TO_CLASS( env_model, CStaticModel );

// MP3 Playback

#define SF_LOOP     1
#define SF_REMOVE_ON_FIRE 2

class CTargetMP3Audio : public CPointEntity
{
public:
     void Spawn( void );

     void Use( CBaseEntity *pActivator, CBaseEntity *pCaller,
          USE_TYPE useType, float value );

     BOOL m_bPlaying;
};

LINK_ENTITY_TO_CLASS( trigger_mp3audio, CTargetMP3Audio );

void CTargetMP3Audio :: Spawn( void )
{
     pev->solid = SOLID_NOT;
     pev->movetype = MOVETYPE_NONE;

     m_bPlaying = FALSE; // start out not playing
}

void CTargetMP3Audio::Use( CBaseEntity *pActivator, CBaseEntity *pCaller,
     USE_TYPE useType, float value )
{
     char command[64];

     if (!pActivator->IsPlayer()) // activator should be a player
          return;

     if (!m_bPlaying) // if we're not playing, start playing!
          m_bPlaying = TRUE;
     else
     {     // if we're already playing, stop the mp3
          m_bPlaying = FALSE;
          CLIENT_COMMAND(pActivator->edict(), "mp3 stop\n");
          return;
     }

     // issue the play/loop command
	 //Thothie AUG2007a - removing music/ dependancy, so you can play MP3's in valve/media/ folder
	 msstring th_test_string = STRING(pev->message);
	 if ( th_test_string.contains("/") ) sprintf(command, "mp3 %s %s\n", FBitSet(pev->spawnflags, SF_LOOP) ? "loop" : "play", STRING(pev->message));
	 else sprintf(command, "mp3 %s music/%s\n", FBitSet(pev->spawnflags, SF_LOOP) ? "loop" : "play", STRING(pev->message));

     CLIENT_COMMAND(pActivator->edict(), command); //thothie - not sure how this works, might be useful to know

     // remove if set
     if (FBitSet(pev->spawnflags, SF_REMOVE_ON_FIRE))
          UTIL_Remove(this);
}

class CMSMusic : public CPointEntity
{
public:
	mslist<song_t> m_Songs;
	msstring main_song;
	float main_song_length;
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
	{
		//if( !pActivator->IsPlayer() )
		//	return;
		//CBasePlayer *pPlayer = (CBasePlayer *)pActivator;
		if ( main_song.len() > 0 )
		{
			if ( m_Songs.size() && m_Songs[0].Name != main_song )
			{
				ALERT( at_console,"DEBUG: msarea_music - setting main song %s as idx 0.\n", main_song.c_str() );
				m_Songs[0].Name = main_song;
				m_Songs[0].Length = main_song_length;
			}
			else
			{
				ALERT( at_console,"DEBUG: msarea_music - adding main song %s.\n", main_song.c_str() );
				song_t Song;
				Song.Name = main_song;
				Song.Length = main_song_length;
				m_Songs.add(Song);
			}
		}

		static msstringlist Params;
		Params.clearitems( );
		Params.add( "0" ); //gm_set_idle_music ignores first var, in case it comes from scriptevent
		Params.add( m_Songs[0].Name.c_str() );
		Params.add( (m_Songs[0].Length>0) ? FloatToString(m_Songs[0].Length/60) : "0" );
		Params.add( "3" );

		CBaseEntity *pGameMasterEnt = UTIL_FindEntityByString( NULL, "netname", msstring("�") + "game_master" );
		IScripted *pGMScript = pGameMasterEnt->GetScripted();
		pGMScript->CallScriptEvent("gm_set_idle_music",&Params);

		//old way
		/*
		if( m_Songs.size() )
		{	
			for( int i = 1; i <= gpGlobals->maxClients; i++ )
			{
				//Thothie - attempting to make mstrig_music play music on ALL players
				//- so we can have event driven music
				CBasePlayer *pPlayer = (CBasePlayer *)UTIL_PlayerByIndex( i );
				if ( pPlayer ) pPlayer->Music_Play( m_Songs, this );
			}
			return;
		}
		
		if ( !m_Songs.size() )
		{
			for( int i = 1; i <= gpGlobals->maxClients; i++ )
			{
				//Thothie - attempting to make mstrig_music play music on ALL players
				//- so we can have event driven music
				CBasePlayer *pPlayer = (CBasePlayer *)UTIL_PlayerByIndex( i );
				if ( pPlayer ) pPlayer->Music_Stop( this );
			}
		}
		*/
	 }
	void Deactivate( ) { m_Songs.clear( ); }

	void KeyValue( KeyValueData *pkvd )
	{
		//JAN2013_08 Thothie - Noticed msarea_musics were adding "zhlt_invisible" as a song. :O
		msstring sTemp = pkvd->szKeyName;
		if ( sTemp.contains(".mp3") || sTemp.contains(".midi") )
		{
			song_t Song;
			Song.Name = pkvd->szKeyName;
			Song.Length = UTIL_StringToSecs(pkvd->szValue);
			m_Songs.add( Song );
		}
		else if (FStrEq(pkvd->szKeyName, "song")) //NOV2014_12 - Thothie - making this a bit more intuitive to use via smartedit
		{
			main_song = pkvd->szValue;
			pkvd->fHandled = TRUE;
		}
		else if (FStrEq(pkvd->szKeyName, "songlength"))
		{
			main_song_length = UTIL_StringToSecs(pkvd->szValue);
			pkvd->fHandled = TRUE;
		}
		else CBaseEntity::KeyValue( pkvd );
	}

};
LINK_ENTITY_TO_CLASS( mstrig_music, CMSMusic );

//Thothie's half-assed game_text fix
class CGameText : public CPointEntity
{
public:
	inline	void	MessageSet( const char *pMessage ) { pev->message = ALLOC_STRING(pMessage); }
	inline	const char *MessageGet( void )	{ return STRING(pev->message); }
	msstring ms_npcname;
	bool ms_sayasnpc;

	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
	{
		//for( int i = 1; i <= gpGlobals->maxClients; i++ )
		//{
			//CBasePlayer *pOtherPlayer = (CBasePlayer *)UTIL_PlayerByIndex( i );
		if ( !ms_sayasnpc )
		{
			SendHUDMsgAll( (ms_npcname.len()>0) ? ms_npcname.c_str() : "   ", MessageGet() );
		}
		else
		{
			static msstringlist Params;
			Params.clearitems( );
			Params.add( ms_npcname.c_str() );
			Params.add( MessageGet() );

			CBaseEntity *pGameMasterEnt = UTIL_FindEntityByString( NULL, "netname", msstring("�") + "game_master" );
			IScripted *pGMScript = pGameMasterEnt->GetScripted();
			pGMScript->CallScriptEvent("gm_ms_text",&Params);
		}

		//UTIL_SayTextAll( MessageGet(), pActivator );
		//UTIL_HudMessageAll( thoth_textdef, MessageGet() );
		//}
	}

	//NOV2014_20 Thothie - add ms_text option to "say" text as NPC
	void CGameText::KeyValue( KeyValueData *pkvd )
	{
		if (FStrEq(pkvd->szKeyName, "npcname"))
		{
			ms_npcname = pkvd->szValue;
			pkvd->fHandled = TRUE;
		}
		else if (FStrEq(pkvd->szKeyName, "sayasnpc"))
		{
			ms_sayasnpc = (atoi(pkvd->szValue)==1)?true:false;
			pkvd->fHandled = TRUE;
		}
	}
};
LINK_ENTITY_TO_CLASS( game_text, CGameText );
LINK_ENTITY_TO_CLASS( ms_text, CGameText ); //Thothie AUG2007a - To get around FGD issues

//
// Weather Changer
//
class CMSWeather : public CBaseEntity
{
public:
	void Spawn( void )
	{
		//Thothie DEC2010_27 - weather is precached with player script
		/*
		msstringlist Parameters;
		Parameters.add( m_WeatherName );
		if( MSGlobals::GameScript )
		{
			MSGlobals::GameScript->SetScriptVar( "game.precacheweather", m_WeatherName );
			MSGlobals::GameScript->CallScriptEvent( "game_precache_weather", &Parameters );
		}
		*/
	}
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
	{
		msstringlist Parameters;
		Parameters.add( m_WeatherName );
		TokenizeString( m_WeatherOptions, Parameters );

		//Thothie DEC2010_27 - lock weather via game master
		CBaseEntity *pGameMasterEnt = UTIL_FindEntityByString( NULL, "netname", msstring("�") + "game_master" );
		IScripted *pGMScript = pGameMasterEnt->GetScripted();
		
		if ( pGameMasterEnt )
		{
			pGMScript->CallScriptEvent( "game_set_weather", &Parameters );
		}

		//Thothie DEC2010_27 - old system
		/*
		if( MSGlobals::GameScript )
			MSGlobals::GameScript->CallScriptEvent( "game_change_weather", &Parameters );
		*/
	}

	void KeyValue( KeyValueData *pkvd )
	{
		if(		 !strcmp( pkvd->szKeyName,	"weather"		) ) m_WeatherName    = pkvd->szValue;
		else if( !strcmp( pkvd->szKeyName,	"options"		) ) m_WeatherOptions = pkvd->szValue;
		else CBaseEntity::KeyValue( pkvd );
	}

	string_i m_WeatherName;
	string_i m_WeatherOptions;
};
LINK_ENTITY_TO_CLASS( mstrig_weather, CMSWeather );

// **************************************************
//
//		   msarea_* Classes.  Entity brushes
//
// **************************************************

class CAreaInvisible : public CBaseEntity
{
	public:

	void Spawn( )
	{ 
		pev->angles		= g_vecZero;
		pev->movetype	= MOVETYPE_PUSH;  // so it doesn't get pushed by anything
		pev->solid		= SOLID_NOT;
		SET_MODEL( ENT(pev), STRING(pev->model) );
		
		// If it can't move/go away, it's really part of the world
		//pev->flags |= FL_WORLDBRUSH;
		m_Brush = true;

		//pev->spawnflags |= SF_WALL_START_OFF; 
		
		SetBits( pev->effects, EF_NODRAW );
		BaseClass::Spawn( );
	}
};

//[begin] Thothie NOV2014_07 msarea_music_dynamic for CBM system
class CAreaMusicDyn : public CAreaInvisible {
public:
	typedef CAreaInvisible BaseClass;

	msstring mt_idle;
	msstring mt_idle_length;
	msstring mt_combat;
	msstring mt_combat_length;
	msstring mt_global; //play for all players
	msstring mt_playnow; //0= neither, 1= idle, 2= combat
	string_t ms_master;

	void Spawn( )
	{
		BaseClass::Spawn( );
		pev->solid = SOLID_TRIGGER;
		UTIL_SetOrigin( pev, pev->origin );
		SetTouch( MusicTouch );
	}

	void KeyValue( KeyValueData *pkvd )
	{
		if (FStrEq(pkvd->szKeyName, "midle"))
		{
			mt_idle = pkvd->szValue;
			pkvd->fHandled = TRUE;
		}
		else if (FStrEq(pkvd->szKeyName, "midlelen"))
		{
			mt_idle_length = FloatToString(UTIL_StringToSecs(pkvd->szValue));
			pkvd->fHandled = TRUE;
		}
		else if (FStrEq(pkvd->szKeyName, "mcombat"))
		{
			mt_combat = pkvd->szValue;
			pkvd->fHandled = TRUE;
		}
		else if (FStrEq(pkvd->szKeyName, "mcombatlen"))
		{
			mt_combat_length = FloatToString(UTIL_StringToSecs(pkvd->szValue));
			pkvd->fHandled = TRUE;
		}
		else if (FStrEq(pkvd->szKeyName, "playall"))
		{
			mt_global = pkvd->szValue;
			pkvd->fHandled = TRUE;
		}
		else if (FStrEq(pkvd->szKeyName, "playnow"))
		{
			mt_playnow = pkvd->szValue;
			pkvd->fHandled = TRUE;
		}
		else if (FStrEq(pkvd->szKeyName, "master"))
		{
			ms_master = ALLOC_STRING(pkvd->szValue);
			pkvd->fHandled = TRUE;
		}
		else CBaseEntity::KeyValue( pkvd );
	}

	void MusicTouch( CBaseEntity *pOther ) 
	{
		if( !pOther->IsPlayer( ) ) 
			return;

		if ( ms_master )
		{
			ALERT( at_console,"DEBUG: %s - checking for master %s\n",STRING(pev->classname),STRING(ms_master));
			if (!UTIL_IsMasterTriggered(ms_master,pOther))
			{
				ALERT( at_console,"DEBUG: %s - master not unlocked.\n",STRING(pev->classname) );
				return;
			}
			else
			{
				ALERT( at_console,"DEBUG: %s - master unlocked, activating.\n", STRING(pev->classname));
			}
		}

		CBasePlayer *pPlayer = (CBasePlayer *)pOther;

		static msstringlist Params;
		Params.clearitems( );
		if ( mt_global.c_str() == "1" ) Params.add( EntToString(pOther) );
		Params.add( mt_idle.c_str() );
		Params.add( atof(mt_idle_length)>0 ? FloatToString( atof(mt_idle_length)/60 ) : "0" );
		Params.add( mt_combat.c_str() );
		Params.add( atof(mt_combat_length)>0 ? FloatToString( atof(mt_combat_length)/60 ) : "0" );
		Params.add( mt_playnow.c_str() );

		if ( mt_global.c_str() == "1" )
		{
			CBaseEntity *pGameMasterEnt = UTIL_FindEntityByString( NULL, "netname", msstring("�") + "game_master" );
			IScripted *pGMScript = pGameMasterEnt->GetScripted();
			static msstringlist Params;
			pGMScript->CallScriptEvent("gm_set_music",&Params);
		}
		else
		{
			pPlayer->CallScriptEvent("set_music",&Params);
		}
		//send script command to player, unless global, then GM
	}
};

LINK_ENTITY_TO_CLASS( msarea_music_dynamic, CAreaMusicDyn );
//[end] Thothie NOV2014_07 msarea_music_dynamic for CBM system


class CAreaMusic : public CAreaInvisible {
public:
	songplaylist m_Songs;
	string_t ms_master;
	msstring main_song;
	float main_song_length;
	typedef CAreaInvisible BaseClass;

    void Spawn( )
	{
		BaseClass::Spawn( );
		pev->solid = SOLID_TRIGGER;
		UTIL_SetOrigin( pev, pev->origin );
		SetTouch( MusicTouch );
	}

	void Deactivate( ) { m_Songs.clear( ); }

	void MusicTouch( CBaseEntity *pOther ) 
	{
		if( !pOther->IsPlayer( ) ) 
			return;

		//NOV2014_12 - seeing if we can give msarea_music a master switch
		//yes, we can, even if it's a bit hacky (added a new iuser to multisource)
		if ( ms_master )
		{
			ALERT( at_console,"DEBUG: %s - checking for master %s\n",STRING(pev->classname),STRING(ms_master));
			if (!UTIL_IsMasterTriggered(ms_master,pOther))
			{
				ALERT( at_console,"DEBUG: %s - master not unlocked.\n",STRING(pev->classname) );
				return;
			}
			else
			{
				ALERT( at_console,"DEBUG: %s - master unlocked, activating.\n", STRING(pev->classname));
			}
		}

		CBasePlayer *pPlayer = (CBasePlayer *)pOther;

		//NOV2014_12 Thothie - don't play if we're using combat music
		IScripted *iScripted = pPlayer ? pOther->GetScripted() : NULL;
		if ( iScripted )
		{
			if ( main_song.len() > 0 )
			{
				if ( m_Songs.size() && m_Songs[0].Name != main_song )
				{
					ALERT( at_console,"DEBUG: msarea_music - setting main song %s as idx 0.\n", main_song.c_str() );
					m_Songs[0].Name = main_song;
					m_Songs[0].Length = main_song_length;
				}
				else
				{
					ALERT( at_console,"DEBUG: msarea_music - adding main song %s.\n", main_song.c_str() );
					song_t Song;
					Song.Name = main_song;
					Song.Length = main_song_length;
					m_Songs.add(Song);
				}
			}

			bool playnow;
			msstring plr_cbm = iScripted->GetFirstScriptVar("PLR_COMBAT_MUSIC" );
			if ( plr_cbm != "none" && plr_cbm != "stop.mp3" )
			{
				ALERT( at_console,"DEBUG: msarea_music - plr has cbm %s.\n",plr_cbm.c_str());				
				msstring plr_cur = iScripted->GetFirstScriptVar("PLR_CURRENT_MUSIC" );
				if ( plr_cbm != plr_cur )
				{
					ALERT( at_console,"DEBUG: msarea_music current plr music is not cbm, playing first in list.\n");	
					playnow = true;
				}
				else
				{
					ALERT( at_console,"DEBUG: msarea_music - current plr music is cbm, adding first in list as idle.\n");
					playnow = false;
				}
			}
			else
			{
				ALERT( at_console,"DEBUG: msarea_music - current plr music has no cbm, playing first in list.\n");
				playnow = true;
			}

			//this method disables the ability to play lists, but I've never seen a map use that feature
			msstringlist Parameters;
			Parameters.add( m_Songs[0].Name.c_str() );
			Parameters.add( (m_Songs[0].Length>0) ? FloatToString( m_Songs[0].Length/60 ) : "0" );
			Parameters.add( playnow ? "1" : "0" );
			iScripted->CallScriptEvent("set_idle_music",&Parameters);
			//Old way jams up sometimes
			/*
			if( m_Songs.size() )
				pPlayer->Music_Play( m_Songs, this );
			else
				pPlayer->Music_Stop( this );
			*/
		}
	}

	void KeyValue( KeyValueData *pkvd )
	{
		//JAN2013_08 Thothie - Noticed msarea_musics were adding "zhlt_invisible" as a song. :O
		msstring sTemp = pkvd->szKeyName;
		if ( sTemp.contains(".mp3") || sTemp.contains(".midi") )
		{
			song_t Song;
			Song.Name = pkvd->szKeyName;
			Song.Length = UTIL_StringToSecs(pkvd->szValue); //DEC2014_21 Thothie - Centralizing music/time conversion
			m_Songs.add( Song );
		}
		else if (FStrEq(pkvd->szKeyName, "master")) 
		{
			ms_master = ALLOC_STRING(pkvd->szValue);
			pkvd->fHandled = TRUE;
		}
		else if (FStrEq(pkvd->szKeyName, "song")) //NOV2014_12 - Thothie - making this a bit more intuitive to use via smartedit
		{
			main_song = pkvd->szValue;
			pkvd->fHandled = TRUE;
		}
		else if (FStrEq(pkvd->szKeyName, "songlength"))
		{
			main_song_length = UTIL_StringToSecs(pkvd->szValue); //DEC2014_21 Thothie - Centralizing music/time conversion
			pkvd->fHandled = TRUE;
		}
		else CBaseEntity::KeyValue( pkvd );
	}
};

LINK_ENTITY_TO_CLASS( msarea_music, CAreaMusic );

struct monster_data_t {
	string_t classname,
			target, 
			targetname,
			killtarget,
			perishtarget,
			title, //Thothie AUG2007b allow monster name changes
			addparams, //Thothie DEC2007a allow pass params to script
			scriptfile;

	float	delaylow,
			delayhigh,
			delayvalue, //the current delay
			deathtime,
			spawnchance,
			dmgmulti,	//Thothie SEP2007a
			hpmulti;	//Thothie SEP2007a

	int		lives,
		    livesleft,
			hpreq_min, //Thothie FEB2011_22 - allow min;max hpreq definition
			hpreq_max, //Thothie FEB2011_22 - allow min;max hpreq definition
			nplayers, //Thothie AUG2007a - req# players for monster to spawn
			m_nRndMobs; //NOV2014_20 - Thothie msmonster_random

	Vector  origin, angles;
	bool	spawned,
		    spawnontrigger,  //monster only spawns when individually triggered
			triggered;
	long	lPrivData,
		    lTrigPrivData;  //For monsters spawned by trigger

	mslist <random_monster_t> random_monsterdata;  //NOV2014_20 - Thothie msmonster_random
};

class CAreaMonsterSpawn : public CAreaInvisible 
{
public:
	monster_data_t mdSpawnMonster[32];
	int iMonstersToSpawn;
	int iPlayerReq; //Thothie AUG2007a - adding optional player req
	int iHPReq_min; //Thothie AUG2007a - adding optional total HP on server req
	int iHPReq_max; //Thothie FEB2011_22 - adding min;max option for reqhp
	float thoth_next_spawn; //Thothie - JUN2007 - trying to stagger monster spawns to reduce lag
	enum spawnloc_e { SPAWNLOC_FIXED, SPAWNLOC_RANDOM } m_SpawnLoc;
	bool m_fSpawnOnTrigger, 
		 m_fActive; //Set to false when ALL monsters run out of lives.
	bool thoth_org_spawnstart;  //Thothie JUN2007b this bits tells it to spawn all monsters imediately, if spawn isnt triggered (see second occurance below)
	string_t m_sTargetAllPerish;
	int resetwhen; //NOV2014_20 Thothie - attempting to allow changes as to when ms_monsterspawn can respawn mobs 0=when all dead, 1=when any mob dead, 2=whenever triggered
	bool didfirstspawn; //NOV2014_20 Thothie - the above requires us to know whether we've done the initial spawn or not

	void Spawn( ) 
	{
		bool didfirstspawn = false; //NOV2014_20 Thothie - adding resetwhen options
		if( !m_fSpawnOnTrigger )
		{
			m_fActive = true;
			SetThink( SpawnMonsters );
			pev->nextthink = pev->ltime + 3.0;
		}
		else m_fActive = false;
		CAreaInvisible::Spawn();
		SetUse( ResetUse );
	}

	//Triggering this resets it.  It restores monster lives and lets them (re)spawn
	void ResetUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
	{
		if( m_fActive )
		{
			//NOV2014_20 Thothie - attempting to allow changes as to when ms_monsterspawn can respawn mobs 0=when all dead, 1=when any mob dead, 2=whenever triggered
			if ( !resetwhen ) return;
		}

		if ( resetwhen == 1 && didfirstspawn ) //>= just to deal with the stump below
		{
			//cycle through my m obs, see if any are out of lives, if so, reset their lives
			for( int i = 0; i < iMonstersToSpawn; i++ ) 
			{
				if ( mdSpawnMonster[i].livesleft == 0 && !mdSpawnMonster[i].spawned )
				{
					mdSpawnMonster[i].livesleft = mdSpawnMonster[i].lives;
					RespawnMonster( &mdSpawnMonster[i] );
					mdSpawnMonster[i].delayvalue = 0;
				}
			}
		}

		if ( resetwhen == 2 && didfirstspawn )
		{
			//trigger a new wave
			for( int i = 0; i < iMonstersToSpawn; i++ ) 
			{
				mdSpawnMonster[i].livesleft = mdSpawnMonster[i].lives;
				mdSpawnMonster[i].spawned = false;
				RespawnMonster( &mdSpawnMonster[i] );
				mdSpawnMonster[i].delayvalue = 0;
			}
		}

		if ( resetwhen && !didfirstspawn )
		{
			didfirstspawn = true;
			for( int i = 0; i < iMonstersToSpawn; i++ ) 
			{
				mdSpawnMonster[i].livesleft = mdSpawnMonster[i].lives;
				RespawnMonster( &mdSpawnMonster[i] );
				//Monsters spawn immediately the first time
				mdSpawnMonster[i].delayvalue = 0;
			}
		}

		//standard op
		if ( !resetwhen )
		{
			didfirstspawn = true;
			for( int i = 0; i < iMonstersToSpawn; i++ ) 
			{
				mdSpawnMonster[i].livesleft = mdSpawnMonster[i].lives;
				RespawnMonster( &mdSpawnMonster[i] );
				//Monsters spawn immediately the first time
				mdSpawnMonster[i].delayvalue = 0;
			}
		}

		m_fActive = true;
		SetThink( SpawnMonsters );
		pev->nextthink = pev->ltime + 0.1;
	}

	void KeyValue( KeyValueData *pkvd )
	{
		if (FStrEq(pkvd->szKeyName, "spawnloc"))
		{
			m_SpawnLoc = (spawnloc_e)atoi(pkvd->szValue);
			if( m_SpawnLoc < SPAWNLOC_FIXED || m_SpawnLoc > SPAWNLOC_RANDOM )
				m_SpawnLoc = SPAWNLOC_FIXED;
			pkvd->fHandled = TRUE;
		}
		else if (FStrEq(pkvd->szKeyName, "spawnstart"))
		{
			//Thothie - JUN2007a
			//various atttempts to force monster spawn to spawnstart 1 -  fail
			//bool thoth_in_value = atoi(pkvd->szValue) ? true : false;
			//m_fSpawnOnTrigger = true;
			//msstring thoth_new_targetname = "game_playerspawned";
			//if ( !thoth_in_value ) pev->targetname = ALLOC_STRING(thoth_new_targetname);
			//
			//Thothie JUN2007 FAIL - see m_fSpawnOnTrigger above
			/*if ( !m_fSpawnOnTrigger )
			{
				pev->targetname = ALLOC_STRING("game_playerspawn");
				m_TargetName = pev->targetname;
				logfile << "Monsterspawn with spawnstart 0! \r\n";
			}*/
			//original:
			m_fSpawnOnTrigger = (atoi(pkvd->szValue)) ? true : false;
			//Thothie - store original spawnstart state			
			thoth_org_spawnstart = (atoi(pkvd->szValue)) ? true : false;
			pkvd->fHandled = TRUE;
		}
		//NOV2014_20 seeing if we can change how ms_monsterspawn handles mob respawning
		//- the fact that ms_monster and ms_monsterspawn seem to kinda be the same entity is making this confusing though
		else if (FStrEq(pkvd->szKeyName, "resetwhen"))
		{
			//0=all mobs depleated, 1=any mob depleated, 2=whenever called
			resetwhen = atoi( pkvd->szValue );
			pkvd->fHandled = TRUE;
		}
		else if (FStrEq(pkvd->szKeyName, "fireallperish"))
		{
			m_sTargetAllPerish = pev->target = ALLOC_STRING( pkvd->szValue );
			pkvd->fHandled = TRUE;
		}
		else if (FStrEq(pkvd->szKeyName, "nplayers"))
		{
			//Thothie AUG2007a - player req for monster spawns
			iPlayerReq = (atoi(pkvd->szValue));
			pkvd->fHandled = TRUE;
		}
		else if (FStrEq(pkvd->szKeyName, "reqhp"))
		{
			//Thothie AUG2007a - total hp present req for monster spawns
			msstringlist reqhp_stringlist;
			TokenizeString((pkvd->szValue),reqhp_stringlist);
			iHPReq_min = 0;
			iHPReq_max = 0;
			if ( reqhp_stringlist.size() > 0 ) iHPReq_min = atoi(reqhp_stringlist[0].c_str());
			if ( reqhp_stringlist.size() > 1 )
			{
				iHPReq_max = atoi(reqhp_stringlist[1].c_str());
				if ( iHPReq_max < iHPReq_min )
				{
					logfile << "MAP_ERROR: " << this->pev->classname << " - max reqhp set higher than min.";
					iHPReq_max = 0;
				}
				if ( iHPReq_min == 0 ) iHPReq_min = 1;
			}
			pkvd->fHandled = TRUE;
		}
		else CBaseEntity::KeyValue( pkvd );
	}

	void RespawnMonster( monster_data_t *pMonsterData )
	{
		if( pMonsterData->spawned )
		{
			return;
		}

		//NOV2014_20 - Thothie msmonster_random [begin]
		if ( pMonsterData->m_nRndMobs > 0 )
		{
			int idx = RANDOM_LONG( 0, FLOAT(pMonsterData->m_nRndMobs)-1 );
			foreach(i,pMonsterData->m_nRndMobs)
			{
				logfile << UTIL_VarArgs("DEBUG: respawn randommob list #%i / %i as %s\n",i,pMonsterData->m_nRndMobs,pMonsterData->random_monsterdata[i].m_ScriptName ? pMonsterData->random_monsterdata[i].m_ScriptName.c_str() : "???");
			}
			//logfile << UTIL_VarArgs("DEBUG: respawn randommob chose: #i %s\n",idx,pMonsterData->random_monsterdata[idx].m_ScriptName?pMonsterData->random_monsterdata[idx].m_ScriptName:"???");
			logfile << UTIL_VarArgs("DEBUG: respawn chose randommob #%i %s\n",idx);
			//I DIE HERE:
			logfile << UTIL_VarArgs("DEBUG: specifically: %s\n",pMonsterData->random_monsterdata[idx].m_ScriptName.c_str());
			pMonsterData->scriptfile = ALLOC_STRING(pMonsterData->random_monsterdata[idx].m_ScriptName);
			pMonsterData->title = ALLOC_STRING(pMonsterData->random_monsterdata[idx].m_title);
			pMonsterData->addparams = ALLOC_STRING(pMonsterData->random_monsterdata[idx].m_addparams);
			pMonsterData->dmgmulti = pMonsterData->random_monsterdata[idx].m_DMGMulti;
			pMonsterData->hpmulti = pMonsterData->random_monsterdata[idx].m_HPMulti;
			//nix these from the struct, this ain't gonna work
			//pMonsterData->lives = pMonsterData->random_monsterdata[idx].m_Lives;
			//pMonsterData->livesleft = pMonsterData->random_monsterdata[idx].m_Lives;
			pMonsterData->nplayers = pMonsterData->random_monsterdata[idx].m_ReqPlayers;
			pMonsterData->hpreq_min = pMonsterData->random_monsterdata[idx].m_HPReq_min;
			pMonsterData->hpreq_max = pMonsterData->random_monsterdata[idx].m_HPReq_max;
		}
		//NOV2014_20 - Thothie msmonster_random [end]

		//If the monster has unlimited lives or has some lives left, respawn it
		if( pMonsterData->lives == -1 || pMonsterData->livesleft > 0 )
		{
			pMonsterData->deathtime = gpGlobals->time;
			pMonsterData->delayvalue = RANDOM_FLOAT(pMonsterData->delaylow,pMonsterData->delayhigh);
		}
		else if( pMonsterData->lives > 0 && !pMonsterData->livesleft ) 
			FireTargets( STRING(pMonsterData->perishtarget), this, this, USE_TOGGLE, 0 );
	}

	//count players and hp were here, moved to utils.cpp

	void Activate( ) 
	{
		if( !m_pGoalEnt ) return;
		//if ( iPlayerReq < thoth_CountPlayers() && iPlayerReq > 0 ) return; //Thothie AUG2007a - player req for monster spawns (FAIL - but the monster side one works fine)

		CMSMonster *pMonster = (CMSMonster *)m_pGoalEnt;
		
		//NOV2014_20 - Thothie msmonster_random [begin]
		if ( pMonster->m_nRndMobs > 0 )
		{
			int idx = RANDOM_LONG( 0, FLOAT(pMonster->m_nRndMobs)-1 );
			mdSpawnMonster[iMonstersToSpawn].m_nRndMobs = pMonster->m_nRndMobs;
			foreach(i,pMonster->m_nRndMobs)
			{
				logfile << UTIL_VarArgs("DEBUG: spawn adding randommob #%i / %i as %s\n",i,pMonster->m_nRndMobs,pMonster->random_monsterdata[i].m_ScriptName ? pMonster->random_monsterdata[i].m_ScriptName.c_str() : "???");
				mdSpawnMonster[iMonstersToSpawn].random_monsterdata.add(pMonster->random_monsterdata[i]); //read em in
			}
			//logfile << UTIL_VarArgs("DEBUG: spawn chose randommob #%i = %s\n",idx,pMonster->random_monsterdata[idx].m_ScriptName ? pMonster->random_monsterdata[idx].m_ScriptName : "???");
			logfile << UTIL_VarArgs("DEBUG: spawn chose randommob #%i\n",idx);
			//I DIE HERE:
			logfile << UTIL_VarArgs("DEBUG: specifically %s\n",pMonster->random_monsterdata[idx].m_ScriptName.c_str());
			mdSpawnMonster[iMonstersToSpawn].scriptfile = ALLOC_STRING(pMonster->random_monsterdata[idx].m_ScriptName);
			mdSpawnMonster[iMonstersToSpawn].title = ALLOC_STRING(pMonster->random_monsterdata[idx].m_title);
			mdSpawnMonster[iMonstersToSpawn].addparams = ALLOC_STRING(pMonster->random_monsterdata[idx].m_addparams);
			mdSpawnMonster[iMonstersToSpawn].dmgmulti = pMonster->random_monsterdata[idx].m_DMGMulti;
			mdSpawnMonster[iMonstersToSpawn].hpmulti = pMonster->random_monsterdata[idx].m_HPMulti;
			mdSpawnMonster[iMonstersToSpawn].nplayers = pMonster->random_monsterdata[idx].m_ReqPlayers;
			mdSpawnMonster[iMonstersToSpawn].hpreq_min = pMonster->random_monsterdata[idx].m_HPReq_min;
			mdSpawnMonster[iMonstersToSpawn].hpreq_max = pMonster->random_monsterdata[idx].m_HPReq_max;
			mdSpawnMonster[iMonstersToSpawn].lives =
			mdSpawnMonster[iMonstersToSpawn].livesleft = pMonster->m_Lives;
		}
		//NOV2014_20 - Thothie msmonster_random [end]
		else
		{
			mdSpawnMonster[iMonstersToSpawn].scriptfile = ALLOC_STRING(pMonster->m_ScriptName);
			mdSpawnMonster[iMonstersToSpawn].title = ALLOC_STRING(pMonster->m_title);
			mdSpawnMonster[iMonstersToSpawn].addparams = ALLOC_STRING(pMonster->m_addparams);
			mdSpawnMonster[iMonstersToSpawn].dmgmulti = pMonster->m_DMGMulti;
			mdSpawnMonster[iMonstersToSpawn].hpmulti = pMonster->m_HPMulti;
			mdSpawnMonster[iMonstersToSpawn].lives =
			mdSpawnMonster[iMonstersToSpawn].livesleft = pMonster->m_Lives;
			mdSpawnMonster[iMonstersToSpawn].nplayers = pMonster->m_ReqPlayers;
			mdSpawnMonster[iMonstersToSpawn].hpreq_min = pMonster->m_HPReq_min;
			mdSpawnMonster[iMonstersToSpawn].hpreq_max = pMonster->m_HPReq_max;
		}

		mdSpawnMonster[iMonstersToSpawn].origin = pMonster->pev->origin;
		mdSpawnMonster[iMonstersToSpawn].angles = pMonster->pev->angles;
		mdSpawnMonster[iMonstersToSpawn].classname = pMonster->pev->classname;
		mdSpawnMonster[iMonstersToSpawn].target = pMonster->pev->target;
		mdSpawnMonster[iMonstersToSpawn].targetname = pMonster->pev->targetname;
		mdSpawnMonster[iMonstersToSpawn].killtarget = ALLOC_STRING(pMonster->m_iszKillTarget);
		mdSpawnMonster[iMonstersToSpawn].perishtarget = ALLOC_STRING(pMonster->m_iszPerishTarget);
		mdSpawnMonster[iMonstersToSpawn].delaylow = pMonster->m_SpawnDelayLow;
		mdSpawnMonster[iMonstersToSpawn].delayhigh = pMonster->m_SpawnDelayHigh;
		mdSpawnMonster[iMonstersToSpawn].deathtime = gpGlobals->time;


		//Monsters now spawn immediately the FIRST time
		//The respawn delay only affects respawns... not first spawns.
		//mdSpawnMonster[iMonstersToSpawn].delayvalue = RANDOM_FLOAT(mdSpawnMonster[iMonstersToSpawn].delaylow,mdSpawnMonster[iMonstersToSpawn].delayhigh);
		mdSpawnMonster[iMonstersToSpawn].delayvalue = 0;
		mdSpawnMonster[iMonstersToSpawn].spawnchance = pMonster->m_SpawnChance;
		mdSpawnMonster[iMonstersToSpawn].spawned = false;
		mdSpawnMonster[iMonstersToSpawn].spawnontrigger = pMonster->m_fSpawnOnTrigger;
		mdSpawnMonster[iMonstersToSpawn].triggered = false;
		mdSpawnMonster[iMonstersToSpawn].lTrigPrivData = (long)pMonster;

		iMonstersToSpawn++;

		m_pGoalEnt = NULL;
	}
	void DeathNotice( entvars_t *pevChild ) 
	{
		CMSMonster *pMonster = (CMSMonster *)CBaseEntity::Instance( pevChild );
		if( !pMonster ) return;

		for( int i = 0; i < iMonstersToSpawn; i++ ) {
			if( mdSpawnMonster[i].lPrivData == (long)pMonster ) 
			{
                mdSpawnMonster[i].spawned = false;
				RespawnMonster( &mdSpawnMonster[i] );
				break;
			}
		}
	}
	void SpawnMonsters( ) 
	{
		int i = 0, iDeadMonsters = 0;
		CMSMonster *pMonster;
		for( i = 0; i < iMonstersToSpawn; i++ ) 
		{
			if( mdSpawnMonster[i].spawned ) continue;
			//If the monster is out of lives, don't respawn
			if( mdSpawnMonster[i].lives > 0 && mdSpawnMonster[i].livesleft <= 0 ) 
			{
				iDeadMonsters++; //Count the dead monsters
				continue;
			}

			if ( mdSpawnMonster[i].nplayers > 0 )
			{
				if ( UTIL_NumActivePlayers() < mdSpawnMonster[i].nplayers )
				{
					//Thothie AUG2007a - not enough players to spawn monster
					//count as dead and continue
					iDeadMonsters++; //Count the dead monsters
					continue;
				}
			}

			if ( mdSpawnMonster[i].hpreq_min > 0 || mdSpawnMonster[i].hpreq_max > 0 ) //NOV2014_20 - Thothie - fixed for potential bug if all players flagged AFK
			{
				float thoth_thp = UTIL_TotalHP();
				bool thoth_nospawn = false;
				if ( thoth_thp < mdSpawnMonster[i].hpreq_min ) thoth_nospawn = true;
				if ( thoth_thp >= mdSpawnMonster[i].hpreq_max && mdSpawnMonster[i].hpreq_max > 0 ) thoth_nospawn = true;
				if ( thoth_nospawn )
				{
					//Thothie AUG2007a - not enough hp on server to spawn monster
					//count as dead and continue
					mdSpawnMonster[i].spawnchance = -1; //don't respawn later just cuz you meet reqs later
					iDeadMonsters++; //count as dead
					continue;
				}
			}

			//If the monster has spawn on trigger set and It's trying to spawn
			//the first time, don't spawn it. (It must be triggered the first time)
			if( mdSpawnMonster[i].spawnontrigger &&
				!mdSpawnMonster[i].triggered &&
				mdSpawnMonster[i].lives == mdSpawnMonster[i].livesleft )
					continue;

			if( (gpGlobals->time - mdSpawnMonster[i].deathtime) < mdSpawnMonster[i].delayvalue ) continue;

			if( RANDOM_FLOAT(0,99) > mdSpawnMonster[i].spawnchance )
			{	//The percent chance it would spawn failed..
				RespawnMonster( &mdSpawnMonster[i] );
				continue;
			}

			//Thothie - JUN2007 - trying to stagger monster spawns to reduce lag
			if ( gpGlobals->time < thoth_next_spawn )
			{
				//only if spawnstart 1
				//otherwise it fubars the already buggy fireallperish on sloppily built msarea_monsterspawns
				if ( thoth_org_spawnstart ) 
					continue;
			}
			thoth_next_spawn = gpGlobals->time + 0.2;
	
			//Spawn a monster
			pMonster = (CMSMonster *)CREATE_ENT(STRING(mdSpawnMonster[i].classname));
			if( !pMonster ) continue;

			//Thothie AUG2007b 
			//send title property to monster (other properties in future)
			pMonster->m_title = STRING(mdSpawnMonster[i].title);
			pMonster->m_addparams = STRING(mdSpawnMonster[i].addparams);
			pMonster->m_spawnedby = EntToString(this);
			//[/thothie]

			//Thothie SEP2007a
			//- send hp/dmg multipliers
			pMonster->m_DMGMulti = mdSpawnMonster[i].dmgmulti;
			pMonster->m_HPMulti = mdSpawnMonster[i].hpmulti;


			pMonster->pev->target =	mdSpawnMonster[i].target;
			pMonster->pev->targetname =	mdSpawnMonster[i].targetname;
			pMonster->m_iszKillTarget = STRING(mdSpawnMonster[i].killtarget);
			pMonster->m_iszPerishTarget = STRING(mdSpawnMonster[i].perishtarget);
			pMonster->m_ScriptName = STRING(mdSpawnMonster[i].scriptfile);
			mdSpawnMonster[i].lPrivData = (long)pMonster;

			pMonster->pev->owner = edict();
			if( m_SpawnLoc == SPAWNLOC_RANDOM )
			{
				CBaseEntity *List[1];
				int iTrys = 1;
				do
				{
					pMonster->pev->origin.x = RANDOM_FLOAT(pev->absmin.x,pev->absmax.x);
					pMonster->pev->origin.y = RANDOM_FLOAT(pev->absmin.y,pev->absmax.y);
					pMonster->pev->origin.z = Center().z;
					iTrys++;
				} while( UTIL_MonstersInSphere( List, 1, pMonster->pev->origin, 90 ) && iTrys < 10 );
			}
			else pMonster->pev->origin = mdSpawnMonster[i].origin;

			pMonster->pev->angles = mdSpawnMonster[i].angles;

			mdSpawnMonster[i].spawned = true;
			mdSpawnMonster[i].triggered = false;
			if( mdSpawnMonster[i].lives > 0 && mdSpawnMonster[i].livesleft > 0 )
				mdSpawnMonster[i].livesleft--;
			
			UTIL_SetOrigin( pMonster->pev, pMonster->pev->origin );
			pMonster->Spawn( );
			
			//Log( "Start" );
			//msstring LogStr;
			//sprintf( LogStr.c_str(), "[Spawn monster] %s from %s at %s", pMonster->DisplayName(), STRING(pev->targetname), VecToString(pMonster->pev->origin) );
			//ALERT( at_console, "%s\n", LogStr.c_str() );
			//Log( "%s\n", LogStr.c_str() );
			//Log( "End" );
		}

		if( iDeadMonsters && (iDeadMonsters == iMonstersToSpawn) && m_fActive )
		{
			//All monsters are out of lives
			//Firetargets, become inactive, and stop thinking
			FireTargets( STRING(m_sTargetAllPerish), this, this, USE_TOGGLE, 0 );
			m_fActive = false;
		}
		else pev->nextthink = pev->ltime + 0.2;//0.5
	}
	void *MSQuery( int iRequest )
	{ 
		//Spawn the specified monster RIGHT NOW
		CMSMonster *pMonster = (CMSMonster *)CBaseEntity::Instance( (entvars_t *)iRequest );
		if( !pMonster ) return NULL;

		for( int i = 0; i < iMonstersToSpawn; i++ )
			if( mdSpawnMonster[i].lTrigPrivData == (long)pMonster ) 
			{
				mdSpawnMonster[i].triggered = true;
				mdSpawnMonster[i].deathtime = mdSpawnMonster[i].delayvalue = 0;
				break;
			}
		return NULL;
	}
};

LINK_ENTITY_TO_CLASS( msarea_monsterspawn, CAreaMonsterSpawn );
LINK_ENTITY_TO_CLASS( ms_monsterspawn, CAreaMonsterSpawn );

//This will make a monster spawn area become inactive (monsters stop spawning)
//Thothie note: this does not seem to function
class CTrigStopMonsterSpawn : public CBaseEntity
{

	string_t sTarget;
	bool fRemoveAllMonsters;

	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
	{
		edict_t *peSpawnArea = NULL,
			*peFirstArea = NULL;
		bool fFoundOneArea;

		while( peSpawnArea = FIND_ENTITY_BY_TARGETNAME( peSpawnArea, STRING(pev->target) ) ) {
			if( peSpawnArea == peFirstArea ) break;
			else if( !peFirstArea ) peFirstArea = peSpawnArea;
			CBaseEntity *pSpawnArea = CBaseEntity::Instance( peSpawnArea );
			if( pSpawnArea && ( FStrEq(STRING(pSpawnArea->pev->classname),"msarea_monsterspawn") || FStrEq(STRING(pSpawnArea->pev->classname),"ms_monsterspawn") ) ) {
				fFoundOneArea = true;
				pSpawnArea->SetThink( NULL );
				((CAreaMonsterSpawn *)pSpawnArea)->m_fActive = false;
			}
		}

		if( !fFoundOneArea ) ALERT( at_console, "ERROR: mstrig_stopspawn can't find area named %s\n", STRING(pev->target) );

		CBaseEntity::Use( pActivator, pCaller, useType, value );
	}
};

LINK_ENTITY_TO_CLASS( mstrig_stopspawn, CTrigStopMonsterSpawn );

//[Begin] Thothie NOV2014_08 msarea_transition_local
//- serves as an asthetic teleport/checkpoint between different areas of the same map
class CAreaTransitionLocal : public CAreaInvisible {
public:
	typedef CAreaInvisible BaseClass;

	//todos: add hp/#plr reqs, scriptevent
	//- mtl_respawnat (might wanna string_t that)
	//- script side messages

	bool mtl_req_all_players; //rallplayers
	msstring mtl_title; //title
	//msstring mtl_desc; //desc //swapping for Press +Use to continue
	string_t mtl_teledest; //teleport (if any)
	msstring mtl_respawnat; //spawntotie (if any)
	string_t mtl_target; //firetarget - fire target on activate (if any)
	string_t mtl_touchtarget; //fires when touched (with each message)
	int iTeleIdx; //teleport index tracker
	string_t ms_master;

	void Spawn( )
	{
		BaseClass::Spawn( );
		pev->solid = SOLID_TRIGGER;
		UTIL_SetOrigin( pev, pev->origin );
		SetTouch( TransTouch );
		iTeleIdx = 0;
	}

	void KeyValue( KeyValueData *pkvd )
	{
		if (FStrEq(pkvd->szKeyName, "rallplayers"))
		{
			mtl_req_all_players = (pkvd->szValue == "1") ? true : false;
			pkvd->fHandled = TRUE;
		}
		else if (FStrEq(pkvd->szKeyName, "title"))
		{
			mtl_title = pkvd->szValue;
			pkvd->fHandled = TRUE;
		}
		else if (FStrEq(pkvd->szKeyName, "master"))
		{
			ms_master = ALLOC_STRING(pkvd->szValue);
			pkvd->fHandled = TRUE;
		}
		else if (FStrEq(pkvd->szKeyName, "teleport"))
		{
			mtl_teledest = ALLOC_STRING(pkvd->szValue);
			pkvd->fHandled = TRUE;
		}
		else if (FStrEq(pkvd->szKeyName, "spawntotie"))
		{
			mtl_respawnat = pkvd->szValue;
			pkvd->fHandled = TRUE;
		}
		else if (FStrEq(pkvd->szKeyName, "firetarget"))
		{
			mtl_target = ALLOC_STRING(pkvd->szValue);
			pkvd->fHandled = TRUE;
		}
		else if (FStrEq(pkvd->szKeyName, "touchtarget"))
		{
			mtl_touchtarget = ALLOC_STRING(pkvd->szValue);
			pkvd->fHandled = TRUE;
		}
		else CBaseEntity::KeyValue( pkvd );
	}

	void TransTouch( CBaseEntity *pOther )
	{
		if( !pOther->IsPlayer( ) ) 
			return;
		
		if ( ms_master )
		{
			ALERT( at_console,"DEBUG: %s - checking for master %s\n",STRING(pev->classname),STRING(ms_master));
			if (!UTIL_IsMasterTriggered(ms_master,pOther))
			{
				ALERT( at_console,"DEBUG: %s - master not unlocked.\n",STRING(pev->classname) );
				return;
			}
			else
			{
				ALERT( at_console,"DEBUG: %s - master unlocked, activating.\n", STRING(pev->classname));
			}
		}

		CBaseEntity *pGameMasterEnt = UTIL_FindEntityByString( NULL, "netname", msstring("�") + "game_master" );
		IScripted *pGMScript = pGameMasterEnt->GetScripted();
		if ( pGMScript )
		{
			if ( pGMScript->GetFirstScriptVar("GM_DISABLE_TRANSITIONS") == "1" ) return;
		}

		CBasePlayer *pPlayer = (CBasePlayer *)pOther;
		IScripted *iScripted = pOther->GetScripted();
		if ( iScripted )
		{
			msstring last_ltrans_touch =  iScripted->GetFirstScriptVar("PLR_LOCAL_TRANS");
			if ( !last_ltrans_touch.starts_with(EntToString(this).c_str()) )
			{
				//iScripted->SetScriptVar("PLR_LOCAL_TRANS",EntToString(this)); //script side now
				
				//send event for plr message here
				static msstringlist Params;
				Params.clearitems( );
				Params.add( mtl_title.c_str() );
				Params.add( EntToString(this).c_str() );
				Params.add( mtl_req_all_players ? "0" : "1" );
				//Params.add( VecToString(pev->origin) ); //no good, sends vec0 (prob cuz no origin brush, dun wanna require it)
				Params.add( VecToString(pev->absmin) );
				Params.add( VecToString(pev->absmax) );
				Params.add( (mtl_respawnat.len() > 0) ? mtl_respawnat.c_str() : "none" );
				pPlayer->CallScriptEvent("game_touched_local_trans",&Params);
				if ( mtl_touchtarget ) FireTargets( STRING(mtl_touchtarget), pOther, this, USE_TOGGLE, 0 );
				return;
			}
			else
			{
				if ( (FBitSet(pPlayer->pbs.ButtonsDown,IN_USE)) )
				{
					bool valid_activate;
					valid_activate = false;

					if ( mtl_req_all_players )
					{
						if ( !FAllPlayersAreTouchingMe() )
						{
							pPlayer->SendHUDMsg("","All players must be present to activate this local transition.");
							Print("DEBUG: LTrans not all players present\n");
							return;
						}
						else
						{
							valid_activate = true;
						}
					}
					else
					{
						valid_activate = true;
					}

					if ( valid_activate )
					{
						if ( mtl_teledest )
						{
							entvars_t* pevToucher = pOther->pev;
							edict_t	*pentTarget = NULL;
							mslist <CBaseEntity *> TeleLocs;
							CBaseEntity *pLoc = NULL;
							msstring telefound;
							while ( pLoc = UTIL_FindEntityByClassname( pLoc, "info_teleport_destination" ) )
							{
								if ( FNullEnt(pLoc) )
									continue;
								
								if ( msstring( STRING(mtl_teledest) ) == msstring ( STRING(pLoc->pev->targetname) ) )
								{
									TeleLocs.add( pLoc );
								}
							}

							if ( TeleLocs.size() )
							{
								if ( iTeleIdx > (int)(TeleLocs.size()-1) ) iTeleIdx = 0;
								pentTarget = TeleLocs[ iTeleIdx ]->edict();
								++iTeleIdx;
								Vector tmp = VARS( pentTarget )->origin;
								tmp.z -= pOther->pev->mins.z;
								tmp.z++;
								pevToucher->flags &= ~FL_ONGROUND;
								UTIL_SetOrigin( pevToucher, tmp );
								pevToucher->angles = pentTarget->v.angles;
								pevToucher->fixangle = TRUE;
								pevToucher->velocity = pevToucher->basevelocity = g_vecZero;
							}
							else
							{
								ALERT( at_console, "Warning: Ltrans msarea_transition_local - no info_teleport_destination named %s found.\n",STRING(mtl_teledest) );
							}
						}

						if ( mtl_target )
						{
							FireTargets( STRING(mtl_target), pOther, this, USE_TOGGLE, 0 );
						}

						iScripted->SetScriptVar("PLR_LOCAL_TRANS","none");
					}
					else
					{
						return;
					} //end valid_activate
				}
			} //end toucher in use
		}
		else return; //not iscripted (for some reason)
	}


	bool FAllPlayersAreTouchingMe( )
	{
		Print("DEBUG: Ltrans FAllPlayersAreTouchingMe\n");
		for( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CBasePlayer *pOtherPlayer = (CBasePlayer *)UTIL_PlayerByIndex( i );
			if ( !pOtherPlayer ) continue;
			if ( !pOtherPlayer->IsActive() ) continue;

			CBaseEntity *pEntity = UTIL_PlayerByIndex( i );
			IScripted *iScripted = pEntity ? pEntity->GetScripted() : NULL;

			if ( !iScripted ) continue;

			msstring last_ltrans_touch = iScripted->GetFirstScriptVar("PLR_LOCAL_TRANS");
			if ( last_ltrans_touch.c_str() != EntToString(this).c_str() ) return false;
		}

		return true;
	}

	//Doesn't work - be nice if it did, as I'm gonna have to hack something up
	/*
	void DeathNotice ( entvars_t *pev ) 
	{
		if( !CBaseEntity::Instance(pev)->IsPlayer( ) ) return;
		CBaseEntity *pExiter = CBaseEntity::Instance(pev); //iffy
		IScripted *iScripted = pExiter->GetScripted();
		if ( iScripted )
		{
			msstring last_ltrans_touch =  iScripted->GetFirstScriptVar("PLR_LOCAL_TRANS");
			if ( last_ltrans_touch == EntToString(this) )
			{
				iScripted->SetScriptVar("PLR_LOCAL_TRANS","none");
				return;
			}
		}
		else return;
	}
	*/
};


LINK_ENTITY_TO_CLASS( msarea_transition_local, CAreaTransitionLocal );
//[End] Thothie NOV2014_08 msarea_transition_local


//int CountPlayers( void );
class CAreaTransition : public CAreaInvisible {
public:
	string_t sDestName, sDestMap, sDestTrans, sName, ms_master;

	CAreaTransition::CAreaTransition( ) : CAreaInvisible( ) {
		sDestName = sDestMap = sDestTrans = 0;
	}

	int PlayerVotes;
	bool thoth_didvote;

	bool FAllPlayersAreTouchingMe( )
	{
		for( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CBasePlayer *pOtherPlayer = (CBasePlayer *)UTIL_PlayerByIndex( i );
			if( !pOtherPlayer ) continue;
			if( !pOtherPlayer->IsActive() ) continue;

			if( pOtherPlayer->CurrentTransArea != this )
				return false;
		}

		return true;
	}
	
	void Spawn( ) {
		CAreaInvisible::Spawn();
		thoth_didvote = false;
		//For some reason, the targetname gets unset after this Spawn()
		//function.  No time to find out why, just save it here.
		sName = pev->targetname;
		if( !sName )
		{
			ALERT( at_console, "ERROR: msarea_transition with no Name!!\n" );
			UTIL_Remove( this );
		}
	}	
	BOOL OnControls( entvars_t *pev ) 
	{
		if ( ms_master )
		{
			CBaseEntity *pMchecker = CBaseEntity::Instance(pev);
			ALERT( at_console,"DEBUG: %s - checking for master %s\n",STRING(pev->classname),STRING(ms_master));
			if (!UTIL_IsMasterTriggered(ms_master,pMchecker))
			{
				ALERT( at_console,"DEBUG: %s - master not unlocked.\n","msarea_transition" );
				return FALSE;
			}
			else
			{
				ALERT( at_console,"DEBUG: %s - master unlocked, activating.\n", "msarea_transition");
			}
		}

		CBaseEntity *pGameMasterEnt = UTIL_FindEntityByString( NULL, "netname", msstring("�") + "game_master" );
		IScripted *pGMScript = pGameMasterEnt->GetScripted();
		if ( pGMScript )
		{
			if ( pGMScript->GetFirstScriptVar("GM_DISABLE_TRANSITIONS") == "1" ) return FALSE;
		}

		if( !CBaseEntity::Instance(pev)->IsPlayer( ) ) return FALSE;
		
		CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance(pev);

		if( pPlayer->CurrentTransArea == this ) return FALSE;
		pPlayer->CurrentTransArea = this;
		if( pPlayer->m_MapStatus == FIRST_MAP ) pPlayer->m_MapStatus = OLD_MAP;


		bool fChangeLocalMap = FAllPlayersAreTouchingMe( );
		bool fAutoShowBrowser = !fChangeLocalMap;

		if( fChangeLocalMap )
		{
			for( int i = 1; i <= gpGlobals->maxClients; i++ )
			{
				CBasePlayer *pOtherPlayer = (CBasePlayer *)UTIL_PlayerByIndex( i );
				if( !pOtherPlayer ) continue;

				PlayerVotes = 0;
			}
		}

		#define TRANS_AUTOSHOWBROWSER (1<<0)
		#define TRANS_PLAYSOUND (1<<1)

		strcpy( pPlayer->m_OldTransition, STRING(sName) );
		strcpy( pPlayer->m_NextMap, STRING(sDestMap) );
		strcpy( pPlayer->m_NextTransition, STRING(sDestTrans) );
		pPlayer->m_SpawnTransition = pPlayer->m_OldTransition;

		//Save character
		pPlayer->SaveChar( );

		if( !MSGlobals::ServerSideChar )
			pPlayer->m_TimeCharLastSent = 0;	//Ensure char is sent down to the client immediately

		//Thothie JUN2007 - tired of this not displaying, letting scripts handle it
		//msstring Text = msstring("It appears that you wish to travel to ") + STRING(sDestName) + ".\nPress enter (accept), to continue.";
		//pOtherPlayer->SendHUDMsg( "Travel", Text );
		msstringlist Parameters;
		Parameters.add( STRING(sDestName) );
		Parameters.add( STRING(sDestMap) );
		Parameters.add( STRING(sName) );
		Parameters.add( STRING(sDestTrans) );
		pPlayer->CallScriptEvent( "game_transition_entered", &Parameters );

		MESSAGE_BEGIN( MSG_ONE, g_netmsg[NETMSG_CLDLLFUNC], NULL, pPlayer->pev );
			WRITE_BYTE( 3 );
			WRITE_BYTE( 0 );
			WRITE_BYTE( (fAutoShowBrowser?TRANS_AUTOSHOWBROWSER:0) | TRANS_PLAYSOUND );
			WRITE_STRING( STRING(sDestMap) );
			WRITE_STRING( STRING(sName) );
			WRITE_STRING( STRING(sDestTrans) );
		MESSAGE_END();


		return TRUE;
	}
	// DeathNotice - Lets this transition area know that a client has left it;
	void DeathNotice ( entvars_t *pev ) 
	{
		if( !CBaseEntity::Instance(pev)->IsPlayer( ) ) return;

		if ( ms_master )
		{
			CBaseEntity *pMchecker = CBaseEntity::Instance(pev);
			ALERT( at_console,"DEBUG: %s - checking for master %s\n",STRING(pev->classname),"msarea_transition");
			if (!UTIL_IsMasterTriggered(ms_master,pMchecker))
			{
				ALERT( at_console,"DEBUG: %s - master not unlocked.\n","msarea_transition");
				return;
			}
			else
			{
				ALERT( at_console,"DEBUG: %s - master unlocked, allowing exit notice.\n", "msarea_transition");
			}
		}

		CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance(pev);
		if( !pPlayer || pPlayer->CurrentTransArea != this ) return;

		CBaseEntity *pGameMasterEnt = UTIL_FindEntityByString( NULL, "netname", msstring("�") + "game_master" );
		IScripted *pGMScript = pGameMasterEnt->GetScripted();
		if ( pGMScript )
		{
			if ( pGMScript->GetFirstScriptVar("GM_DISABLE_TRANSITIONS") == "1" ) return;
		}

		MESSAGE_BEGIN( MSG_ONE, g_netmsg[NETMSG_CLDLLFUNC], NULL, pPlayer->pev );
			WRITE_BYTE( 3 );
			WRITE_BYTE( 1 );
		MESSAGE_END();

		pPlayer->CurrentTransArea = NULL;

		thoth_didvote = false;
		msstringlist Parameters;
		Parameters.add( STRING(sDestName) );
		Parameters.add( STRING(sDestMap) );
		Parameters.add( STRING(sName) );
		pPlayer->CallScriptEvent( "game_transition_exited", &Parameters );
	}

	// MSQuery - Called by CHalfLifeMultiplay::ClientCommand to let me know who voted
	//			 iRequest == index of player
	//           (You can either vote yes or not vote, which means no)
	void *MSQuery( int iRequest )
	{
		CBaseEntity *pGameMasterEnt = UTIL_FindEntityByString( NULL, "netname", msstring("�") + "game_master" );
		IScripted *pGMScript = pGameMasterEnt->GetScripted();
		if ( pGMScript )
		{
			if ( pGMScript->GetFirstScriptVar("GM_DISABLE_TRANSITIONS") == "1" ) return NULL;
		}

		if ( !thoth_didvote )
		{
			/*char thoth_trans_string[64];
			strcpy(thoth_trans_string,"touch_trans_");
			strcat(thoth_trans_string,STRING(sDestMap));
			FireTargets( thoth_trans_string, this, this, USE_TOGGLE, 0 );*/
			//Thothie JAN2008a moving vote system from amx to scripts
			//CBaseEntity *pGameMasterEnt = UTIL_FindEntityByString( NULL, "netname", msstring("�") + "game_master" );
			//IScripted *pGMScript = pGameMasterEnt->GetScripted();

			msstringlist Parameters;
			Parameters.add( STRING(sDestName) );
			Parameters.add( STRING(sDestMap) );
			Parameters.add( STRING(sName) );
			Parameters.add( STRING(sDestTrans) );
			pGMScript->CallScriptEvent( "game_transition_triggered", &Parameters );
			thoth_didvote = true;
		}

		if( !FAllPlayersAreTouchingMe( ) ) return NULL;

		byte PlayerBits = (1<<(iRequest-1));

		//Already voted
		if( FBitSet( PlayerVotes, PlayerBits ) )
			return NULL;

		SetBits( PlayerVotes, PlayerBits );

		//Compare the votes to what it should be
		int AllVotes = 0;
		for( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CBasePlayer *pOtherPlayer = (CBasePlayer *)UTIL_PlayerByIndex( i );
			if( !pOtherPlayer || pOtherPlayer->m_CharacterState == CHARSTATE_UNLOADED || !pOtherPlayer->IsActive() ) continue;
			SetBits( AllVotes, (1<<(i-1)) );
		}

		if( UTIL_NumPlayers() > 1 )
		{		
			SendHUDMsgAll( "Travel", msstring(CBaseEntity::Instance(INDEXENT(iRequest))->DisplayName()) + " wants to go to " + STRING(sDestName) );
		}

		//Everyone voted, switch the map
		if( PlayerVotes == AllVotes )
		{
			UTIL_ClientPrintAll( HUD_PRINTCENTER, UTIL_VarArgs("Traveling to %s\n",STRING(sDestName)) );
			for( int i = 1; i <= gpGlobals->maxClients; i++ )
			{
				CBasePlayer *pOtherPlayer = (CBasePlayer *)UTIL_PlayerByIndex( i );
				if( !pOtherPlayer ) continue;
				if( !pOtherPlayer->IsActive() ) continue;

				msstring dest_map = STRING(sDestMap);
				if ( IS_MAP_VALID(dest_map.c_str()) ) pOtherPlayer->EnableControl( FALSE );

				msstringlist Parameters;
				Parameters.add( STRING(sDestMap) );
				pOtherPlayer->CallScriptEvent( "game_map_change", &Parameters );

				//Thothie JUN2007 make sure all trans stats are set right
				/*
				strcpy( pOtherPlayer->m_OldTransition, STRING(sName) );
				strcpy( pOtherPlayer->m_NextMap, STRING(sDestMap) );
				strcpy( pOtherPlayer->m_NextTransition, STRING(sDestTrans) );
				pOtherPlayer->CurrentTransArea = this;
				pOtherPlayer->m_SpawnTransition = pOtherPlayer->m_OldTransition;
				*/

				//Save character
				pOtherPlayer->SaveChar( );

				if( !MSGlobals::ServerSideChar )
					pOtherPlayer->m_TimeCharLastSent = 0;
			}

			//JAN2008a Give AMX a chance to change map first (has its own delay)
			//char thoth_trans_string[64];
			//strcpy(thoth_trans_string,"force_map_");
			//strcat(thoth_trans_string,STRING(sDestMap));
			//FireTargets( thoth_trans_string, this, this, USE_TOGGLE, 0 );

			//JAN2008a - letting game_master script handle changelevel functions
			//pev->nextthink = pev->ltime + 5.0; //Thothie JUN2007 - added to give more time for char save
			//SetThink( ChangeLevel );
		}

		return NULL;
	}
	void ChangeLevel( )
	{
		//Thothie (note only, no changes)
		//- here we should send ClientCmd disconnect/reconnect sequence for all players
		//- who are not loopback
		//JAN2008a commenting out, letting game_master script handle changelevel functions
		//CHANGE_LEVEL( (char *)STRING(sDestMap), NULL );
	}
	void KeyValue( KeyValueData *pkvd )
	{
		if (FStrEq(pkvd->szKeyName, "destname"))
		{
			sDestName = ALLOC_STRING( pkvd->szValue );
			pkvd->fHandled = TRUE;
		}
		else if (FStrEq(pkvd->szKeyName, "destmap"))
		{
			sDestMap = ALLOC_STRING( pkvd->szValue );
			pkvd->fHandled = TRUE;
		}	
		else if (FStrEq(pkvd->szKeyName, "desttrans"))
		{
			sDestTrans = ALLOC_STRING( pkvd->szValue );
			pkvd->fHandled = TRUE;
		}
		else if (FStrEq(pkvd->szKeyName, "master"))
		{
			ms_master = ALLOC_STRING( pkvd->szValue );
			pkvd->fHandled = TRUE;
		}
		else CBaseEntity::KeyValue( pkvd );
	}
};

LINK_ENTITY_TO_CLASS( msarea_transition, CAreaTransition );


//
//  CAreaNoSave - An area the prevents the client from saving
//

class CAreaNoSave : public CAreaInvisible {
public:
	
	//Player touched me
	BOOL OnControls( entvars_t *pev ) 
	{
		if( !CBaseEntity::Instance(pev)->IsPlayer( ) ) return FALSE;
		
		CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance(pev);
		if( pPlayer->CurrentNoSaveArea == this ) return FALSE;
		pPlayer->CurrentNoSaveArea = this;

		//Let the client know they can no longer save
		MESSAGE_BEGIN( MSG_ONE, g_netmsg[NETMSG_CLDLLFUNC], NULL, pPlayer->pev );
			WRITE_BYTE( 4 );
			WRITE_BYTE( 0 );
		MESSAGE_END();

		return TRUE;
	}
	// DeathNotice - Let the client save again
	void DeathNotice ( entvars_t *pev ) 
	{
		if( !CBaseEntity::Instance(pev)->IsPlayer( ) ) return;

		CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance(pev);
		if( !pPlayer || !pPlayer->CurrentNoSaveArea || pPlayer->CurrentNoSaveArea != this ) return;
		
		MESSAGE_BEGIN( MSG_ONE, g_netmsg[NETMSG_CLDLLFUNC], NULL, pPlayer->pev );
			WRITE_BYTE( 4 );
			WRITE_BYTE( 1 );
		MESSAGE_END();

		pPlayer->CurrentNoSaveArea = NULL;
	}
};

LINK_ENTITY_TO_CLASS( msarea_nosave, CAreaNoSave );

//
//  CAreaTown - An area the prevents the player from attacking
//

class CAreaTown : public CAreaInvisible {
public:
	
	bool m_fAllowPK;

	//Player touched me
	BOOL OnControls( entvars_t *pev ) 
	{
		if( !CBaseEntity::Instance(pev)->IsPlayer( ) ) return FALSE;
		
		CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance(pev);
		if( pPlayer->CurrentTownArea == this ) return FALSE;
		
		pPlayer->CurrentTownArea = this;

		return TRUE;
	}
	// DeathNotice - Player left the area
	void DeathNotice ( entvars_t *pev ) 
	{
		if( !CBaseEntity::Instance(pev)->IsPlayer( ) ) return;

		CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance(pev);
		if( !pPlayer || !pPlayer->CurrentTownArea || pPlayer->CurrentTownArea != this ) return;
		
		pPlayer->CurrentTownArea = NULL;
	}

	//Player tried to attack another player, is this allowed?
	void *MSQuery( int iRequest )
	{
		if( m_fAllowPK ) return (void *)TRUE;
		return (void *)FALSE;
	}

	void KeyValue( KeyValueData *pkvd )
	{
		if (FStrEq(pkvd->szKeyName, "pkill"))
		{
			m_fAllowPK = atoi( pkvd->szValue ) ? true : false;
			pkvd->fHandled = TRUE;
		}
		else CBaseEntity::KeyValue( pkvd );
	}
};

LINK_ENTITY_TO_CLASS( msarea_town, CAreaTown );

//
//  CMonsterBrush - A Brush that's a monster
//

class CMonsterBrush : public CMSMonster {
public:
	void Spawn( )
	{
		pev->angles = g_vecZero;
		pev->movetype = MOVETYPE_PUSHSTEP;
		pev->solid = SOLID_BSP;
		SET_MODEL( ENT(pev), STRING(pev->model) );
		m_Brush = true;

		CMSMonster::Spawn( );
	}
	void Precache( )
	{
		//CMSMonster::Precache( );
	}
	
	void Think( )
	{
		pev->nextthink = BaseThinkTime( ) + 0.01;
		CScriptedEnt::Think( );
	}
	void Touch( CBaseEntity *pOther )
	{
		CScriptedEnt::Touch( pOther );
	}
	void Killed( entvars_t *pevAttacker, int iGib )
	{
		msstringlist Parameters;
		Parameters.add( EntToString(CBaseEntity::Instance(pevAttacker)) );
		CallScriptEvent( "game_death", &Parameters );
	}
};

LINK_ENTITY_TO_CLASS( msarea_scripted, CMonsterBrush );
//-------------------
