/*

	Magic spells   

*/

#include "inc_weapondefs.h"
#include "Stats/statdefs.h"

struct spelldata_t 
{
	int RequiredSkill;
	float CastSuccess;	//Base percentage that determines whether this spell preparation succeeds
	float TimeFizzle;	//Fizzle after this amount of time
};

#define SpellCheck if( !SpellData ) return

void CGenericItem::RegisterSpell( )
{
	Spell_Deactivate( );

	SpellData = msnew(spelldata_t);

	SpellData->RequiredSkill = atoi(GetFirstScriptVar("reg.spell.reqskill"));
	SpellData->TimeFizzle = atof(GetFirstScriptVar("reg.spell.fizzletime"));
	SpellData->CastSuccess = atof(GetFirstScriptVar("reg.spell.castsuccess"));
	Spell_TimePrepare = atof(GetFirstScriptVar("reg.spell.preparetime"));

	SetBits( Properties, ITEM_SPELL );
}

bool CGenericItem::Spell_LearnSpell( const char *pszSpellName )
{
	CGenericItem *pSpell = NewGenericItem( pszSpellName );
	bool fSuccess = false;
	if( !pSpell ) 
		return false;

	if( !pSpell->SpellData )
		goto End;

	if( !m_pPlayer )
		goto End;

	if( m_pPlayer->GetSkillStat( SKILL_SPELLCASTING ) < pSpell->SpellData->RequiredSkill )
		goto End;

	//Thothie attempting to fix spell limit before tome removed
	//- how do I pull the #of spell from here? :/
	/*
	if ( m_pPlayer->m_SpellList.size() > 7 )
	{
		m_pPlayer->SendEventMsg( HUDEVENT_NORMAL, msstring( "You can memorize no more tomes." ));
		pSpell->SUB_Remove( );
		return false;
	}
	*/

	m_pPlayer->LearnSpell( pszSpellName, true );
	fSuccess = true;

End:
	pSpell->SUB_Remove( );
	return fSuccess;
}

void CGenericItem::Spell_Think( )
{
	SpellCheck;

	if( gpGlobals->time >= Spell_TimeCast + SpellData->TimeFizzle )
		{
			if( m_pPlayer ) m_pPlayer->SendEventMsg( HUDEVENT_NORMAL, msstring("The ") + SPEECH_GetItemName( this ) + " spell�s duration ends" );
			DelayedRemove( );
		}
}
bool CGenericItem::Spell_Prepare( )
{
	//Attempt to prepare this spell
	if( !SpellData ) return false;
	if( !m_pOwner ) return false;

	Spell_TimeCast = gpGlobals->time;

	float OwnerPercent = m_pOwner->GetSkillStat(SKILL_SPELLCASTING) / STAT_MAX_VALUE;
	float Number = SpellData->CastSuccess + (100.0f-SpellData->CastSuccess) * OwnerPercent;
	if( RANDOM_LONG(0,100) > (int)Number )
	{
		CallScriptEvent( "game_prepare_failed" );
		return false;
	}

	CallScriptEvent( "game_prepare_success" );
	Spell_CastSuccess = true; //Tells client to call this event from deploy()

	return true;
}
void CGenericItem::Spell_Deactivate( )
{
	if( !SpellData ) 
		return;

	delete SpellData; 
	SpellData = NULL;
}