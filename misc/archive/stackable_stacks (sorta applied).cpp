[quote="The Man In Black"]Another night where I should have been able to work, but aliens :\ Was at least able to put the finishing touches on making items stack





Go to the definition of "bool CGenericItem::Container_CanAcceptItem( CGenericItem *pItem )" and change "if( PackData->MaxItems && Container_ItemCount() + 1 > PackData->MaxItems )" to:
[code]
if( PackData->MaxItems && Container_ItemCount() + 1 > PackData->MaxItems )
		{
			//MiB - Making this into an if-block - If the pack is "full" we check to see
			//		if the item is groupable and if it can be fully grouped into another.
			if ( FBitSet( pItem->Properties , ITEM_GROUPABLE ) )
			{
				foreach ( i , PackData->ItemList.size() )
				{
					CGenericItem *pCur = Container_GetItem( i );
					if ( msstring(pCur->m_Name) != msstring( pItem->m_Name ) ) //If it has the same script name
						continue;

					if ( !FBitSet( pCur->Properties , ITEM_GROUPABLE ) ) //If it's groupable (Paranoia)
						continue;

					if ( pCur->iQuantity + pItem->iQuantity > pCur->m_MaxGroupable ) //If it won't over-fill the stack
						continue;

					return true; //It's ok to put the item in here. It will all get absorbed into the stack.
					
				}
			}

			return false; //Wasn't groupable or couldn't find a suitable group.
		}
[/code]

Search "bool CGenericItem::Container_AddItem( CGenericItem *pItem )" and replace the function with
(Also search for the declaration of the function and change the return type from bool to int.
[code]
int CGenericItem::Container_AddItem( CGenericItem *pItem )
{
	if( !Container_CanAcceptItem(pItem) ) 
		return 0;

	int leftOver = 0;
	if ( FBitSet( pItem->Properties, ITEM_GROUPABLE ) )
	{
		foreach ( i , PackData->ItemList.size() )
		{
			CGenericItem *pCur = Container_GetItem( i );
			if ( msstring(pCur->m_Name) != msstring( pItem->m_Name ) ) //If it has the same script name
				continue;

			if ( !FBitSet( pCur->Properties , ITEM_GROUPABLE ) ) //If it's groupable (Paranoia)
				continue;

			if ( pCur->iQuantity >= pCur->m_MaxGroupable ) //If it isn't full
				continue;

			leftOver = ( pCur->iQuantity + pItem->iQuantity ) - pCur->m_MaxGroupable; //Positive if we need to keep pItem as a new stack
			pCur->iQuantity += pItem->iQuantity;
			pItem->iQuantity = leftOver;
			if ( leftOver <= 0 )
			{
				Container_SendItem( pCur , true );
				return -1;
			}

			break;
		}
	}

	PackData->ItemList.AddItem( pItem );

	pItem->m_pParentContainer = this;

	//Update client
	Container_SendItem( pItem, true );

	//Thothie FEB20080a
	//- This event goes off for every item the player in a container has at spawn
	//- making much lag on connect
	//- It'd be good if we could rig this so it'd only do so post-spawn (ie. when a player actually adds an item to a container)
	//- at the moment I'm commenting it out as doing so doesn't seem to mess with anything
	//- but it maybe a useful function in the future if we can get around the lag on connect issue
	//static msstringlist Params;
	//Params.clearitems( );
	//ifdef VALVE_DLL
	//	Params.add( EntToString(pItem) );
	//endif
	//CallScriptEvent( "game_container_addeditem", &Params );

	return 1;
}
[/code]

Search "pContainer->Container_AddItem( this );" and replace it with this
[code]
int ret = pContainer->Container_AddItem( this );
	if ( ret == -1 )
	{
		SUB_Remove();
		return true;
	}
[/code]

Add to the class CGenericItem "int m_MaxGroupable"

Search "else if( Cmd.Name() == "groupable" )" and add below "if( iQuantity <= 0 ) iQuantity = 1;"
m_MaxGroupable = iMaxGroupable;[/quote]
