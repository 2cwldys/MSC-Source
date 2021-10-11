// MSCentral.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include ".\MSCentralTransaction.h"

CMSCentral MSCentral;
CRITICAL_SECTION g_CriticalSection;
ofstream log;
string curLogFileName;

int _tmain( int argc, _TCHAR* argv[] )
{
	InitializeCriticalSection( &g_CriticalSection );

	MSCentral.Init( argc, argv );

	DeleteCriticalSection( &g_CriticalSection );
	return 0;
}

CMSCentral::CMSCentral(void)
{
	m_Port = CENTRAL_DEFAULT_PORT;
	m_DebugLevel = 0;
	m_MaxThreads = 10;
	m_Threads = 0;
}

void CMSCentral::Init( int argc, _TCHAR* argv[] )
{
	//MIB FEB2008a
	string timeStamp;
	char cDate[128];
	_strdate( cDate );
	ReplaceChar( cDate , '/', '-' );
	timeStamp = cDate;
	timeStamp += "_log.txt";
	log.open( timeStamp.c_str(), ios_base::app );
	//[/mib]

	curLogFileName = timeStamp;


	Print( "Master Sword Central Server v%.2f.", VERSION );
	Print( "CTRL-C to Quit." );
	Print( "" );


	WSADATA wsaData;
	WSAStartup( MAKEWORD( 2, 2 ), &wsaData );
	
	LoadOptions( );
	Print( "Options Loaded." );

	m_ListenSocket = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );

	sockaddr_in AddrIn;
	AddrIn.sin_family = AF_INET;
	AddrIn.sin_port = htons(m_Port);
	AddrIn.sin_addr.s_addr = htonl(INADDR_ANY);
	memset( &AddrIn.sin_zero, 0, 8 );
	int Success = bind( m_ListenSocket, (sockaddr *)&AddrIn, sizeof(sockaddr) );

	if( Success != SOCKET_ERROR )
		Print( "Listening on port %i...", m_Port );
	else
	{
		Print( "Listening failed.  Could not to bind to port %s", m_Port );
	}

	fd_set FDSet;
	FDSet.fd_count = 1;
	FDSet.fd_array[0] = m_ListenSocket;

	listen( m_ListenSocket, 32 );

	do
	{
		if( select( 0, &FDSet, NULL, NULL, NULL ) > 0 )
		{
			THREADLOCK;
			if( m_Threads < m_MaxThreads )
			{
				//Incoming connection accepted
				Transaction_Accept( );
			}
			else
			{
				//Rejected
				int AddrSize = sizeof(sockaddr);
				sockaddr_in AddrIn;

				SOCKET socket = accept( MSCentral.m_ListenSocket, (sockaddr *)&AddrIn, &AddrSize );
				closesocket( socket );
			}
			THREADUNLOCK;
		}
	}
	while( 1 );

	log.close( );
	char a;
	cin.get( a );

	WSACleanup( );
}

void CMSCentral::LoadOptions( )
{
	ifstream file( "options.txt" );
	char Option[256], Value[256];

	m_NetWorkName = "Unnamed Network";
	m_Port = CENTRAL_DEFAULT_PORT;
	m_MaxThreads = 10;

	if( file.is_open() )
	{
		while( ReadFileLine( file, Option, Value ) )
		{
			if( !stricmp( Option, "NetworkName" ) )
			{
				m_NetWorkName = Value;
			}
			else if( !stricmp( Option, "Password" ) )
			{
				m_Password = Value;
			}		
			else if( !stricmp( Option, "SaveDir" ) )
			{
				m_SaveDir = Value;
			}
			else if( !stricmp( Option, "Port" ) )
			{
				m_Port = atoi(Value);
			}
			else if( !stricmp( Option, "DebugLevel" ) )
			{
				m_DebugLevel = atoi(Value);
			}
			else if( !stricmp( Option, "MaxThreads" ) )
			{
				m_MaxThreads = atoi(Value);
				if( m_MaxThreads <= 0 )
				{
					Print( "Invalid MaxThreads, defaulting to 10..." );
					m_MaxThreads = 10;
				}
			}
		}

		file.close( );
	}

	file.open( "motd.txt" );
	if( file.is_open() )
	{
		char Buffer[MOTD_SIZE];
		file.read( Buffer, MOTD_SIZE );
		m_MOTD = Buffer;
		file.close( );
	}
}
bool CMSCentral::ReadFileLine( ifstream &file, char *OptionName, char *OutValue )
{
	char Buffer[4096];
	char *ReadPtr = Buffer;
	OptionName[0] = 0;
	OutValue[0] = 0;

	do {
		file.getline( Buffer, 4096 );
	}
	while( !file.eof() && strlen(Buffer) >= 2 && Buffer[0] == '/' && Buffer[1] == '/' );

	if( file.eof() )
		return false;
	
	size_t Length = strlen(Buffer);
	size_t i = 0;
	for( ; i < Length; i++, ReadPtr++ )
		if( !isspace(ReadPtr[0]) )
			break;

	char *EqualsSign = strstr(ReadPtr,"=");
	if( !EqualsSign )
		return true;

	size_t OptionLength = EqualsSign - ReadPtr;

	strncpy( OptionName, ReadPtr, OptionLength );
	OptionName[OptionLength] = 0;

	ReadPtr = EqualsSign + 1;
	size_t ValueLength = strlen( ReadPtr );
	if( !ValueLength )
		return true;

	for( i = 0; i < ValueLength; i++, ReadPtr++ )
		if( !isspace(ReadPtr[0]) )
			break;

	strcpy( OutValue, ReadPtr );
	return true;
}

void Print( char *szFormat, ... ) 
{
	//mass changes by MIB FEB2008a
	//- MiB says, "Curse out the fuckwit that decided to name his char array "string" (changed to STRING) >_<"
	va_list		argptr;
	char		STRING[4096];
	
	va_start( argptr, szFormat );
	vsprintf( STRING, szFormat, argptr );
	va_end( argptr );

	string timeStamp;
	char cDate[128];
	_strdate( cDate );
	ReplaceChar( cDate , '/', '-' );
	timeStamp = cDate;
	timeStamp += "_log.txt";
	if( strcmp( timeStamp.c_str() , curLogFileName.c_str() ) != 0 )
	{
		char cTime[128];
		_strtime( cTime );
		log.close();
		log.open( timeStamp.c_str(), ios_base::app );
		curLogFileName = timeStamp;
		Print("%s %s New log file opened", cDate, cTime);
	}

	THREADLOCK;	
	cout << STRING << endl;
	log << STRING << endl;
	THREADUNLOCK;
}

void DbgPrint( int DebugLevel, char *szFormat, ... ) 
{ 
	if( MSCentral.m_DebugLevel < DebugLevel )
		return;

	va_list	argptr;
	va_start( argptr, szFormat );

	Print( szFormat, argptr );
}

CMSCentral::~CMSCentral(void)
{
}


void Critical( )
{
	EnterCriticalSection( &g_CriticalSection );
}
void EndCritical( )
{
	LeaveCriticalSection( &g_CriticalSection );
}

void ReplaceChar( char *pString, char org, char dest ) 
{
	int i = 0;
	while( pString[i] ) {
		if( pString[i] == org ) pString[i] = dest;
		i++;
	}
}