#include "stackstring.h"
#include "string"
using namespace std;

#undef msstring
#define MSSTRING_MAXLEN (MSSTRING_SIZE-1)
msstring::msstring( )											{ data[0] = 0; }
msstring::msstring( const msstring_ref a )						{ operator = ( a ); }
msstring::msstring( const msstring_ref a, size_t length )		{ strncpy( data, a, length ); data[length] = 0; }
msstring::msstring( const msstring &a )							{ operator = ( a ); }
msstring &msstring::operator = ( const msstring_ref a )			{ if( a == data ) return *this; data[0] = 0; append( a ); return *this; }
msstring &msstring::operator = ( int a )						{ sprintf( data, "%i", a ); return *this; }
msstring &msstring::operator = ( const msstring &a )			{ return operator = ( a.data ); }
msstring &msstring::operator += ( const msstring_ref a )		{ append( a ); return *this; }
msstring &msstring::operator += ( int a )						{ msstring tmp; tmp = a; return operator += ( (const msstring_ref &)(tmp) ); }
msstring msstring::operator + ( const msstring_ref a )			{ return msstring( data ) += a; }
msstring msstring::operator + ( msstring &a )					{ return msstring( data ) += (const char *)a; }
msstring msstring::operator + ( int a )							{ return msstring( data ) += a; }
bool msstring::operator == ( char *a ) const					{ return !strcmp( data, a ); }
bool msstring::operator == ( const char *a ) const				{ return !strcmp( data, (const char *)a ); }
msstring::operator char * ( )									{ return data; }
char *msstring::c_str( )										{ return data; }
void msstring::append( const msstring_ref a, size_t length )	{ size_t my_sz = len( ); size_t capped_sz = min(length,MSSTRING_MAXLEN-my_sz); if( capped_sz <= 0 ) return; strncpy( &data[my_sz], a, capped_sz ); data[my_sz+capped_sz] = 0; }
void msstring::append( const msstring_ref a )					{ size_t len = strlen(a); append( a, len ); }
size_t msstring::len( )	const									{ return strlen(data); }
size_t msstring::find( const msstring_ref a, size_t start )	const				{ msstring_ref substring = strstr(&data[start],a);  return substring ? (substring - &data[start]) : msstring_error; }
msstring_ref msstring::find_str( const msstring_ref a, size_t start ) const		{ size_t ret = find( a, start );  return (ret != msstring_error) ? &data[ret] : &data[start]; }
size_t msstring::findchar( const msstring_ref a, size_t start )	const			{ for( int i = start; i < (signed)len(); i++ ) { char datachar[2] = {data[i], 0}; if( strstr(a,datachar) ) return i - start; } return msstring_error; }
msstring_ref msstring::findchar_str( const msstring_ref a, size_t start ) const	{ size_t ret = findchar( a, start );  return (ret != msstring_error) ? &data[ret] : &data[start]; }
bool msstring::contains( const msstring_ref a )	const							{ return find( a ) != msstring_error; }
bool msstring::starts_with( const msstring_ref a )	const						{ return find( a ) == 0; }
bool msstring::ends_with( const msstring_ref a ) const							{ msstring temp = a; int loc = len( ) - temp.len(); return loc == find( temp ); }
msstring msstring::substr( size_t start, size_t length )						{ return msstring( &data[start], length ); }
msstring msstring::substr( size_t start )										{ return msstring( &data[start] ); }
msstring msstring::thru_substr( const msstring_ref a, size_t start ) const		{ size_t ret = find( a, start );  return (ret != msstring_error) ? msstring( &data[start], ret ) : &data[start]; }
msstring msstring::thru_char( const msstring_ref a, size_t start ) const		{ size_t ret = findchar( a, start );  return (ret != msstring_error) ? msstring( &data[start], ret ) : &data[start]; }
msstring msstring::skip( const msstring_ref a ) const							{ size_t my_sz = len( ); foreach( i, my_sz ) { char datachar[2] = {data[i], 0}; if( !strstr(a,datachar) ) return msstring( &data[i], my_sz-i); } return &data[my_sz]; }
//#define msstring str256


bool TokenizeString( const char *pszString, msstringlist &Tokens, msstring_ref Separator )
{
	char cTemp[MSSTRING_SIZE-1];
	int i = 0;
	bool AnyFound = false;
	msstring ParseStr = "%[^";
	ParseStr += Separator;
	ParseStr += "]";
	while( sscanf( &pszString[i], ParseStr, cTemp ) > 0 )
	{
		i += strlen(cTemp);
		Tokens.add( cTemp );
		AnyFound = true;

		if( pszString[i] ) i++;	//Hit a semi-colon, continue
	}
	return AnyFound;
}

void ReplaceChar( char *pString, char org, char dest ) 
{
	int i = 0;
	while( pString[i] ) {
		if( pString[i] == org ) pString[i] = dest;
		i++;
	}
}


//
//	msvariant
//

msvariant::msvariant( ) { clrmem( *this ); }

void msvariant::SetFromString( msstring_ref a )
{
	m_String = a;
	m_Int = atoi( a );
	m_Float = (float)atof( a );
}
void msvariant::SetFromInt( int a )
{
	m_String = "";
	m_String += a;
	m_Int = a;
	m_Float = (float)a;
}
void msvariant::SetFromFloat( float a )
{
	sprintf( m_String.c_str(), "%f", a );
	m_Int = (int)a;
	m_Float = a;
}
