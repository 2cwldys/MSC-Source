//
//  Character functions for the Server
//

#include "inc_weapondefs.h"
#include "Stats/Stats.h"
#include "../MSShared/Global.h"
#include "MSCentral.h"
#include "logfile.h"
#include "MSCharacter.h"
#include "../MSShared/Magic.h"
#include "../MSShared/Script.h"

#ifndef _WIN32
	#include "sys/io.h"
#endif

void CBasePlayer::CreateChar( createchar_t &CharData )
{
	//Create a new character, using the options the user specified.
	//A custom savedata_t struct can be created for the new
	//character and passed to SaveAll, but any items the character
	//starts with need to be added to Gear

	savedata_t Data;
	memset( &Data, 0, sizeof(savedata_t) );

	strcpy( Data.Name, CharData.Name );
	strcpy( Data.Race, CharData.Race ); // MIB FEB2015_21 [RACE_MENU] - Create with given race
	strcpy( Data.MapName, MSGlobals::MapName );
	Data.Gender = CharData.Gender;
	Data.Gold = MSGlobals::DefaultGold;

	#ifndef VALVE_DLL
		MSCLGlobals::RemoveAllEntities( );
	#else
		RemoveAllItems( false, true );
	#endif
	//Give 1 skill to each stat
	foreach( i, m_Stats.size() )
	{
		CStat &Stat = m_Stats[i];
		if( Stat.m_SubStats.size() == 1 )					//Parry - Only give 1 to proficiency
			Stat.m_SubStats[0].Value = 1;
		else if( Stat.m_SubStats.size() <= STATPROP_TOTAL )	//Weapon Skills - Give 1 to power
			Stat.m_SubStats[STATPROP_POWER].Value = 1;
		else if( Stat.m_SubStats.size() > STATPROP_TOTAL )	//Spellcasting - Give 1 to each spell category
			foreach( r, Stat.m_SubStats.size() ) Stat.m_SubStats[r].Value = 1;
	}

	//Give free items.  Wear any packs
	foreach( i, MSGlobals::DefaultFreeItems.size() )
	{
		CGenericItem *pStartingItem = NewGenericItem( MSGlobals::DefaultFreeItems[i] );
		if( !pStartingItem ) continue;

		if( FBitSet(pStartingItem->MSProperties(),ITEM_WEARABLE) )
		{
			//Wear packs
			//m_fClientInitiated = true;				//Dont send a client update
			if( !AddItem( pStartingItem, false, true ) || !pStartingItem->WearItem( ) )
				pStartingItem->SUB_Remove( );
			//m_fClientInitiated = false;
		}
		else if( FBitSet(pStartingItem->MSProperties(),ITEM_SPELL) )
		{
			LearnSpell( pStartingItem->ItemName, true );
			pStartingItem->SUB_Remove( );
		}
		else
		{
			//Normal item
			if( !AddItem( pStartingItem, true, true ) )
				pStartingItem->SUB_Remove( );
		}
	}

	//Put the chosen weapon in the player's right hand
	CGenericItem *pStartingItem = NewGenericItem( CharData.Weapon );

	if( FBitSet(pStartingItem->MSProperties(),ITEM_SPELL) )
		//It's a spell
	{
		LearnSpell( pStartingItem->ItemName, false );
		pStartingItem->SUB_Remove( );
	}
	else
	{
		if( !AddItem( pStartingItem, true, true ) )
			pStartingItem->SUB_Remove( );
	}

	m_CharacterNum = CharData.iChar;
	m_CharacterState = CHARSTATE_LOADED;				//Temporary, so the save will succeed
	MSChar_Interface::SaveChar( this, &Data );			//Save the new character

	m_CharacterState = CHARSTATE_UNLOADED;				//Created a new character... this one is no longer valid

	//Clean up
	RemoveAllItems( false, true );

	#ifdef VALVE_DLL
		//Update player's character list, so the new char is sent down to client
		if( !MSCentral::Enabled( ) )
			PreLoadChars( );
	#endif
}

bool DeleteChar( CBasePlayer *pPlayer, int iCharacter )
{
	#ifdef VALVE_DLL
		if( MSCentral::Enabled( ) )
		{
			MSCentral::RemoveChar( pPlayer->AuthID(), iCharacter );
			return true;
		}
	#endif

	const char *pszCharFileName = GetSaveFileName( iCharacter, pPlayer );
	int ret = remove( pszCharFileName );		//Delete savefile
	remove( BACKUP_NAME(pszCharFileName) );		//Delete backup

	//Update player's char list
	if( iCharacter < (signed)pPlayer->m_CharInfo.size() )
		pPlayer->m_CharInfo[iCharacter].Status = CDS_NOTFOUND;

	return (!ret) ? true : false;
}
void MSChar_Interface::AutoSave( CBasePlayer *pPlayer )
{
	if( gpGlobals->time > pPlayer->m_TimeNextSave )
	{
		SaveChar( pPlayer, NULL );
		pPlayer->m_TimeNextSave = gpGlobals->time + 3.0f;
	}
}

//
//  Read chunks from raw char data
//

static char cTemp[256];
void chardata_t::ReadMaps1( byte DataID, CPlayer_DataBuffer &m_File )
{
	startdbg;
	if( DataID == CHARDATA_MAPSVISITED1 )
	{
		//Read Maps Visited
		//Must come DIRECTLY after reading the savedata_t Data
		dbg( "Read Stats" );
		int Maps = 0;
		m_File.ReadInt( Maps );												//[INT]
		m_VisitedMaps.clear( );
		foreach( m, Maps )
		{
			m_File.ReadString( cTemp );										//[STRING]
			m_VisitedMaps.add( cTemp );
		}
	}
	enddbg;
}
void chardata_t::ReadSkills1( byte DataID, CPlayer_DataBuffer &m_File )
{
	startdbg;
	if( DataID == CHARDATA_SKILLS1 )
	{
		//Read skills
		CStat::InitStatList( m_Stats );
		byte Stats = 0;														//[CHUNK - STATS]
		m_File.ReadByte( Stats );											//[BYTE]
		foreach( i, Stats )													
		{
			if( i >= (signed)m_Stats.size() ) 
				break;
			CStat &Stat = m_Stats[i];
			byte SubStats = 0;
			m_File.ReadByte( SubStats );									//[BYTE]

			foreach( r, SubStats )
			{
				if( r >= (signed)Stat.m_SubStats.size() )
					break;
				short Value = 0;
				int Exp = 0;
				m_File.ReadShort( Value );									//[SHORT]
				m_File.ReadInt( Exp );										//[INT]

				Stat.m_SubStats[r].Value = Value;
				Stat.m_SubStats[r].Exp = Exp;
			}
		}

		// MiB JUL2010_02 - Hacky, but if we add more stats and we load a character that doesn't have said stat in the file
		//		we set it to the default new stat value (level 0 in Prof and Balance, 1 in Power)
		int StatsRemaining = m_Stats.size() - Stats;
		foreach( i , StatsRemaining )
		{
			CStat &Stat = m_Stats[ i + Stats ];
			Stat.m_SubStats[ Stat.m_SubStats.size()-1 ].Value = 1;
		}
	}
	enddbg;
}
void chardata_t::ReadSpells1( byte DataID, CPlayer_DataBuffer &m_File )
{
	startdbg;
	if( DataID == CHARDATA_SPELLS1 )
	{
		//Read Magic spells
		dbg( "Read Magic spells" );
		
		byte Spells = 0;
		m_File.ReadByte( Spells );							//[BYTE]
		foreach( s, Spells )
		{
			m_File.ReadString( cTemp );						//[STRING]	
			m_Spells.add( cTemp );
		}
	}
	enddbg;
}
void chardata_t::ReadItems1( byte DataID, CPlayer_DataBuffer &m_File )
{
	startdbg;
	if( DataID == CHARDATA_ITEMS1 || DataID == CHARDATA_ITEMS2 )
	{
		//Read Items
		dbg( "Read Items" );

		byte GearItems = 0;
		m_File.ReadByte( GearItems );						//[SHORT]
		foreach( i, GearItems )
		{
			genericitem_full_t Item;
			if( !ReadItem1( DataID, m_File, Item ) )		//[X ITEMS]
				continue;

			m_Items.add( Item );
		}
	}
	enddbg;
}
void chardata_t::ReadStorageItems1( byte DataID, CPlayer_DataBuffer &m_File )
{
	startdbg;
	if( DataID == CHARDATA_STORAGE1 )
	{
		//Read storage items
		dbg( "Read Storage Items" );

		short Storages = 0;
		m_File.ReadShort( Storages );						//[SHORT]
		foreach( i, Storages )
		{
			storage_t Storage;
			m_File.ReadString( Storage.Name );				//[X STRINGS]
			
			short Items = 0;
			m_File.ReadShort( Items );						//[X SHORTS]
			foreach( i, Items )
			{
				genericitem_full_t Item;
				if( !ReadItem1( DataID, m_File, Item ) )	//[Y ITEMS]
					continue;
				
				Storage.Items.add( Item );
			}

			m_Storages.add( Storage );
		}
	}
	enddbg;
}
void chardata_t::ReadCompanions1( byte DataID, CPlayer_DataBuffer &m_File )
{
	startdbg;
	if( DataID == CHARDATA_COMPANIONS1 )
	{
		//Read Companions
		dbg( "Read Companions" );

		short Companions = 0;
		m_File.ReadShort( Companions );									//[SHORT]
		foreach( c, Companions )
		{
			companion_t &Companion = m_Companions.add( companion_t() );
			m_File.ReadString( cTemp );									//[STRING]
			Companion.ScriptName = cTemp;
			Companion.Active = false;

			//Read the saved variables
			short Vars = 0;
			m_File.ReadShort( Vars );									//[SHORT]
			foreach( var, Vars )
			{
				m_File.ReadString( cTemp );								//[STRING]
				Companion.SaveVarName.add( cTemp ); 
				m_File.ReadString( cTemp );								//[STRING]
				Companion.SaveVarValue.add( cTemp ); 
			}
		}
	}
	enddbg;
}
void chardata_t::ReadHelpTips1( byte DataID, CPlayer_DataBuffer &m_File )
{
	startdbg;
	if( DataID == CHARDATA_HELPTIPS1 )
	{
		//Read Help tips
		short HelpTips = 0;
		m_File.ReadShort( HelpTips );										//[SHORT]
		m_ViewedHelpTips.clear( );
		foreach( t, HelpTips )
		{
			m_File.ReadString( cTemp );										//[STRING]
			m_ViewedHelpTips.add( cTemp );
		}
	}
	enddbg;
}
void chardata_t::ReadQuests1( byte DataID, CPlayer_DataBuffer &m_File )
{
	startdbg;
	if( DataID == CHARDATA_QUESTS1 )
	{
		//Read Quests
		int Quests = 0;
		m_File.ReadInt( Quests );											//[INT]
		m_Quests.clear( );
		foreach( q, Quests )
		{
			quest_t Quest;
			m_File.ReadString( cTemp );										//[STRING]
			Quest.Name = cTemp;
			m_File.ReadString( cTemp );										//[STRING]
			Quest.Data = cTemp;
			m_Quests.add( Quest );
		}
	}
	enddbg;
}
void chardata_t::ReadQuickSlots1( byte DataID, CPlayer_DataBuffer &m_File )
{
	startdbg;
	if( DataID == CHARDATA_QUICKSLOTS1 )
	{
		//Read Quickslots
		byte QuickSlots = 0;
		m_File.ReadByte( QuickSlots );										//[BYTE]
		m_QuickSlots.clear( );
		foreach( q, QuickSlots )
		{
			quickslot_t QuickSlot;
			byte Type = 0;
			m_File.ReadByte( Type );										//[BYTE]

			if( Type )	//Something in this slot
			{
				int OldID = -1;
				m_File.ReadInt( OldID );									//[INT]

				QuickSlot.Active = true;
				QuickSlot.Type = (quickslottype_e)(Type - 1);				//Slot type is offset by 1
				QuickSlot.ID = OldID;
			}
			else	//Nothing in this slot
			{
				QuickSlot.Active = false;
			}

			m_QuickSlots.add( QuickSlot );
		}
	}
	enddbg;
}

bool chardata_t::ReadItem1( byte DataID, CPlayer_DataBuffer &Data, genericitem_full_t &outItem )
{
	clrmem( outItem );

	char cTemp[128];
	Data.ReadString( cTemp );						//[STRING]
	if( !strlen(cTemp) )
		return false;
	outItem.Name = cTemp;

	//It is now possible to read an item from file correctly, but not be able to spawn that item.
	//Be sure that, even if the item can't be created, all of the item's data is read from file properly

	short Properties = 0;
	Data.ReadShort( Properties );				//[SHORT]

	outItem.Properties = Properties;

	short Location = 0;
	Data.ReadShort( Location );					//[SHORT]
	outItem.Location = Location;

	byte Hand = 0;
	Data.ReadByte( Hand );						//[BYTE]
	outItem.Hand = Hand;

	if( DataID == CHARDATA_ITEMS2 )
	{
		int LastID = 0;								
		Data.ReadInt( LastID );					//[INT]
		outItem.ID = LastID;
	}


	if( FBitSet( Properties, ITEM_PERISHABLE ) ||
		FBitSet( Properties, ITEM_DRINKABLE ) )
	{
		short Quality = 0, MaxQuality = 0;
		Data.ReadShort( Quality );				//[SHORT]
		Data.ReadShort( MaxQuality );			//[SHORT]

		outItem.Quality = Quality;
		outItem.MaxQuality = MaxQuality;
	}

	if( FBitSet( Properties, ITEM_GROUPABLE ) )
	{
		short GroupAmt = 0;
		Data.ReadShort( GroupAmt );				//[SHORT]

		outItem.Quantity = GroupAmt;
	}

	if( FBitSet( Properties, ITEM_CONTAINER ) )
	{
		short iItemCount = 0;
		Data.ReadShort( iItemCount );			//[SHORT]
	
		genericitem_full_t PackItem;
		foreach( i, iItemCount )
		{
			bool Success = ReadItem1( DataID, Data, PackItem );
			if( !Success )
			{
				MSErrorConsoleText( "ReadItem()", "Bad item in container, skipping..." );
				continue;
			}
			outItem.ContainerItems.add( PackItem );
			
			/*if( pItem )
				pPackItem->PutInPack( pItem );
			else
				//If I couldn't spawn this pack, drop the item that's supposed to be inside it
				pPackItem->Drop( 3, (Vector &)g_vecZero, (Vector &)g_vecZero, (Vector &)g_vecZero );*/
		}
	}

	return true;
}

//
// Save Character
// ==============

void WriteItem( CPlayer_DataBuffer &gFile, genericitem_full_t &Item );

//If pData != NULL, then this is a new char
void MSChar_Interface::SaveChar( CBasePlayer *pPlayer, savedata_t *pData ) 
{
	startdbg;
	dbg( "Begin" );

	//#ifndef VALVE_DLL
	//	if( MSGlobals::ServerSideChar )
	//		return;
	//#endif

	//Can I save right now?
	if( pPlayer->m_CharacterState == CHARSTATE_UNLOADED /*||	//Can't save if no character is created
		MSGlobals::GameType != GAMETYPE_ADVENTURE*/ )			//If the gametype isn't adventure, we can't save no matter what.
	{
		return;
	}

	//MiB JUL2010_13 - Don't save in dev mode  
	if ( MSGlobals::DevModeEnabled )
		return;

	//FEB2015_25 Thothie - don't save if <15 hp (char delete bug workaround)
	if ( pPlayer->MaxHP() < 15 ) return;

	//Add this map to the list of maps visited
	if( !HasVisited( MSGlobals::MapName, pPlayer->m_Maps ) ) 
		pPlayer->m_Maps.add( MSGlobals::MapName );

	const char *pszFileName;
	//#ifdef VALVE_DLL
		pszFileName = GetSaveFileName( pPlayer->m_CharacterNum, pPlayer );
	//#else
	//	pszFileName = GetSaveFileName( pPlayer->m_CharacterNum );
	//#endif

	//if( fVerbose ) Print( "Saving to file: %s\n", pszFileName );

	CPlayer_DataBuffer gFile( 1<<16 );

	//Initialize
	savedata_t Data;
	memset( &Data, 0, sizeof(savedata_t) );
	if( pData ) memcpy( &Data, pData, sizeof(savedata_t) );

	//Copy the data
	Data.Version = SAVECHAR_VERSION;
	//strcpy( Data.MapName, (pPlayer->iHP > 0) ? g_MapName : "" );

	if( !pData )
	{
		strcpy( Data.Name, pPlayer->m_DisplayName );		//Store actual character name (DisplayName() is servername, and will have a (#) at the end if there are duplicates on the server)
		strcpy( Data.Race, pPlayer->m_Race );
		strcpy( Data.Party, pPlayer->GetPartyName() );
		Data.PartyID = pPlayer->GetPartyID( );

		strcpy( Data.MapName, MSGlobals::MapName );
		
		strncpy( Data.OldTrans,	pPlayer->m_OldTransition, 32 );
		strncpy( Data.NextMap,	pPlayer->m_NextMap, 32 );
		strncpy( Data.NewTrans,	pPlayer->m_NextTransition, 32 );

		Data.IsElite = pPlayer->m_fIsElite;
		Data.Gold = pPlayer->m_Gold;
		Data.MaxHP = pPlayer->m_MaxHP;
		Data.MaxMP = pPlayer->m_MaxMP;
		if( pPlayer->pev->deadflag == DEAD_NO ) 
		{
			Data.HP = pPlayer->m_HP;
			Data.MP = pPlayer->m_MP;
		}
		else { 
			//Tried to save while dead.
			//Set Health/Mana to 0 to indicate this
			Data.HP = Data.MP = 0.0;
			//fDead = true;
			//pPlayer->iHP = 1.0;
		}
		
		//Data.Origin = LastGoodPos;
		//Data.Angles = LastGoodAng;
		//Print( "Save: Data.Origin: %f %f %f\n", Data.Origin.x, Data.Origin.y, Data.Origin.z );
		//Data.SayType = pPlayer->m_SayType;
		Data.Gender = pPlayer->m_Gender;
		Data.PlayerKills = pPlayer->m_PlayersKilled;
		Data.TimeWaitedToForgetKill = pPlayer->m_TimeWaitedToForgetKill;
		//#ifdef VALVE_DLL
			strncpy( Data.SteamID, GETPLAYERAUTHID(pPlayer->edict()), 32 );
		//#else
		//	strncpy( Data.SteamID, MSCLGlobals::AuthID, 32 );
		//#endif
	}

	gFile.WriteByte( CHARDATA_HEADER1 );
	gFile.Write( &Data, sizeof(savedata_t) );							//[VAR] Player Info

	//Save Maps Visited
	//Must come just after writing savedata_t Data
	gFile.WriteByte( CHARDATA_MAPSVISITED1 );							//[BYTE - CHUNK - MAPS VISITED]
	gFile.WriteInt( pPlayer->m_Maps.size() );							//[INT]
	foreach( m, pPlayer->m_Maps.size() )								
		gFile.WriteString( pPlayer->m_Maps[m] );						//[STRING]

	//Save skills
	gFile.WriteByte( CHARDATA_SKILLS1 );								//[BYTE - CHUNK - STATS]
	statlist &StatList = pPlayer->m_Stats;
	gFile.WriteByte( StatList.size() );									//[BYTE]
	foreach( i, StatList.size() )										
	{
		CStat &Stat = StatList[i];
		gFile.WriteByte( Stat.m_SubStats.size() );						//[BYTE]
		foreach( r, Stat.m_SubStats.size() )
		{
			CSubStat &SubStat = Stat.m_SubStats[r];
			gFile.WriteShort( SubStat.Value );							//[SHORT]
			gFile.WriteInt( SubStat.Exp );								//[INT]
		}
	}

	//Save magic spells
	dbg( "Write spells" );

	spellgroup_v &SpellList = pPlayer->m_SpellList;
	gFile.WriteByte( CHARDATA_SPELLS1 );							//[BYTE - CHUNK - SPELLS]
	gFile.WriteByte( SpellList.size() );							//[BYTE]

	foreach( s, SpellList.size() )
		gFile.WriteString( SpellList[s] );							//[X STRINGS]

	//Save Items
	dbg( "Write Items" );
	gFile.WriteByte( CHARDATA_ITEMS2 );								//[BYTE - CHUNK - ITEMS]

	static mslist<CGenericItem *> WriteList;
	WriteList.clearitems( );

	foreach( i,  pPlayer->Gear.size() )
		if( pPlayer->Gear[i] != pPlayer->PlayerHands )				//Skip player hands
			WriteList.add( pPlayer->Gear[i] );					

	gFile.WriteByte( WriteList.size() );							//[BYTE]

	foreach( i,  WriteList.size() )
		WriteItem( gFile, genericitem_full_t( WriteList[i] ) );		//[X ITEMS]

	//Save storage items
	gFile.WriteByte( CHARDATA_STORAGE1 );								//[BYTE - CHUNK - STORAGE ITEMS]
	gFile.WriteShort( pPlayer->m_Storages.size() );					//[SHORT]

	foreach( s, pPlayer->m_Storages.size() )
	{
		storage_t &Storage = pPlayer->m_Storages[s];

		gFile.WriteString( Storage.Name );							//[STRING]
		gFile.WriteShort( Storage.Items.size() );					//[SHORT]
		foreach( i, Storage.Items.size() )
			WriteItem( gFile, Storage.Items[i] );					//[X ITEMS]
		
	}

	//Save Companions
	gFile.WriteByte( CHARDATA_COMPANIONS1 );						//[BYTE - CHUNK - COMPANIONS]
	gFile.WriteShort( pPlayer->m_Companions.size() );				//[SHORT]
	static msstringlist SaveVarName, SaveVarValue;
	SaveVarName.clearitems( );
	SaveVarValue.clearitems( );

	foreach( c, pPlayer->m_Companions.size() )
	{
		companion_t &Companion = pPlayer->m_Companions[c];
		gFile.WriteString( Companion.ScriptName );					//[STRING]

		//Save any variables that start with "companion.save."
		CBaseEntity *pEntity = Companion.Entity.Entity( );
		if( !pEntity ) continue;
		IScripted *pScripted = pEntity->GetScripted( );
		if( !pScripted || !pScripted->m_Scripts.size() ) continue;
		
		pScripted->CallScriptEvent( "game_companion_save" );

		CScript *Script = pScripted->m_Scripts[0];
		foreach( v, Script->m_Variables.size() )
			if( Script->m_Variables[v].Name.starts_with("companion.save.") )
			{
				SaveVarName.add( Script->m_Variables[v].Name );
				SaveVarValue.add( Script->m_Variables[v].Value );
			}

		gFile.WriteShort( SaveVarName.size() );						//[SHORT]
		foreach( var, SaveVarName.size() )
		{
			gFile.WriteString( SaveVarName[var] );				//[STRING]
			gFile.WriteString( SaveVarValue[var] );				//[STRING]
		}
		SaveVarName.clearitems( );	//Reset these for the next companion
		SaveVarValue.clearitems( );
	}

	//Save Help tips
	gFile.WriteByte( CHARDATA_HELPTIPS1 );							//[BYTE - CHUNK - HELPTIPS]
	gFile.WriteShort( pPlayer->m_ViewedHelpTips.size() );			//[SHORT]
	foreach( t, pPlayer->m_ViewedHelpTips.size() )
		gFile.WriteString( pPlayer->m_ViewedHelpTips[t] );			//[STRING]

	//Save Quests
	gFile.WriteByte( CHARDATA_QUESTS1 );							//[BYTE - CHUNK - QUESTS]
	gFile.WriteInt( pPlayer->m_Quests.size() );						//[INT]
	foreach( q, pPlayer->m_Quests.size() )
	{
		gFile.WriteString( pPlayer->m_Quests[q].Name );				//[STRING]
		gFile.WriteString( pPlayer->m_Quests[q].Data );				//[STRING]
	}

	//Save Quickslots
	gFile.WriteByte( CHARDATA_QUICKSLOTS1 );						//[BYTE - CHUNK - QUICKSLOTS]
	gFile.WriteByte( MAX_QUICKSLOTS );								//[INT]
	foreach( q, MAX_QUICKSLOTS )
	{
		quickslot_t &QuickSlot = pPlayer->m_QuickSlots[q];
		if( QuickSlot.Active )
		{
			gFile.WriteByte( ((byte)QuickSlot.Type)+1 );			//[BYTE]
			gFile.WriteInt( QuickSlot.ID );							//[INT]
		}
		else
			gFile.WriteByte( 0 );									//[BYTE]
	}	
	//-------------

	dbg( "Encrypt" );
	gFile.m_BufferSize = gFile.GetWritePtr();
	gFile.Encrypt( ENCRYPTION_TYPE );


	//#ifdef VALVE_DLL
		if( MSCentral::Enabled( ) )
		{
			//If Central Server is enabled, save to the Central Server instead of locally
			MSCentral::SaveChar( GETPLAYERAUTHID( pPlayer->edict() ), pPlayer->m_CharacterNum, (const char *)gFile.m_Buffer, gFile.GetFileSize(), pData != NULL );
			gFile.Close( );
			return;
		}
		else if( !MSGlobals::ServerSideChar )
		{
			charinfo_t &CharInfo = pPlayer->m_CharInfo[pPlayer->m_CharacterNum];
			CharInfo.AssignChar( pPlayer->m_CharacterNum, LOC_CLIENT, (char *)gFile.m_Buffer, gFile.GetFileSize(), pPlayer );
			gFile.Close( );
			return;
		}
	//#endif
	dbg( "Write to File" );
	gFile.WriteToFile( pszFileName, "wb", true );

	gFile.Close( );

	enddbg;
}