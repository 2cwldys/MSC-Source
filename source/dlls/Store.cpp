#include "MSDLLHeaders.h"
#include "Player/player.h"
#include "vgui_MenuDefsShared.h"

#include "Store.h"

mslist<CStore *> CStore::m_gStores;


void CStore :: SetName( const char *pszName )
{
	m_Name = pszName;
}
bool CStore :: AddItem( const char *pszItemName, int iQuantity, int iCost, float flSellRatio, int iBundleAmt )
{
	storeitem_t *pNewItem;
	if( !(pNewItem = GetItem( pszItemName )) ) 
	{
		storeitem_t NewItem;
		clrmem( NewItem );
		pNewItem = &Items.add( NewItem );
		pNewItem->ID = Items.size()-1;
	}

	pNewItem->Name = pszItemName;
	pNewItem->Quantity += iQuantity;
	pNewItem->iCost = iCost;
	pNewItem->flSellRatio = flSellRatio;
	pNewItem->iBundleAmt = iBundleAmt;

	return TRUE;
}
storeitem_t *CStore :: GetItem( const char *pszItemName )
{
	foreach( i, Items.size( ) )
		if( FStrEq(Items[i].Name, pszItemName ) )
			return &Items[i];

	return NULL;
}
storeitem_t *CStore :: GetItem( int idx )
{
	if( idx < 0 || idx >= (signed)Items.size( ) )
		return NULL;

	return &Items[idx];
}
void CStore::RemoveItem( msstring_ref Name ) 
{
	foreach( i, Items.size( ) )
		if( FStrEq(Items[i].Name, Name ) )
		{
			Items.erase( i-- );
			break;
		}
	
}
void CStore::RemoveAllItems( ) 
{
	Items.clear( );
}
void CStore :: Deactivate( )
{
	RemoveAllItems( );
	foreach( i, m_gStores.size() )
		if( m_gStores[i] == this )
		{
			m_gStores.erase( i );
			break;
		}
}
void CStore :: Offer( edict_t *pePlayer, int iBuyFlags, CBaseMonster *pVendor )
{
	CBasePlayer *pPlayer = (CBasePlayer *)MSInstance( pePlayer );

	if( !pPlayer || !pPlayer->IsPlayer() /*|| !ItemTypes*/ ) return;

	pVendor->SetConditions( MONSTER_TRADING );
	pVendor->m_hEnemy = pPlayer;
	pPlayer->SetConditions( MONSTER_TRADING );
	pPlayer->m_hEnemy = pVendor;

	MESSAGE_BEGIN( MSG_ONE, g_netmsg[NETMSG_VGUIMENU], NULL, pPlayer->pev );
		WRITE_BYTE( MENU_STORE );
		WRITE_BYTE( iBuyFlags );
		WRITE_STRING( pVendor->DisplayName() );
		WRITE_BYTE( Items.size() );
	MESSAGE_END();

	foreach( i, Items.size() )
	{
		//Send all store items.

		storeitem_t &Item = Items[i];

		MESSAGE_BEGIN( MSG_ONE, g_netmsg[NETMSG_STOREITEM], NULL, pPlayer->pev );
			WRITE_BYTE( 0 );					//store item
			WRITE_STRING( Item.Name );
			WRITE_SHORT( Item.Quantity );
			WRITE_LONG( Item.iCost ); //MiB JAN2010_15 Shop Prices over 32k.rtf
			WRITE_SHORT( Item.flSellRatio * 100 );
			WRITE_SHORT( Item.iBundleAmt );
		MESSAGE_END();
	}
	if( FBitSet(iBuyFlags,STORE_INV) )
	{
		MESSAGE_BEGIN( MSG_ONE, g_netmsg[NETMSG_STOREITEM], NULL, pPlayer->pev );
			WRITE_BYTE( 0 );					//store item
			WRITE_STRING( "gold" );
			WRITE_LONG( ((CMSMonster *)pVendor)->m_Gold );
		MESSAGE_END();
	}
}
