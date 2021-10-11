#pragma once

typedef unsigned char byte;
typedef unsigned long ulong;
typedef unsigned short ushort;

class CMSCentral
{
public:
	CMSCentral( void );
	~CMSCentral( void );

	void Init( int argc, _TCHAR* argv[] );
	void LoadOptions( );
	bool ReadFileLine( ifstream &file, char *Option, char *Value );

	string m_NetWorkName, m_MOTD, m_Password, m_SaveDir;
	ushort m_Port;
	SOCKET m_ListenSocket;
	int m_DebugLevel;
	int m_MaxThreads;
	int m_Threads;
};

extern CMSCentral MSCentral;

void Thread_Transaction( void *dummy );

void Print( char *szFormat, ... );
void DbgPrint( int DebugLevel, char *szFormat, ... );

#define THREADLOCK Critical( )
#define THREADUNLOCK EndCritical( )
void Critical( );
void EndCritical( );
void ReplaceChar( char *pString, char org, char dest );

#define VERSION 1.0
