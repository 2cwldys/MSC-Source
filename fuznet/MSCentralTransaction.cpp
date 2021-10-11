#include "stdafx.h"
#include ".\MSCentralTransaction.h"

class CTransaction_CENTRAL : public CTransaction
{
public:
	void Accept( );

	//Scoped
	virtual void HandleMsg( msg_t &Msg );
	virtual void ReceivedChar( const char *AuthID, int CharNum, char *Data, int DataLen );
	virtual void SentChar( const char *AuthID, int CharNum );
	virtual void DeleteChar( const char *AuthID, int CharNum );
	virtual void SendCharSuccess( const char *AuthID, int CharNum, ushort OrigMsg );
	virtual void Error_FileNotFound( msg_e MsgID, const char *AuthID, int CharNum );
	virtual void ValidationFailed( bool disp = true ); //MIB JAN2010_15
	virtual void Disconnected( );
	void Print( char *szFormat, ... );
	void DbgPrint( int DebugLevel, char *szFormat, ... );

	int m_ID;
};

void Transaction_Accept( )
{
	CTransaction_CENTRAL *Trans = new CTransaction_CENTRAL;
	Trans->Accept( );
	_beginthread( Thread_Transaction, 0, Trans );
}
void CTransaction_CENTRAL::Accept( )
{
	m_ID = MSCentral.m_Threads;
	int AddrSize = sizeof(sockaddr);
	m_Socket = accept( MSCentral.m_ListenSocket, (sockaddr *)&m_AddrIn, &AddrSize );

	m_RemoteIP = inet_ntoa(m_AddrIn.sin_addr);
	DbgPrint( 3, "Incoming Connection [%i/%i]", m_ID+1, MSCentral.m_MaxThreads );
}
void CTransaction_CENTRAL::Print( char *szFormat, ... )
{
	va_list		argptr;
	char		string[4096];
	
	va_start( argptr, szFormat );
	vsprintf( string, szFormat, argptr );
	va_end( argptr );

	//time_t CurrentTime;
	//time( CurrentTime
	
	char cDate[128], cTime[128];
	_strdate( cDate );
	_strtime( cTime );

	::Print( "%s %s [%s][%i]\t%s", cDate, cTime, m_RemoteIP.c_str(), m_ID, szFormat );
}
void CTransaction_CENTRAL::DbgPrint( int DebugLevel, char *szFormat, ... ) 
{ 
	if( MSCentral.m_DebugLevel < DebugLevel )
		return;

	va_list		argptr;
	char		string[4096];
	
	va_start( argptr, szFormat );
	vsprintf( string, szFormat, argptr );
	va_end( argptr );

	Print( string );
}

//MIB JAN2010_15
int VerifyMap( char * smap , long ssize )
{
	//Save FN a bit of work if the server already knows it has a good/bad map
	if ( !strcmp( smap, "MAP_VERIFIED" ) )
		return true;
	else if ( !strcmp( smap, "BAD_MAP" ) )
		return false;

	string fileloc = MSCentral.m_SaveDir;
	fileloc += "\\";
	fileloc += "mapsizes.txt";

	bool FoundAMap = false;

	ifstream file( fileloc.c_str() );
	if ( file.is_open() )
	{
		while ( !file.eof() )
		{
			char fmap[512];
			long fsize;
			file >> fmap >> fsize;
			//-1 on FN lets the map go no matter what
			//Don't return false on mismatched sizes, as there may be more than one
			//size per map (allowing people to update). Returns false later.
			if ( !stricmp( fmap, smap ) )
			{
				if ( fsize == ssize || fsize == -1 ) 
					return 1;

				FoundAMap = true; //We found a map with the right name but wrong size
								  //For error notification - Wrong Size vs Illegal Map
			}
		}
	}

	return FoundAMap ? -1 : -2;
}

void CTransaction_CENTRAL::HandleMsg( msg_t &Msg )
{
	switch( Msg.Msg )
	{
		//MIB FEB2010
		//Old password message - Instantly deny it.
		case MSG_PASSWORD:
		{
			DbgPrint( 4, "Old password-message denied" );
			ValidationFailed(false);
			break;
		}

		//MIB FEB2010
		case MSG_PASSWORD_NEW:
		{
			password_t &MsgPass = (password_t &)Msg;

			int map_ver = VerifyMap( MsgPass.mapName , MsgPass.mapSize );
			bool CMap = map_ver == 1;
			bool CPass = MSCentral.m_Password == MsgPass.Password;
			if ( CMap && CPass )
			{
				DbgPrint( 4, "Password and Map Validated" );
				m_Validated = true;
			}
			else
			{
				if ( !CMap )
				{
					//Only send this until the server "knows" the map is bad
					if ( strcmp( MsgPass.mapName , "BAD_MAP" ) )
					{
						error_t e;
						e.Msg = MSG_ERROR;
						e.flags = ( ERR_POPUP | ERR_BADMAP );
						e.Length = sizeof( e );
						sprintf( e.error_msg, ( map_ver == -1 ? "Your map differs from %s's" : "This map is not approved by %s" ), MSCentral.m_NetWorkName.c_str() );
						send( m_Socket, (const char *)&e, e.Length, 0 );
						if ( map_ver == -1 ) DbgPrint( 0, "Wrong map size: %s %i", MsgPass.mapName, MsgPass.mapSize );
						else DbgPrint( 0, "Illegal Map: %s", MsgPass.mapName );
					}
				}

				ValidationFailed(!CPass);
			}
			//Old way
			/*if( MSCentral.m_Password == MsgPass.Password )
			{
				DbgPrint( 4, "Password Validated" );
				m_Validated = true;
			}
			else
			{
				ValidationFailed( );
			}*/
			break;
		}
		//MIB JUN2010_18 - restore char delete func on FN
		case MSG_DELEFILE:
		{
			if( !m_Validated )
			{
			ValidationFailed( );
			break;
			}
			deletechar_t &MsgDeleChar = (deletechar_t &)Msg;
			DeleteChar( MsgDeleChar.AuthID, MsgDeleChar.CharNum );
			break;
		}
		case MSG_RETRINFO:
		{
			if( !m_Validated )
			{
				ValidationFailed( );
				break;
			}
			networkinfo_t MsgInfo;
			MsgInfo.Msg = MSG_INFO;
			MsgInfo.Length = sizeof(networkinfo_t);
			strncpy( MsgInfo.NetworkName, MSCentral.m_NetWorkName.c_str(), sizeof(MsgInfo.NetworkName) );
			strncpy( MsgInfo.MOTD, MSCentral.m_MOTD.c_str(), sizeof(MsgInfo.MOTD) );
			DbgPrint( 2, "Info Requested..." );

			MsgInfo.CentralServerTime = time(NULL); // MiB 06DEC_2014 - Central server's time

			send( m_Socket, (const char *)&MsgInfo, MsgInfo.Length, 0 );
			break;
		}
		case MSG_RETRFILE:
		{
			if( !m_Validated )
			{
				ValidationFailed( );
				break;
			}
			retrchar_t &MsgRetrChar = (retrchar_t &)Msg;

			int CharNum = MsgRetrChar.CharNum;

			string PathName;
			GetCharFileName( MsgRetrChar.AuthID, MsgRetrChar.CharNum, PathName );

			ifstream file( PathName.c_str(), ios::binary );
			if( file.is_open( ) )
			{
				file.seekg( 0, ios::end );
				int FileSize = file.tellg( );
				file.seekg( 0, ios::beg );

				char *Buffer = new char[BUFFER_SIZE];
				file.read( Buffer, FileSize );
				file.close( );

				SendChar( MsgRetrChar.AuthID, CharNum, Buffer, FileSize );
				SentChar( MsgRetrChar.AuthID, CharNum );

				delete Buffer;
			}
			else
			{
				filenotfound_t MsgNotFound;
				MsgNotFound.Length = sizeof(filenotfound_t);
				MsgNotFound.Msg = MSG_FILENOTFOUND;
				strncpy( MsgNotFound.AuthID, MsgRetrChar.AuthID, sizeof(MsgNotFound.AuthID) );
				MsgNotFound.CharNum = CharNum;
				MsgNotFound.CentralServerTime = time(NULL); // MiB 06DEC_2014 - Central server's time

				send( m_Socket, (const char *)&MsgNotFound, MsgNotFound.Length, 0 );
				Error_FileNotFound( (msg_e)Msg.Msg, MsgRetrChar.AuthID, MsgRetrChar.CharNum );
				SendDisconnect( );
			}
			break;
		}
		case MSG_WRITEFNFILE: //MIB FEB2008a
		{
			fnfilewrite_t &FileMsg = (fnfilewrite_t &)Msg;
			char type = FileMsg.type;
			bool o = false;
			switch( type )
			{
				case 'a':
				{
					ofstream outfile;
					outfile.open( FileMsg.FileName, ios_base::app );
					outfile << FileMsg.line << endl;
					outfile.close();
					break;
				}
				case 'o':
					o = true;
				case 'i':
				{
					string mainFile = FileMsg.FileName;
					string tempFile = mainFile;
					tempFile += "TEMP";

					ifstream mainIn;
					mainIn.open( mainFile.c_str() );
					if( mainIn.is_open() )
					{
						ofstream tempOut;
						tempOut.open( tempFile.c_str() );

						::Print("File existed");

						bool wrote = false;
						int count = 0;
						do
						{
							char line[512];
							mainIn.getline( line , 512 );
							::Print("Pulled in %s as line %i", line , count );
							if( count == FileMsg.lineNum )
							{
                                wrote = true;
								if( count ) tempOut << endl;
								tempOut << FileMsg.line;
								if( !o )
									tempOut << endl << line;
							}
							else
							{
								if( count ) tempOut << endl;
								tempOut << line;
							}

							++count;
						}while( !mainIn.eof() );

						//Likely because someone specified an "illegal" line index
						if( !wrote ) tempOut << FileMsg.line << endl;

						tempOut.close();
						mainIn.close();

						ifstream tempIn;
						tempIn.open( tempFile.c_str() );

						ofstream mainOut;
						mainOut.open( mainFile.c_str() );

						bool wrotefirst = false;
						while( !tempIn.eof() )
						{
							char line[512];
							tempIn.getline( line , 512 );
							if( wrotefirst ) mainOut << endl;
							mainOut << line;
							wrotefirst = true;
						}

						tempIn.close();
						mainIn.close();

						std::remove( tempFile.c_str() );
					}
					else
					{
						mainIn.close();
						ofstream mainOut;
						mainOut.open( mainFile.c_str() );
						mainOut << FileMsg.line << endl;
						mainOut.close();
					}
				}
			}
			SendDisconnect();
		
			break;
		}
		case MSG_READFNFILE:
		{
			//MiB Feb2008a
			fnfileread_t &ReadFile = (fnfileread_t &)Msg;
			ifstream file( ReadFile.FileName );
			if( file.is_open() )
			{
				file.seekg(0,ios_base::end);
				long DataLen = file.tellg();
				file.seekg( 0 , ios_base::beg );
				char *Data = new char[DataLen];
				file.read( Data, DataLen );
				file.close();

				int MsgLength = (sizeof(fnfileread_t) - 1 ) + DataLen;
				char *Buffer = new char[MsgLength];
				fnfileread_t &ReadOut = *(fnfileread_t *)Buffer;
				strcpy( ReadOut.EntString, ReadFile.EntString );
				strcpy( ReadOut.FileName, ReadFile.FileName );
				ReadOut.Msg = MSG_READFNFILE;
				ReadOut.Length = MsgLength;
				ReadOut.DataSize = DataLen;
				ReadOut.fileExists = 1;
				ReadOut.CentralServerTime = time(NULL); // MiB 06DEC_2014 - Central server's time
				memcpy( &ReadOut.Data[0], Data, DataLen );
				
				send( m_Socket, Buffer, MsgLength, 0 );

				delete Buffer;
			}
			else
			{
				ReadFile.fileExists = 0;
				ReadFile.CentralServerTime = time(NULL); // MiB 06DEC_2014 - Central server's time
				send( m_Socket, (const char *)&ReadFile, sizeof( ReadFile ), 0 );
			}

			SendDisconnect();

			break;
		}
	}

	CTransaction::HandleMsg( Msg );
}

//MIB JAN2010_15 (mods)
void CTransaction_CENTRAL::ValidationFailed( bool disp )
{
	if( !m_Disconnected && disp )
		DbgPrint( 0, "Bad Password" );
	SendDisconnect( );
}

void CTransaction_CENTRAL::ReceivedChar( const char *AuthID, int CharNum, char *Data, int DataLen )
{
	string PathName;
	GetCharFileName( AuthID, CharNum, PathName );

	ofstream file( PathName.c_str(), ios::binary );
	file.write( (char *)Data, DataLen );

	file.close( );

	SendCharSuccess( AuthID, CharNum, MSG_STORFILE );

	DbgPrint( 1, "Char Updated: [%s][%i]", AuthID, CharNum );
}
void CTransaction_CENTRAL::SentChar( const char *AuthID, int CharNum )
{
	DbgPrint( 1, "Char Requested: [%s][%i]", AuthID, CharNum );
}
void CTransaction_CENTRAL::DeleteChar( const char *AuthID, int CharNum )
{
	//Delete char file
	string PathName;
	GetCharFileName( AuthID, CharNum, PathName );
	remove( PathName.c_str() );

	SendCharSuccess( AuthID, CharNum, MSG_DELEFILE );
}
void CTransaction_CENTRAL::SendCharSuccess( const char *AuthID, int CharNum, ushort OrigMsg )
{
	charsuccess_t MsgOut;
	MsgOut.Msg = MSG_CHAR_SUCCESS;
	MsgOut.Length = sizeof(charsuccess_t);
	MsgOut.OrigMsg = OrigMsg;
	strncpy( MsgOut.AuthID, AuthID, sizeof(MsgOut.AuthID) );
	MsgOut.CharNum = CharNum;
	MsgOut.CentralServerTime = time(NULL); // MiB 06DEC_2014 - Central server's time

	send( m_Socket, (const char *)&MsgOut, MsgOut.Length, 0 );
}

void CTransaction_CENTRAL::Error_FileNotFound( msg_e MsgID, const char *AuthID, int CharNum )
{
	DbgPrint( 2, "Char Requested: NOT FOUND [%s][%i]", AuthID, CharNum );
}

void CTransaction_CENTRAL::Disconnected( )
{
	THREADLOCK;	
	DbgPrint( 3, "Disconnected [%i/%i]", MSCentral.m_Threads, MSCentral.m_MaxThreads );
	THREADUNLOCK;
}

void CTransaction::GetCharFileName( const char *AuthID, int iCharacter, string &FileName )
{
	char cFileName[MAX_PATH];

	//Server
	sprintf( cFileName, "%s/%s_%i.char", MSCentral.m_SaveDir.c_str(), AuthID, iCharacter+1 );
	ReplaceChar( cFileName, ':', '-' );

	FileName = cFileName;
}


void Thread_Transaction( void *dummy )
{
	CTransaction_CENTRAL &Trans = *(CTransaction_CENTRAL *)dummy;

	THREADLOCK;	
	MSCentral.m_Threads++;
	THREADUNLOCK;

	Trans.Run( );

	THREADLOCK;	
	MSCentral.m_Threads--;
	THREADUNLOCK;
}

