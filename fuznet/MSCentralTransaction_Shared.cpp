typedef unsigned char byte;
typedef unsigned long ulong;
typedef unsigned short ushort;

#include <iostream>
#include <tchar.h>
#include <string>
#include "winsock2.h"
#include <fstream>
#include <process.h>
#include "string.h"
#include <ctime>
using namespace std;

#include "./MSCentralTransaction.h"


void CTransaction::HandleMsg( msg_t &Msg )
{
	switch( Msg.Msg )
	{
		case MSG_FILENOTFOUND:
		{
			filenotfound_t &MsgNotFound = (filenotfound_t &)Msg;
			Error_FileNotFound( (msg_e)Msg.Msg, MsgNotFound.AuthID, MsgNotFound.CharNum );
			//no break
		}
		case MSG_DISC:
		{
			Disconnect( );
			break;
		}
		case MSG_STORFILE:
		{
			if( !m_Validated )
			{
				ValidationFailed( );
				break;
			}
			storechar_t &MsgChar = (storechar_t &)Msg;
			ReceivedChar( MsgChar.AuthID, MsgChar.CharNum, (char *)MsgChar.CharData, MsgChar.CharSize );
			SendDisconnect( );

			break;
		}
	}
}

void CTransaction::SendChar( const char *AuthID, int CharNum, const char *Data, int DataLen )
{
	int MsgLength = (sizeof(storechar_t) - 1 ) + DataLen;
	char *Buffer = new char[MsgLength];

	storechar_t &MsgStorChar = *(storechar_t *)Buffer;
	MsgStorChar.Msg = MSG_STORFILE;
	MsgStorChar.Length = MsgLength;
	strncpy( MsgStorChar.AuthID, AuthID, sizeof(MsgStorChar.AuthID) );
	MsgStorChar.CharNum = CharNum;
	MsgStorChar.CharSize = DataLen;
	MsgStorChar.CentralServerTime = time(NULL); // MiB 06DEC_2014 - Central server's time
												//					(If this is sent from the DLL, FN ignores it)

	memcpy( &MsgStorChar.CharData[0], Data, DataLen );

	int Amt = send( m_Socket, Buffer, MsgStorChar.Length, 0 );
	if( Amt < (signed)MsgStorChar.Length )
	{
		int stop = 0;
		//Print( 3, "Send Error.  Sent %i, Returned %i", MsgStorChar.Length, Amt );
	}
	delete Buffer;
}
void CTransaction::SendDisconnect( )
{
	disconnect_t MsgDisc;
	MsgDisc.Msg = MSG_DISC;
	MsgDisc.Length = sizeof(disconnect_t);
	MsgDisc.CentralServerTime = time(NULL); // MiB 06DEC_2014 - Central server's time
											//					(If this is sent from the DLL, FN ignores it)
	send( m_Socket, (const char *)&MsgDisc, MsgDisc.Length, 0 );

	Disconnect( );
}

void CTransaction::Run( )
{
	char *Buffer = new char[BUFFER_SIZE];
	size_t BufferLen = 0;
	int DataLen = 0;

	while( (DataLen = recv( m_Socket, &Buffer[BufferLen], 4096, 0 )) )
	{
		if( DataLen == SOCKET_ERROR || BufferLen + DataLen > BUFFER_SIZE )
			break;

		BufferLen += DataLen;

		while( BufferLen >= sizeof(msg_t) && BufferLen >= ((msg_t *)Buffer)->Length )
		{
			msg_t &Msg = *(msg_t *)Buffer;
			size_t Length = Msg.Length;
			HandleMsg( Msg );
			memmove( Buffer, &Buffer[Length], BufferLen - Length );
			BufferLen -= Length;
			if( m_Disconnected )
				break;
		}

		if( m_Disconnected )
			break;
	}

	Disconnect( );
	delete Buffer;
}

void CTransaction::Disconnect( )
{
	if( !m_Disconnected )
	{
		m_Disconnected = true;
		shutdown( m_Socket, SD_SEND );
		closesocket( m_Socket );

		Disconnected( );
	}
}

