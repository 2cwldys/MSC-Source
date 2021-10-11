[quote="The Man In Black"]You can make up your own name for the script command. If you can't tell, to define who can pick up the item you add their ent-string to the script array PICKUP_ALLOW_LIST.

Don't forget that I didn't actually compile this, much less test it.



Search "if( pObject->IsMSMonster() && ((CMSMonster *)pObject)->IsLootable( this ) )" and put below that if-block this:
[code]
		if ( FBitSet(pObject->Properties,ITEM_NOPICKUP) )
			continue;

		bool ALLOW = true;
		foreach ( i , pObject->scriptedArrays.size() )
		{
			if ( pObject->scriptedArrays[i].Name == "PICKUP_ALLOW_LIST" )
			{
				ALLOW = false;
				foreach ( j , pObject->scriptedArrays[i].Vals.size() )
				{
					if ( EntToString(this) == pObject->scriptedArrays[i].Vals[j] )
					{
						ALLOW = true;
						break;
					}
				}
				break;
			}
		}

		if ( !ALLOW )
			continue;
[/code]

ScriptCmds.cpp (Add it above)
[code]
else if ( Cmd.Name() == "WHATEVER_YOU_WANT" )
{
	if ( Params.size() >= 1 )
	{
		CGenericItem *pItem = m.pScriptedEnt.IsMSItem() ? (CGenericItem *)m.pScriptedEnt : NULL;
		if ( pItem )
		{
			if ( Params[0] == "1" )
				SetBits(pItem->Properties,ITEM_NOPICKUP);
			else
				ClearBits(pItem->Properties,ITEM_NOPICKUP);
		}
	}
	else ERROR_MISSING_PARMS;
[/code]

Search out "//***** Item properties *****" and add at the end of the group
[code]
#define ITEM_NOPICKUP ( 1 << 10 )
[/code][/quote]
