#ifdef VALVE_DLL
   //removeeffect <target> <game.effect.id>
   else if( Cmd.Name() == "removeeffect" )
   {
      if ( Params.size() >= 2 )
      {
         CBaseEntity *pTarget = RetrieveEntity( Params[0] );
         if ( pTarget )
         {
            IScripted *pTargetScript = pTarget->GetScripted();
            foreach ( i, pTargetScript->m_Scripts.size() )
            {
               if ( pTargetScript->m_Scripts[i]->GetVar("game.effect.id") == Params[1] )
                  pTargetScript->m_Scripts[i]->m.RemoveNextFrame = true;
            }
         }
      }
   }
#endif

edit:


if ( pTargetScript->m_Scripts[i]->GetVar("game.effect.id") == Params[1] )
{
  
  pTargetScript->m_Scripts[i]->RunScriptEventByName( "effect_die" );
  pTargetScript->m_Scripts[i]->m.RemoveNextFrame = true;
}
