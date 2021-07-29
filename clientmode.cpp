#include "includes.h"

bool Hooks::ShouldDrawParticles( ) {
	return g_hooks.m_client_mode.GetOldMethod< ShouldDrawParticles_t >( IClientMode::SHOULDDRAWPARTICLES )( this );
}

bool Hooks::ShouldDrawFog( ) {
	// remove fog.
	if( g_menu.main.visuals.nofog.get( ) )
		return false;

	return g_hooks.m_client_mode.GetOldMethod< ShouldDrawFog_t >( IClientMode::SHOULDDRAWFOG )( this );
}

void Hooks::OverrideView( CViewSetup* view ) {
	// damn son.
	g_cl.m_local = g_csgo.m_entlist->GetClientEntity< Player* >( g_csgo.m_engine->GetLocalPlayer( ) );

	// g_grenades.think( );
	g_visuals.ThirdpersonThink( );

    // call original.
	g_hooks.m_client_mode.GetOldMethod< OverrideView_t >( IClientMode::OVERRIDEVIEW )( this, view );

    // remove scope edge blur.
	if( g_menu.main.visuals.noblur.get( ) ) {
		if( g_cl.m_local )
            view->m_edge_blur = 0;
	}
}

bool Hooks::CreateMove( float time, CUserCmd* cmd ) {
	Stack   stack;
	bool    ret;

	// let original run first.
	ret = g_hooks.m_client_mode.GetOldMethod< CreateMove_t >( IClientMode::CREATEMOVE )( this, time, cmd );

	// called from CInput::ExtraMouseSample -> return original.
	if( !cmd->m_command_number )
		return ret;

	// if we arrived here, called from -> CInput::CreateMove
	// call EngineClient::SetViewAngles according to what the original returns.
	if( ret )
		g_csgo.m_engine->SetViewAngles( cmd->m_view_angles );

	// random_seed isn't generated in ClientMode::CreateMove yet, we must set generate it ourselves.
	cmd->m_random_seed = g_csgo.MD5_PseudoRandom( cmd->m_command_number ) & 0x7fffffff;

	// get bSendPacket off the stack.
	g_cl.m_packet = stack.next( ).local( 0x1c ).as< bool* >( );

	// get bFinalTick off the stack.
	g_cl.m_final_packet = stack.next( ).local( 0x1b ).as< bool* >( );

	// let's better be setting unpredicted curtime when dead.. (fixes clantag and other stuff in the future)
	if (g_cl.m_local && !g_cl.m_local->alive())
		g_inputpred.m_curtime = g_csgo.m_globals->m_curtime;

	// let's wait till we successfully charged if we want to, hide shots. (this fixes anti-aim and shit, sorry, redundant :/)
	
	/*&& g_cl.m_goal_shift == 7 && (g_tickbase.m_shift_data.m_prepare_recharge || g_tickbase.m_shift_data.m_did_shift_before && !g_tickbase.m_shift_data.m_should_be_ready)*/
	//if (g_tickbase.m_shift_data.m_should_attempt_shift && !g_tickbase.m_shift_data.m_should_disable) {
		if (g_menu.main.aimbot.rapidfire.get()) {
			// are we IN_ATTACK?
			*g_cl.m_packet = true;
			static int last_dt_tick = 0;
			if (g_cl.m_cmd->m_buttons & IN_ATTACK) {
				last_dt_tick = g_cl.m_tick;
				if (last_dt_tick >= game::TIME_TO_TICKS(1.2f)) {
					g_tickbase.m_shift_data.m_ticks_to_shift = 13;
				}
			}
		}
	//}

	// invoke move function.
	g_cl.OnTick( cmd );

	// let's wait till we successfully charged if we want to, hide shots.
	//if (g_tickbase.m_shift_data.m_should_attempt_shift && !g_tickbase.m_shift_data.m_should_disable) {
		if (g_menu.main.aimbot.rapidfire.get()) {
			// are we IN_ATTACK?
			*g_cl.m_packet = true;
			// doing this will delete our fake aa but we shift which changes our position so it should be fine...
			static int last_dt_tick = 0;
			if (g_cl.m_cmd->m_buttons & IN_ATTACK) {
				last_dt_tick = g_cl.m_tick;
				if (last_dt_tick >= game::TIME_TO_TICKS(1.2f)) {
					g_tickbase.m_shift_data.m_ticks_to_shift = 13;
				}
			}
		}
	//}

	return false;
}

bool Hooks::DoPostScreenSpaceEffects( CViewSetup* setup ) {
	g_visuals.RenderGlow( );

	return g_hooks.m_client_mode.GetOldMethod< DoPostScreenSpaceEffects_t >( IClientMode::DOPOSTSPACESCREENEFFECTS )( this, setup );
}