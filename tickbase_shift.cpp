#include "includes.h"

#define dont
// whos ur daddy
TickbaseSystem g_tickbase;

void TickbaseSystem::WriteUserCmd( bf_write* buf, CUserCmd* incmd, CUserCmd* outcmd ) {
	// Generic WriteUserCmd code as seen in every hack ever
	using WriteUsercmd_t = void( __fastcall* )( void*, CUserCmd*, CUserCmd* );
	static WriteUsercmd_t fnWriteUserCmd = pattern::find( g_csgo.m_client_dll, XOR( "55 8B EC 83 E4 F8 51 53 56 8B D9 8B 0D" ) ).as<WriteUsercmd_t>( );

	__asm {
		mov     ecx, buf
		mov     edx, incmd
		push    outcmd
		call    fnWriteUserCmd
		add     esp, 4
	}
}

bool Hooks::WriteUsercmdDeltaToBuffer( int m_nSlot, void* m_pBuffer, int m_nFrom, int m_nTo, bool m_bNewCmd ) {
	if( g_tickbase.m_shift_data.m_ticks_to_shift <= 0 )
		return g_hooks.m_client.GetOldMethod<WriteUsercmdDeltaToBuffer_t>( 24 )( this, m_nSlot, m_pBuffer, m_nFrom, m_nTo, m_bNewCmd );

	if( m_nFrom != -1 )
		return true;

	m_nFrom = -1;

	if (g_menu.main.movement.slow_motion.get())
		g_tickbase.m_shift_data.m_ticks_to_shift = 2;

	g_tickbase.m_shift_data.m_ticks_to_shift = 12;
	int m_nTickbase = g_tickbase.m_shift_data.m_ticks_to_shift;

	int* m_pnNewCmds = ( int* )( ( uintptr_t )m_pBuffer - 0x2C );
	int* m_pnBackupCmds = ( int* )( ( uintptr_t )m_pBuffer - 0x30 );

	*m_pnBackupCmds = 0;

	int m_nNewCmds = *m_pnNewCmds;
	int m_nNextCmd = g_csgo.m_cl->m_choked_commands + g_csgo.m_cl->m_last_outgoing_command + 1;
	int m_nTotalNewCmds = std::min( m_nNewCmds + abs( m_nTickbase ), 16 );

	*m_pnNewCmds = m_nTotalNewCmds;

	for( m_nTo = m_nNextCmd - m_nNewCmds + 1; m_nTo <= m_nNextCmd; m_nTo++ ) {
		if( !g_hooks.m_client.GetOldMethod<WriteUsercmdDeltaToBuffer_t>( 24 )( this, m_nSlot, m_pBuffer, m_nFrom, m_nTo, true ) )
			return false;

		m_nFrom = m_nTo;
	}

	CUserCmd* m_pCmd = g_csgo.m_input->GetUserCmd( m_nSlot );
	if( !m_pCmd )
		return true;

	CUserCmd m_ToCmd = *m_pCmd, m_FromCmd = *m_pCmd;
	m_ToCmd.m_command_number++;
	m_ToCmd.m_tick += 3 * g_cl.m_rate;

	for (int i = m_nNewCmds; i <= m_nTotalNewCmds; i++) {
		g_tickbase.WriteUserCmd( ( bf_write* )m_pBuffer, &m_ToCmd, &m_FromCmd );
		m_FromCmd = m_ToCmd;

		m_ToCmd.m_command_number++;
		m_ToCmd.m_tick++;
	}

	g_tickbase.m_shift_data.m_current_shift = m_nTickbase;
	return true;
}

void TickbaseSystem::PreMovement( ) {

	// Invalidate next shift amount and the ticks to shift prior to shifting
	g_tickbase.m_shift_data.m_next_shift_amount = g_tickbase.m_shift_data.m_ticks_to_shift = g_menu.main.movement.slow_motion.get() ? 2 : 12;
}

void TickbaseSystem::PostMovement( ) {
	// Perform sanity checks to make sure we're able to shift
	if( !g_cl.m_processing || !g_menu.main.aimbot.rapidfire.get()) {
		return;
	}

	if( !g_cl.m_cmd || !g_cl.m_weapon) {
		return;
	}

	if(g_cl.m_weapon_id == REVOLVER ||
		g_cl.m_weapon_id == C4 ||
		g_cl.m_weapon_type == WEAPONTYPE_KNIFE ||
		g_cl.m_weapon_type == WEAPONTYPE_GRENADE)
	{
		g_tickbase.m_shift_data.m_did_shift_before = false;
		g_tickbase.m_shift_data.m_should_be_ready = false;
		g_tickbase.m_shift_data.m_should_disable = true;
		return;
	}

	// Don't attempt to shift if we're not supposed to
	if( !g_tickbase.m_shift_data.m_should_attempt_shift ) {
		g_tickbase.m_shift_data.m_did_shift_before = false;
		g_tickbase.m_shift_data.m_should_be_ready = false;
		return;
	}

	g_tickbase.m_shift_data.old_tickbase = g_tickbase.m_shift_data.m_should_attempt_shift;

	g_tickbase.m_shift_data.m_should_disable = false;

	// Setup variables we will later use in order to dermine if we're able to shift tickbase or not

	// This could be made into a checkbox 
	bool bFastRecovery = false;

	float flNonShiftedTime = game::TICKS_TO_TIME( g_cl.m_local->m_nTickBase( ) - g_cl.m_goal_shift );

	// Determine if we are able to shoot right now (at the time of the shift)
	bool bCanShootNormally = g_cl.CanFireWeapon(game::TICKS_TO_TIME(g_cl.m_local->m_nTickBase()));

	// Determine if we are able to shoot in the previous iShiftAmount ticks (iShiftAmount ticks before we shifted)
	bool bCanShootIn12Ticks = g_cl.CanFireWeapon( flNonShiftedTime );

	bool bIsShooting = g_cl.IsFiring(game::TICKS_TO_TIME(g_cl.m_local->m_nTickBase()));

	// Determine if we are able to shift the tickbase respective to previously setup variables (rofl)
	g_tickbase.m_shift_data.m_can_shift_tickbase = bCanShootIn12Ticks || ( !bCanShootNormally || bFastRecovery ) && ( g_tickbase.m_shift_data.m_did_shift_before );

	// If we can shift tickbase, shift enough ticks in order to double-tap
	if( g_tickbase.m_shift_data.m_can_shift_tickbase ) {
		// Tell the cheat to shift tick-base and disable fakelag
		g_tickbase.m_shift_data.m_next_shift_amount = g_cl.m_goal_shift;
		*g_cl.m_packet = true;
	}
	else {
		g_tickbase.m_shift_data.m_next_shift_amount = 0;
		g_tickbase.m_shift_data.m_should_be_ready = false;
	}

	// we want to recharge after stopping fake duck.
	if (g_hvh.m_fake_duck) {
		g_tickbase.m_shift_data.m_prepare_recharge = true;

		g_tickbase.m_shift_data.m_next_shift_amount = 0;
		g_tickbase.m_shift_data.m_should_be_ready = false;

		g_tickbase.m_shift_data.m_should_disable = true;

		return;
	}

	// Are we even supposed to shift tickbase?
	if( g_tickbase.m_shift_data.m_next_shift_amount > 0 ) {
		// Prevent m_iTicksAllowedForProcessing from being incremented.
		g_cl.m_cmd->m_tick = INT_MAX;
		// Determine if we're able to double-tap  
		// note - slow walk shift is such a weird exploit to code.
		if (g_menu.main.antiaim.lbyexploit.get() ? (!g_menu.main.movement.slow_motion.get() && bCanShootIn12Ticks) : bCanShootIn12Ticks) {
			if (g_tickbase.m_shift_data.m_prepare_recharge && !bIsShooting) {
				g_tickbase.m_shift_data.m_needs_recharge = g_cl.m_goal_shift;
				g_tickbase.m_shift_data.m_prepare_recharge = false;
				if (g_menu.main.misc.debug.get())
					g_notify.add(tfm::format(XOR("Attempting doubletap")));
			}
			else {
				if (bIsShooting) {
					// Store shifted command
					g_tickbase.m_prediction.m_shifted_command = g_cl.m_cmd->m_command_number;

					// Store shifted ticks
					g_tickbase.m_prediction.m_shifted_ticks = abs(g_tickbase.m_shift_data.m_current_shift);

					// Store original tickbase
					g_tickbase.m_prediction.m_original_tickbase = g_cl.m_local->m_nTickBase();

					// Update our wish ticks to shift, and later shift tickbase
					g_tickbase.m_shift_data.m_ticks_to_shift = g_tickbase.m_shift_data.m_next_shift_amount;

					// note - this is for preserving fake in desync, but i think it still applies;
					// the maximum amount of ticks we can shift is sv_maxusrcmdprocessticks
					// which is defaulted to 16 - but since we want to be fakelagging atleast 2 ticks
					// for our fake angle to work, we need to reserve 2 ticks for the fakelag. 
					// we want our doubletap to be as fast as possible, and our fake to be as wide as possible
					// therefore let's just fakelag 2 ticks - resulting in our max shift ticks being 14
					// cos sv_maxusrcmdprocessticks Take Away Two Is Fourteen
					// (assuming that the convar wasn't changed to a higher/lower value).
					g_cl.m_goal_shift = 14;
					if (g_tickbase.m_shift_data.old_tickbase != g_tickbase.m_shift_data.m_should_attempt_shift) {
						if (g_menu.main.misc.debug.get())
							g_notify.add(tfm::format(XOR("Tried doubletap")));

						if (g_tickbase.m_shift_data.m_should_attempt_shift)
							g_tickbase.m_shift_data.m_needs_recharge = g_cl.m_goal_shift;
						else
							g_tickbase.m_shift_data.m_needs_recharge = 0;

						g_tickbase.m_shift_data.m_did_shift_before = false;
					}
				}
			}
		}
		else if (g_menu.main.movement.slow_motion.get()) {
			g_cl.m_goal_shift = 8;
			if (g_menu.main.misc.debug.get())
				g_notify.add(tfm::format(XOR("Attempting to shift ticks")));
			if (g_tickbase.m_shift_data.old_tickbase != g_tickbase.m_shift_data.m_should_attempt_shift) {
				if(g_menu.main.misc.debug.get())
					g_notify.add( tfm::format( XOR( "Tried breaking lby using tickbase" )) );

				if (g_tickbase.m_shift_data.m_should_attempt_shift)

					g_tickbase.m_shift_data.m_needs_recharge = 0;
				else
					g_tickbase.m_shift_data.m_needs_recharge = g_cl.m_goal_shift;

				g_tickbase.m_shift_data.m_did_shift_before = false;
			}
		}
		else {
			g_tickbase.m_shift_data.m_prepare_recharge = true;
			g_tickbase.m_shift_data.m_should_be_ready = false;
		}
	}
	else {
		g_tickbase.m_shift_data.m_prepare_recharge = true;
		g_tickbase.m_shift_data.m_should_be_ready = false;
	}

	// Note DidShiftBefore state 
	g_tickbase.m_shift_data.m_did_shift_before = g_tickbase.m_shift_data.m_next_shift_amount > 0;
}