#ifndef SHAREDUTIL_H
#define SHAREDUTIL_H

#include "buildcontrol.h"

//Extentions
#define SCRIPT_EXT ".script"

typedef unsigned char  uchar;
typedef unsigned short ushort;
typedef unsigned int   uint;
typedef unsigned long  ulong;	//All item IDs are ulong

typedef struct COLOR_T {
	COLOR_T( ) { r = 0; g = 0; b = 0; a = 0; }
	COLOR_T( uchar R, uchar G, uchar B, uchar A ) { r = R; g = G; b = B; a = A; }
	COLOR_T( ulong a ) { r = ((uchar *)&a)[0]; g = ((uchar *)&a)[1]; b = ((uchar *)&a)[2]; a = ((uchar *)&a)[3]; }
	operator ulong ( ) { ulong l; ((uchar *)&l)[0] = r; ((uchar *)&l)[1] = g; ((uchar *)&l)[2] = b; ((uchar *)&l)[3] = a; return l; }
	unsigned char r,g,b,a;
} COLOR;
#define COLORTORGBA( color )  MakeRGBA( color.r, color.g, color.b, color.a )

class Color4F	//For opengl or when you need colors in the [0-1] range
{
public:
	Color4F( ) { r = g = b = a = 0; }
	Color4F( float r, float g, float b, float a ) { Color4F::r = r, Color4F::g = g, Color4F::b = b; Color4F::a = a; }
	float &operator [] ( int Idx ) { return *(&r + Idx); }
	operator float * ( ) { return &r; }

	float r, g, b, a;
};

#define MACRO_CREATEITEM( item ) (CBaseEntity *)GET_PRIVATE(CREATE_NAMED_ENTITY(MAKE_STRING(item)))
#define foreach( var, max ) for( int var = 0; var < (signed)max; var++ )	//variable declared
#define Foreach( var, max ) for( var = 0; var < (signed)max; var++ )		//variable not declared
#define clrmem( a ) memset( &a, 0, sizeof(a) );
int numofdigits( int x );
void Print( char *szFmt, ... );
#define FloatToString( a ) UTIL_VarArgs( "%.2f", a )
#define IntToString( a ) UTIL_VarArgs( "%i", a )
#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

#ifndef _WIN32
	extern "C" char* strlwr( char* str );
#endif

#include "stackstring.h"
#include "msdebug.h"
#include "mem_encrypt.h"

#ifdef VECTOR_H
	msstring_ref VecToString( Vector &Vec );			//Converts a vector to a string of format "(x,y,z)"
	Vector StringToVec( msstring_ref String );			//Converts a string of the format "(x,y)" or "(x,y,z)" to a Vector class
	Color4F StringToColor( msstring_ref String );		//Converts a string of the format "(r,g,b,a)" Color class
	Vector GetRelativePos( Vector &Ang, Vector &Dir );	//Uses Dir.x for right-left, Dir.y for forward-back, and Dir.z as up-down, relative to the angle
#endif

#define ENT_PREFIX "�Pent�P"
msstring EntToString( class CBaseEntity *pEntity );			//Converts an entity to a string of format "�Pent�P(idx,addr)"
CBaseEntity *StringToEnt( msstring_ref EntString);			//Converts an string of format "�Pent�P(idx,addr)" to an entity


void WRITE_FLOAT( float Float );
char *GetFullResourceName( msstring_ref PartialName );	//Adds models/ or sprites/ to a model or sprite filename
void dbgtxt( msstring_ref Text );


#ifdef CONST_H
	//Data used in the client item delta comparison
	#include <pshpack1.h>
	struct genericitem_t
	{
		genericitem_t( ) { }
		genericitem_t( class CGenericItem *pItem );
		operator class CGenericItem *( );
		ulong ID;
		string_i Name;						//Itemname
		ulong Properties;
		unsigned short Quantity;
		unsigned short Quality, MaxQuality;
	};
	#include <poppack.h>
#endif

#endif SHAREDUTIL_H