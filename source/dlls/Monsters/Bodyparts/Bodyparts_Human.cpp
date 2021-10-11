#include "MSDLLHeaders.h"
#include "Monsters/MSMonster.h"
#include "Player/modeldefs.h"
#include "Bodyparts_Human.h"
#include "monsters.h"

extern char *ModelList[4][2];

int HumanModels = ARRAYSIZE(ModelList);

CHumanBodypart *CHumanBodypart::Duplicate( ) {
	CHumanBodypart *pNewBodypart = GetClassPtr((CHumanBodypart *)NULL);
	return (CHumanBodypart *)CBodypart::Duplicate( pNewBodypart );
}

void CHumanBody::Initialize( CBaseEntity *pOwner, void *pvData ) 
{
	foreach( i, HUMAN_BODYPARTS )
	{
		CHumanBodypart &HumanBodypart = *(CHumanBodypart *)GetClassPtr( (CHumanBodypart *)NULL );

		int Gender = ((CMSMonster *)pOwner)->m_Gender;
		if( pvData ) Gender = (int)pvData;

		HumanBodypart.Initialize( pOwner, ModelList[i][Gender], i );

		Bodyparts.add( &HumanBodypart );
	}
}
CBaseBody *CHumanBody::Duplicate( ) 
{
	CHumanBody &NewBody = *msnew(CHumanBody);
	foreach( i, Bodyparts.size() )
		NewBody.Bodyparts.add( Bodyparts[i]->Duplicate() );

	return &NewBody;
}

int HitGroupToBodyPart( int HitGroup )
{
	switch( HitGroup )
	{
	case HITGROUP_HEAD:
		return HBP_HEAD;

	case HITGROUP_CHEST:
	case HITGROUP_STOMACH:
		return HBP_CHEST;

	case HITGROUP_LEFTARM:
	case HITGROUP_RIGHTARM:
		return HBP_ARMS;

	case HITGROUP_LEFTLEG:
	case HITGROUP_RIGHTLEG:
		return HBP_LEGS;

	default:
		return HBP_CHEST;
	}
}