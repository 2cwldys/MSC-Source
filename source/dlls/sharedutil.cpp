#include "MSDLLHeaders.h"
#include "Global.h"
#include "logfile.h"
#include "time.h"

#ifndef VALVE_DLL
	#include "../cl_dll/hud.h"
	#include "../cl_dll/cl_util.h"
#endif

CBaseEntity *MSInstance( edict_t *pent );
CMSStream logfile;
CMSStream NullFile;
bool g_log_initialized = false;
void MSErrorConsoleText( const msstring_ref pszLabel, const msstring_ref Progress ) 
{
	if( g_log_initialized )
	{
		msstring Output = "Error ";
		#ifdef VALVE_DLL
			Output += "(SERVER): ";
		#else
			Output += "(CLIENT): ";
		#endif
		Output += pszLabel;
		Output += " --> ";
		Output += Progress;
		Output += "\r\n";
		if( logfile.is_open() )
			logfile << Output;
		Print( Output );
	}
	else
	{
		//This is prety fatal - We got an error before the logs were initialized
		MessageBox( NULL, msstring(pszLabel) + msstring(" (Logs not yet initialized)"), Progress, MB_OK );
	}
}

msstring CMSStream::buffered;
void CMSStream::open( msstring_ref FileName )
{
	ofstream::open( FileName );
	time_t Time;
	time( &Time );
	msstring_ref TimeString = ctime( &Time );
	if( TimeString )
		operator << ( TimeString ) << endl;
	else
		operator << ( "Couldn't get time" ) << endl;

	operator << ( buffered );
	buffered = "";
}
void CMSStream::DebugOpen( )
{		
	//Force open the debug logs whenever the first log event occurs.
	//The event could occur during dll load, before any functions get called
	if( is_open() ) 
		return;

	#ifdef KEEP_LOG
		char cLogfile[MAX_PATH], *pFileName;

		#ifdef VALVE_DLL
			pFileName = "log_msdll";
		#else
			pFileName = "log_cldll";
		#endif

		sprintf( cLogfile, "%s/../%s.txt", MSGlobals::DllPath.c_str(), pFileName );
		try
		{
			logfile.open( cLogfile );
			g_log_initialized = true;
		}
        catch( ... )
		{
		}
	#endif
}

#define ENT_FORMAT ENT_PREFIX "(%i,%u)"
msstring EntToString( class CBaseEntity *pEntity ) //Converts an entity to a string of format "�Pent�P(idx,addr)"
{
	if( !pEntity ) return "";

	static char RetString[32] = "";
	sprintf( RetString, ENT_FORMAT, pEntity->entindex(), (int)pEntity );

	return RetString;
}
CBaseEntity *StringToEnt( msstring_ref EntString ) //Converts an string of format "�Pent�P(idx,addr)" to an entity
{
	int Idx = -1;
	unsigned int Addr = ~0;

	if( sscanf( EntString, ENT_FORMAT, &Idx, &Addr ) < 2 )
		return NULL;

	CBaseEntity *pEntity = MSInstance( INDEXENT(Idx) );
	if( !pEntity || (uint)pEntity != Addr ) return NULL;

	return pEntity;
}
	
msstring_ref VecToString( Vector &Vec ) { static char RetString[128]; sprintf( RetString, "(%.2f,%.2f,%.2f)", Vec.x, Vec.y, Vec.z ); return RetString; }
Vector StringToVec( msstring_ref String )
{
	Vector Vec;

	//This allows you to specify less than all three coordinates... in case you need a 2D vector
	if( sscanf( String, "(%f,%f,%f)", &Vec.x, &Vec.y, &Vec.z ) < 3 )
		if( sscanf( String, "(%f,%f)", &Vec.x, &Vec.y ) < 2 )
			return g_vecZero;
	return Vec;
}	
Color4F StringToColor( msstring_ref String )		//Converts a string of the format "(r,g,b,a)" Color class
{
	Color4F Color( 0, 0, 0, 0 );

	sscanf( String, "(%f,%f,%f,%f)", &Color.r, &Color.g, &Color.b, &Color.a );
	return Color;
}

//Uses Dir.x for right-left, Dir.y for forward-back, and Dir.z as up-down, relative to the angle
Vector GetRelativePos( Vector &Ang, Vector &Dir )
{
	Vector vForward, vRight, vUp, vPosition;

	EngineFunc::MakeVectors( Ang, vForward, vRight, vUp );	//Use the mutal client/server friendly version of this

	vPosition = vRight * Dir.x;
	vPosition += vForward * Dir.y;
	vPosition += vUp * Dir.z;

	return vPosition;
}
//Adds models/ or sprites/ to a model or sprite filename
char *GetFullResourceName( msstring_ref pszPartialName )
{
	msstring PartialName = pszPartialName;
	if( PartialName.len() < 4 )
		return (char *)PartialName;

	static msstring sReturn;

	sReturn = PartialName;
	msstring Extension = &PartialName[PartialName.len()-4];
	if( Extension == ".spr" ) 
		sReturn = msstring("sprites/") + PartialName;
	else if( Extension == ".mdl" ) 
		sReturn = msstring("models/") + PartialName;

	return sReturn;
}

void *MSCopyClassMemory( void *pDest, void *pSource, size_t Length )
{
	long lFirstPtr = *(long *)pDest;
	void *pReturn = CopyMemory( pDest, pSource, Length );
	*(long *)pDest = lFirstPtr;
	return pReturn;
}

void *MSZeroClassMemory( void *pDest, size_t Length )
{
	long lFirstPtr = *(long *)pDest;
	void *pReturn = ZeroMemory( pDest, Length );
	*(long *)pDest = lFirstPtr;
	return pReturn;
}

inline CBaseEntity *PrivData( entvars_t *pev ) { return (CBaseEntity *)pev->pContainingEntity; }

CBaseEntity *MSInstance( entvars_t *pev )
{
	if( !pev ) return NULL;
#ifdef VALVE_DLL
	CBaseEntity *pEnt = GetClassPtr((CBaseEntity *)pev); 
#else
	//In the client DLL, edict() (our parameter) == pev
	CBaseEntity *pEnt = PrivData((entvars_t *)pev);
#endif
	return pEnt; 
}
CBaseEntity *MSInstance( edict_t *pent )
{
	if( !pent ) return NULL;
#ifdef VALVE_DLL
	CBaseEntity *pEnt = (CBaseEntity *)GET_PRIVATE(pent); 
#else
	//In the client DLL, edict() (our parameter) == pev
	CBaseEntity *pEnt = PrivData((entvars_t *)pent);
#endif
	return pEnt; 
}

int iBeam;

Vector GetHighBone( entvars_t *pev, int Bone ) {
	//Bones positions above the waist are way off so this tries to get it close
	//Y needs to be rotated +/- 120 degrees and the
	//engine swaps the X and Z values so you have to compensate for that
//	ALERT( at_console, "x: %f\n", pev->angles.x );
	Vector vTemp1 = Vector(0,0,0);
	float yrot = 124;
	float oldx = pev->angles.x;
	float oldz = pev->angles.z;
	if( oldx < 0 ) pev->angles.x *= 1.2;
	else pev->angles.x /= -16;
	
	pev->angles.y += yrot;
	if( oldx < 0 ) pev->angles.z = -oldx*1.5;
	else pev->angles.z = -oldx;

	GET_BONE_POSITION( ENT(pev), Bone, vTemp1, NULL );
	pev->angles.x = oldx;
	pev->angles.y -= yrot;
	pev->angles.z = oldz;

	return vTemp1; 
}
void BeamEffect( float SRCx, float SRCy, float SRCz, float DESTx,
				float DESTy, float DESTz, int sprite, int startframe,
				int framerate, int life, int width, int noise,
				int r, int g, int b, int brightness, int ispeed ) {
	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, NULL );
		WRITE_BYTE( TE_BEAMPOINTS);
		WRITE_COORD( SRCx ); WRITE_COORD( SRCy ); WRITE_COORD( SRCz );
		WRITE_COORD( DESTx ); WRITE_COORD( DESTy ); WRITE_COORD( DESTz );
		WRITE_SHORT( sprite );
		WRITE_BYTE( startframe ); // startframe
		WRITE_BYTE( framerate ); // framerate
		WRITE_BYTE( life );		//life
		WRITE_BYTE( width );	// width
		WRITE_BYTE( noise );	// noise
		WRITE_BYTE( r ); WRITE_BYTE( g ); WRITE_BYTE( b );
		WRITE_BYTE( brightness );	// brightness
		WRITE_BYTE( ispeed );		// speed
	MESSAGE_END();
}
void BeamEffect( Vector vStart, Vector vEnd, int sprite, int startframe,
				int framerate, int life, int width, int noise,
				int r, int g, int b, int brightness, int ispeed ) {
	BeamEffect( vStart.x, vStart.y, vStart.z, vEnd.x, vEnd.y, vEnd.z, 
		sprite, startframe,framerate, life, width, noise, r, g, b, 
		brightness, ispeed );
}

BOOL UTIL_IsPointWithinEntity( Vector &vPoint, CBaseEntity *pEntity ) {
	if ( vPoint.x > pEntity->pev->absmax.x ||
		 vPoint.y > pEntity->pev->absmax.y ||
		 vPoint.z > pEntity->pev->absmax.z ||
		 vPoint.x < pEntity->pev->absmin.x ||
		 vPoint.y < pEntity->pev->absmin.y ||
		 vPoint.z < pEntity->pev->absmin.z )
		 return FALSE;
	return TRUE;
}
/*int power( int exponent, int basenum ) {
	int i, ireturn = 1;
	for( i = 0; i < exponent; i++ ) ireturn *= basenum;
	return ireturn;
}*/
int numofdigits( int x ) {
	int idigits = 1;
	while( x >= pow(10,idigits) && idigits < 256) idigits++;
	return idigits;
}
void SpriteEffect( CBaseEntity *pEntity, int Effect, char *cSprite ) {
#ifdef VALVE_DLL
	int iSprite = PRECACHE_MODEL( cSprite );
	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );

		WRITE_BYTE( Effect );
		WRITE_SHORT(pEntity->entindex());	// entity
		WRITE_SHORT( iSprite );	// sprite model
		WRITE_BYTE( 20 ); // life
		WRITE_BYTE( 5 );  // width
		WRITE_BYTE( 224 );   // r, g, b
		WRITE_BYTE( 224 );   // r, g, b
		WRITE_BYTE( 255 );   // r, g, b
		WRITE_BYTE( 255 );	// brightness

	MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)
#endif
}


#ifndef _WIN32
	extern "C" char* strlwr( char* str )
	{
		char* orig = str;
		// process the string
		for ( ; *str != '\0'; str++ )
			*str = tolower(*str);
		return orig;
	}
#endif

/*bool string_i::operator == ( msstring_ref a ) const
{ 
	return FStrEq( EngineFunc::GetString( m_string ), a ) ? true : false; 
}*/

//Needed to support sound.cpp
#include "talkmonster.h"
float	CTalkMonster::g_talkWaitTime = 0;		// time delay until it's ok to speak: used so that two NPCs don't talk at once
