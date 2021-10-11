#define MOTD_SIZE	4096
#define BUFFER_SIZE (1<<20)
#define CENTRAL_DEFAULT_PORT 5710


#include <pshpack1.h>
struct msg_t
{
	ushort Msg;
	ulong Length;
	time_t CentralServerTime; // MiB 06DEC_2014 - Central server's time for holiday events and such
};
struct filemsg_t : msg_t
{
	char AuthID[256];
	int CharNum;
};

struct password_t : msg_t
{
	char Password[128];
	char mapName[128];
	long mapSize;
};

struct retrinfo_t : msg_t
{

};

struct networkinfo_t : msg_t
{
	char NetworkName[256];
	char MOTD[MOTD_SIZE];
};

struct storechar_t : filemsg_t
{
	ulong CharSize;
	char CharData[1];
};

struct retrchar_t : filemsg_t
{

};

struct filenotfound_t : filemsg_t
{

};

struct deletechar_t : filemsg_t
{

};

struct charsuccess_t : filemsg_t
{
	ushort OrigMsg;
};

struct disconnect_t : msg_t
{
	
};

//MIB JAN2010_15
#define	ERR_POPUP	(1<<0)	//Pop up a message (provide with message)
#define ERR_CRASH	(1<<1)	//Cause the server to crash
#define ERR_BADMAP	(1<<2)	//Specifically changes a variable on the server
struct error_t : msg_t
{
	int flags;
	char error_msg[512];
};


//MiB Feb2008a
struct fnfilewrite_t : msg_t
{
	char FileName[128];
	char line[512];
	char type; //(o)verwrite,(i)nsert,(a)ppend
	int lineNum;
};

//MiB Feb2008a
struct fnfileread_t : msg_t
{
	char FileName[128];
	char EntString[64];
	ulong DataSize;
	int fileExists;
	char Data[1];
};

enum msg_e
{
	MSG_PASSWORD,			//Client sent password
	MSG_RETRINFO,			//Client wants some central network info
	MSG_INFO,				//Client is sending network info
	MSG_RETRFILE,			//Client wants to retreive file
	MSG_STORFILE,			//Client wants to store file
	MSG_DELEFILE,			//Client wants to remotely remove file
	MSG_CHAR_SUCCESS,		//Client successfully completed the operation
	MSG_DISC,				//Client wants to disconnect
	MSG_FILENOTFOUND,		//Client couldn't find file
	MSG_READFNFILE,			//Read a file from FN - MiB Feb2008a
	MSG_WRITEFNFILE,         //Write a file to FN - MiB Feb2008a
	MSG_ERROR,				//FN Needs to inform the client of some error (MiB JAN2010_15)
	MSG_PASSWORD_NEW,		//New password message. Needs to be identifiable to stop crashes from reading old messages. (MIB FEB2010)
};

#include <poppack.h>


class CTransaction
{
public:
	CTransaction( ) 
	{
		m_Validated = false;
		m_Disconnected = false;
	}

	//Shared
	void Run( );
	void SendChar( const char *AuthID, int CharNum, const char *Data, int DataLen );
	void SendDisconnect( );
	void Disconnect( );

	//Scoped
	virtual void HandleMsg( msg_t &Msg );
	virtual void ReceivedChar( const char *AuthID, int CharNum, char *Data, int DataLen ) { }
	virtual void SentChar( const char *AuthID, int CharNum ) { }
	virtual void Error_FileNotFound( msg_e MsgID, const char *AuthID, int CharNum ) { }
	virtual void ValidationFailed( ) { }
	static void GetCharFileName( const char *AuthID, int iCharacter, string &FileName );
	virtual void Disconnected( ) { }

	SOCKET m_Socket;
	sockaddr_in m_AddrIn;
	string m_RemoteIP;
	bool m_Validated, m_Disconnected;
};


#define THREADLOCK Critical( )
#define THREADUNLOCK EndCritical( )
void Critical( );
void EndCritical( );
void Transaction_Accept( );
