#include "includes.h"

Resolver g_resolver{};;

LagRecord* Resolver::FindIdealRecord( AimPlayer* data ) {
    LagRecord *first_valid, *current;

	if( data->m_records.empty( ) )
		return nullptr;

    first_valid = nullptr;

    // iterate records.
	for( const auto &it : data->m_records ) {
		if( it->dormant( ) || it->immune( ) || !it->valid( ) )
			continue;

        // get current record.
        current = it.get( );

        // first record that was valid, store it for later.
        if( !first_valid )
            first_valid = current;

        // try to find a record with a shot, lby update, walking or no anti-aim.
		if( it->m_shot || it->m_mode == Modes::RESOLVE_BODY || it->m_mode == Modes::RESOLVE_WALK || it->m_mode == Modes::RESOLVE_NONE )
            return current;
	}

	// none found above, return the first valid record if possible.
	return ( first_valid ) ? first_valid : nullptr;
}

LagRecord* Resolver::FindLastRecord( AimPlayer* data ) {
    LagRecord* current;

	if( data->m_records.empty( ) )
		return nullptr;

	// iterate records in reverse.
	for( auto it = data->m_records.crbegin( ); it != data->m_records.crend( ); ++it ) {
		current = it->get( );

		// if this record is valid.
		// we are done since we iterated in reverse.
		if( current->valid( ) && !current->immune( ) && !current->dormant( ) )
			return current;
	}

	return nullptr;
}

void Resolver::OnBodyUpdate( Player* player, float value ) {
	AimPlayer* data = &g_aimbot.m_players[ player->index( ) - 1 ];

	// set data.
	data->m_old_body = data->m_body;
	data->m_body     = value;
}

float Resolver::GetDirectionAngle(int index, Player* player) {
	const auto left_thickness = g_cl.m_left_thickness[index];
	const auto right_thickness = g_cl.m_right_thickness[index];
	const auto at_target_angle = g_cl.m_at_target_angle[index];

	auto angle = 0.f;

	if ((left_thickness >= 350 && right_thickness >= 350) || (left_thickness <= 50 && right_thickness <= 50) || (std::fabs(left_thickness - right_thickness) <= 7)) {
		angle = math::normalize_float(at_target_angle + 180.f);
	}
	else {
		if (left_thickness > right_thickness) {
			angle = math::normalize_float(at_target_angle - 90.f);
		}
		else if (left_thickness == right_thickness) {
			angle = math::normalize_float(at_target_angle + 180.f);
		}
		else {
			angle = math::normalize_float(at_target_angle + 90.f);
		}
	}
	return angle;
}

float Resolver::GetAwayAngle( LagRecord* record ) {
	float  delta{ std::numeric_limits< float >::max( ) };
	vec3_t pos;
	ang_t  away;

	// other cheats predict you by their own latency.
	// they do this because, then they can put their away angle to exactly
	// where you are on the server at that moment in time.

	// the idea is that you would need to know where they 'saw' you when they created their user-command.
	// lets say you move on your client right now, this would take half of our latency to arrive at the server.
	// the delay between the server and the target client is compensated by themselves already, that is fortunate for us.

	// we have no historical origins.
	// no choice but to use the most recent one.
	//if( g_cl.m_net_pos.empty( ) ) {
		math::VectorAngles( g_cl.m_local->m_vecOrigin( ) - record->m_pred_origin, away );
		return away.y;
	//}

	// half of our rtt.
	// also known as the one-way delay.
	//float owd = ( g_cl.m_latency / 2.f );

	// since our origins are computed here on the client
	// we have to compensate for the delay between our client and the server
	// therefore the OWD should be subtracted from the target time.
	//float target = record->m_pred_time; //- owd;

	// iterate all.
	//for( const auto &net : g_cl.m_net_pos ) {
		// get the delta between this records time context
		// and the target time.
	//	float dt = std::abs( target - net.m_time );

		// the best origin.
	//	if( dt < delta ) {
	//		delta = dt;
	//		pos   = net.m_pos;
	//	}
	//}

	//math::VectorAngles( pos - record->m_pred_origin, away );
	//return away.y;
}

void Resolver::MatchShot( AimPlayer* data, LagRecord* record ) {
	// do not attempt to do this in nospread mode.
	if( g_menu.main.config.mode.get( ) == 1 )
		return;

	float shoot_time = -1.f;

	Weapon* weapon = data->m_player->GetActiveWeapon( );
	if( weapon ) {
		// with logging this time was always one tick behind.
		// so add one tick to the last shoot time.
		shoot_time = weapon->m_fLastShotTime( ) + g_csgo.m_globals->m_interval;
	}

	// this record has a shot on it.
	if( game::TIME_TO_TICKS( shoot_time ) == game::TIME_TO_TICKS( record->m_sim_time ) ) {
		if( record->m_lag <= 2 )
			record->m_shot = true;
		
		// more then 1 choke, cant hit pitch, apply prev pitch.
		else if( data->m_records.size( ) >= 2 ) {
			LagRecord* previous = data->m_records[ 1 ].get( );

			if( previous && !previous->dormant( ) )
				record->m_eye_angles.x = previous->m_eye_angles.x;
		}
	}
}

void Resolver::Override(LagRecord* record)
{
	int     closestfov{ 180 };
	Player* nearest_player = nullptr;

	for (int i = 1; i <= g_csgo.m_globals->m_max_clients; i++)
	{
		Player* player = g_csgo.m_entlist->GetClientEntity< Player* >(i);

		if (!player)
			continue;

		float fov = math::GetFOV(g_cl.m_view_angles, g_cl.m_shoot_pos, player->GetShootPosition());
		if (fov < closestfov) {
			nearest_player = player;
			closestfov = fov;
		}
	}

	if (record->m_player != nearest_player)
		return;

	// get away angle.
	float away = GetAwayAngle(record);

	// find distance between my cursor -> enemy.
	float delta = g_cl.m_view_angles.y - away;
	math::NormalizeAngle(delta);

	// apply angle.
	value = delta;

	if (delta >= 170 || delta <= -170)
	{
		// back
		record->m_eye_angles.y = away + 180;

		//record->m_resolver_mode = "OVERRIDE_BACK";
	}
	else if (delta < 170 && delta > 0)
	{
		// right
		record->m_eye_angles.y = away + 90.f;

		//record->m_resolver_mode = "OVERRIDE_RIGHT";
	}
	else if (delta > -170 && delta < 0)
	{
		// left
		record->m_eye_angles.y = away - 90.f;

		//record->m_resolver_mode = "OVERRIDE_LEFT";
	}
}

void Resolver::AntiFreestand(LagRecord* record) {
	// constants
	constexpr float STEP{ 4.f };
	constexpr float RANGE{ 32.f };

	// best target.
	vec3_t enemypos = record->m_player->GetShootPosition();
	float away = GetAwayAngle(record);

	// construct vector of angles to test.
	std::vector< AdaptiveAngle > angles{ };
	angles.emplace_back(away - 180.f);
	angles.emplace_back(away + 90.f);
	angles.emplace_back(away - 90.f);

	// start the trace at the your shoot pos.
	vec3_t start = g_cl.m_local->GetShootPosition();

	// see if we got any valid result.
	// if this is false the path was not obstructed with anything.
	bool valid{ false };

	// iterate vector of angles.
	for (auto it = angles.begin(); it != angles.end(); ++it) {

		// compute the 'rough' estimation of where our head will be.
		vec3_t end{ enemypos.x + std::cos(math::deg_to_rad(it->m_yaw)) * RANGE,
			enemypos.y + std::sin(math::deg_to_rad(it->m_yaw)) * RANGE,
			enemypos.z };

		// draw a line for debugging purposes.
		if (g_menu.main.misc.debug.get()) {
			g_csgo.m_debug_overlay->AddLineOverlay(start, end, 0, 255, 255, true, 0.1f);
			g_notify.add("[DEBUG] tried anti-freestand");
		}

		// compute the direction.
		vec3_t dir = end - start;
		float len = dir.normalize();

		// should never happen.
		if (len <= 0.f)
			continue;

		// step thru the total distance, 4 units per step.
		for (float i{ 0.f }; i < len; i += STEP) {
			// get the current step position.
			vec3_t point = start + (dir * i);

			// get the contents at this point.
			int contents = g_csgo.m_engine_trace->GetPointContents(point, MASK_SHOT_HULL);

			// contains nothing that can stop a bullet.
			if (!(contents & MASK_SHOT_HULL))
				continue;

			float mult = 1.f;

			// over 50% of the total length, prioritize this shit.
			if (i > (len * 0.5f))
				mult = 1.25f;

			// over 90% of the total length, prioritize this shit.
			if (i > (len * 0.75f))
				mult = 1.25f;

			// over 90% of the total length, prioritize this shit.
			if (i > (len * 0.9f))
				mult = 2.f;

			// append 'penetrated distance'.
			it->m_dist += (STEP * mult);

			// mark that we found anything.
			valid = true;
		}
	}

	if (!valid) {
		return;
	}

	// put the most distance at the front of the container.
	std::sort(angles.begin(), angles.end(),
		[](const AdaptiveAngle& a, const AdaptiveAngle& b) {
			return a.m_dist > b.m_dist;
		});

	// the best angle should be at the front now.
	AdaptiveAngle* best = &angles.front();

	record->m_eye_angles.y = best->m_yaw;
}

void Resolver::SetMode( AimPlayer* data, LagRecord* record ) {
	// the resolver has 3 modes to chose from.
	// these modes will vary more under the hood depending on what data we have about the player
	// and what kind of hack vs. hack we are playing (mm/nospread).

	float speed = record->m_anim_velocity.length( );

	// if on ground, moving, and not fakewalking.
	if ((record->m_flags & FL_ONGROUND) && speed > 0.1f && !record->m_fake_walk) {
		if ((record->m_flags & FL_ONGROUND) && speed > (INT_MAX - 4))
			record->m_mode = Modes::RESOLVE_EXPLOIT;
		else {
			record->m_mode = Modes::RESOLVE_WALK;
		}
	}

	// if on ground, not moving or fakewalking.
	if( ( record->m_flags & FL_ONGROUND ) && ( speed <= 0.1f || record->m_fake_walk ) )
		if(data->m_moved)
		record->m_mode = Modes::RESOLVE_STAND;

	// if not on ground.
	else if( !( record->m_flags & FL_ONGROUND ) )
		record->m_mode = Modes::RESOLVE_AIR;
}

void Resolver::ResolveAngles( Player* player, LagRecord* record ) {
	AimPlayer* data = &g_aimbot.m_players[ player->index( ) - 1 ];
	
	bool printDebug = true;
	

	if (g_cl.m_round_end)
		ResetNiggaShit(data, printDebug);

	// mark this record if it contains a shot.
	MatchShot( data, record );

	// next up mark this record with a resolver mode that will be used.
	SetMode( data, record );

	// if we are in nospread mode, force all players pitches to down.
	// TODO; we should check thei actual pitch and up too, since those are the other 2 possible angles.
	// this should be somehow combined into some iteration that matches with the air angle iteration.
	if( g_menu.main.config.mode.get( ) == 1 )
		record->m_eye_angles.x = 90.f;

	/* note to self - PUT THE RECORD MODE HERE TO CALL THE ACTUAL RESOLVER YOU IDIOT */


	// we arrived here we can do the acutal resolve.
	if (record->m_mode == Modes::RESOLVE_WALK)
		ResolveWalk(data, record);

	else if (record->m_mode == Modes::RESOLVE_EXPLOIT)
		ExploitFix(data, record);

	else if (record->m_mode == Modes::RESOLVE_STAND)
		ResolveStand(data, record);

	else if (record->m_mode == Modes::RESOLVE_AIR)
		ResolveAir(data, record);

	else if (record->m_mode == Modes::RESOLVE_FREESTAND)
		AntiFreestand(record);

	else if (record->m_mode == Modes::RESOLVE_LAST_LBY)
		LastMoveLby(record, data, player);

	else if (record->m_mode == Modes::RESOLVE_OVERRIDE)
		Override(record);
		




	// normalize the eye angles, doesn't really matter but its clean.
	math::NormalizeAngle( record->m_eye_angles.y );
}

void Resolver::ResolveWalk( AimPlayer* data, LagRecord* record ) {
	// apply lby to eyeangles.
	record->m_eye_angles.y = record->m_body;

	// delay body update.
	data->m_body_update = record->m_anim_time + 0.22f;

	// reset stand and body index.
	data->m_moving_index = 0;
	data->m_stand_index = 0;
	data->m_stand_index2 = 0;
	data->m_body_index = 0;
	data->m_freestanding_index = 0;


	// copy the last record that this player was walking
	// we need it later on because it gives us crucial data.
	std::memcpy( &data->m_walk_record, record, sizeof( LagRecord ) );
}

void Resolver::ResetNiggaShit(AimPlayer* data, bool printDebug) {
	if (g_menu.main.misc.debug.get())
		g_notify.add("[DEBUG] reset resolver info");

	data->m_moving_index = 0;
	data->m_stand_index = 0;
	data->m_stand_index2 = 0;
	data->m_body_index = 0;
	data->m_freestanding_index = 0;
}

float Resolver::GetLBYRotatedYaw(float lby, float yaw)
{
	float delta = math::NormalizedAngle(yaw - lby);
	if (fabs(delta) < 25.f)
		return lby;

	if (delta > 0.f)
		return yaw + 25.f;

	return yaw;
}

bool Resolver::IsYawSideways(Player* entity, float yaw)
{
	auto local_player = g_cl.m_local;
	if (!local_player)
		return false;

	const auto at_target_yaw = math::CalcAngle(local_player->m_vecOrigin(), entity->m_vecOrigin()).y;
	const float delta = fabs(math::NormalizedAngle(at_target_yaw - yaw));

	return delta > 20.f && delta < 160.f;
}

void Resolver::LastMoveLby(LagRecord* record, AimPlayer* data, Player* player)
{
	// for no-spread call a seperate resolver.
	if (g_menu.main.config.mode.get() == 1) {
		StandNS(data, record);
		return;
	}

	// pointer for easy access.
	LagRecord* move = &data->m_walk_record;

	// get predicted away angle for the player.
	float away = GetAwayAngle(record);

	C_AnimationLayer* curr = &record->m_layers[3];
	int act = data->m_player->GetSequenceActivity(curr->m_sequence);

	float diff = math::NormalizedAngle(record->m_body - move->m_body);
	float delta = record->m_anim_time - move->m_anim_time;

	ang_t vAngle = ang_t(0, 0, 0);
	math::CalcAngle3(player->m_vecOrigin(), g_cl.m_local->m_vecOrigin(), vAngle);

	float flToMe = vAngle.y;

	const float at_target_yaw = math::CalcAngle(g_cl.m_local->m_vecOrigin(), player->m_vecOrigin()).y;

	//g_visuals.fakeangels = true;

	/*for (int i = 1; i <= 32; i++)
	{
		Player* pEnemy = g_csgo.m_entlist->GetClientEntity< Player* >(i);
		const auto freestanding_record = player_resolve_records[i].m_sAntiEdge;

		AntiFreestand(player, record->m_eye_angles.y, freestanding_record.left_damage, freestanding_record.right_damage, freestanding_record.right_fraction, freestanding_record.left_fraction, flToMe, data->m_last_move);
	}*/

	//const auto freestanding_record = player_resolve_records[player->index()].m_sAntiEdge;

	// we have a valid moving record.
	if (move->m_sim_time > 0.f) {
		vec3_t delta = move->m_origin - record->m_origin;

		// check if moving record is close.
		if (delta.length() <= 128.f) {
			// indicate that we are using the moving lby.
			data->m_moved = true;
		}
	}

	if (!data->m_moved) {

		//record->m_mode = Modes::RESOLVE_UNKNOWM;

		record->m_eye_angles.y = GetDirectionAngle(player->index(), player);

		//ResolveYawBruteforce(record, player, data);

		/*
			const auto left_thickness = g_cl.m_left_thickness[index];
			const auto right_thickness = g_cl.m_right_thickness[index];
			const auto at_target_angle = g_cl.m_at_target_angle[index];
		*/

		//AntiFreestand(player, record->m_eye_angles.y, freestanding_record.left_damage, freestanding_record.right_damage, freestanding_record.right_fraction, freestanding_record.left_fraction, at_target_yaw, data->m_last_move);

		if (data->m_body != data->m_old_body)
		{
			record->m_eye_angles.y = record->m_body;

			data->m_body_update = record->m_anim_time + 1.125f;

			//g_visuals.fakeangels = false;
			record->m_mode = Modes::RESOLVE_BODY;
		}
	}
	else if (data->m_moved) {
		float diff = math::NormalizedAngle(record->m_body - move->m_body);
		float delta = record->m_anim_time - move->m_anim_time;

		record->m_mode = Modes::RESOLVE_LASTMOVE;
		data->m_last_move;

		const float at_target_yaw = math::CalcAngle(g_cl.m_local->m_vecOrigin(), player->m_vecOrigin()).y;

		if (IsYawSideways(player, move->m_body)) // anti-urine
			record->m_eye_angles.y = move->m_body;
		else
			record->m_eye_angles.y = away + 180.f;

		record->m_eye_angles.y = GetLBYRotatedYaw(player->m_flLowerBodyYawTarget(), move->m_body);

		if (data->m_last_move >= 1)
			//ResolveYawBruteforce(record, player, data);

		record->m_eye_angles.y = GetDirectionAngle(player->index(), player);

		if (data->m_body != data->m_old_body)
		{
			auto lby = math::normalize_float(record->m_body);
			if (fabsf(record->m_eye_angles.y - lby) <= 150.f && fabsf(record->m_eye_angles.y - lby) >= 35.f) {
				record->m_eye_angles.y ? lby -= 25.f : lby += 25.f;
			}
			record->m_eye_angles.y = lby;
			player->SetAbsAngles(ang_t(0.f, lby, 0.f));

			data->m_body_update = record->m_anim_time + 1.125f;
			//g_visuals.fakeangels = false;
			record->m_mode = Modes::RESOLVE_BODY;
		}
		else
		{
			// LBY SHOULD HAVE UPDATED HERE.
			if (record->m_anim_time >= data->m_body_update) {
				// only shoot the LBY flick 3 times.
				// if we happen to miss then we most likely mispredicted
				if (data->m_body_index < 1) {
					// set angles to current LBY.
					record->m_eye_angles.y = record->m_body;

					data->m_body_update = record->m_anim_time + 1.125f;

					// set the resolve mode.
					//g_visuals.fakeangels = false;
					record->m_mode = Modes::RESOLVE_BODY;
				}
			}
		}
		//if (data->m_last_move > 1)
			//AntiFreestand(player, record->m_eye_angles.y, freestanding_record.left_damage, freestanding_record.right_damage, freestanding_record.right_fraction, freestanding_record.left_fraction, flToMe, data->m_last_move);
	}
}

void Resolver::ResolveStand(AimPlayer* data, LagRecord* record) {
	// get predicted away angle for the player.
	float away = GetAwayAngle(record);

	// pointer for easy access.
	LagRecord* move = &data->m_walk_record;
	float delta = record->m_anim_time - move->m_anim_time;

	// we have a valid moving record.
	if (move->m_sim_time > 0.f && !move->dormant() && !record->dormant()) {
		vec3_t delta = move->m_origin - record->m_origin;

		// check if moving record is close.
		if (delta.length() <= 128.f) {
			// indicate that we are using the moving lby.
			data->m_moved = true;
		}
	}

	if (delta < .22f) // can't flick if they moved .22 seconds ago so its just their lby
	{
		record->m_eye_angles.y = move->m_body;

		record->m_mode = Modes::RESOLVE_STOPPED_MOVING;
	}
	else if (data->m_body != data->m_old_body && data->m_body_index <= 5) // flick prediction, has .22 flick predict
	{
		record->m_eye_angles.y = data->m_body;

		data->m_body_update = record->m_anim_time + 1.1f;

		record->m_mode = Modes::RESOLVE_LBY_UPDATE;
	}
	else if (g_input.GetKeyState(g_menu.main.aimbot.override_key.get()))
	{
		Override(record);
	}
	else if (data->m_moved && data->m_moving_index < 1) // last moving if we havn't missed it
	{
		record->m_eye_angles.y = move->m_body;

		record->m_mode = Modes::RESOLVE_LAST_LBY;
	}
	else if (!data->m_moved || data->m_moving_index >= 1 && data->m_freestanding_index < 3) // freestanding if we have gone through that shit or missed last moving
	{
		AntiFreestand(record);

		record->m_mode = Modes::RESOLVE_FREESTAND;
		//record->m_resolver_mode = "FREESTANDING";
	}
	else // bruteforce as a last fallback
	{
		switch (data->m_stand_index2 % 4)
		{
		case 1:
			record->m_eye_angles.y -= away + record->m_body;
			break;

		case 2:
			record->m_eye_angles.y += away - 12.f - record->m_body;
			break;

		case 3:
			record->m_eye_angles.y -= away + 180.f + record->m_body;
			break;


		case 4:
			record->m_eye_angles.y = away - 130.f + record->m_body;
			break;
		}

		record->m_mode = Modes::RESOLVE_BRUTEFORCE;
	}
}

void Resolver::ExploitFix(AimPlayer* data, LagRecord* record) {
	// apply lby to eyeangles.
	record->m_eye_angles.y = record->m_body;

	// delay body update.
	data->m_body_update = record->m_anim_time + 0.22f;

	// reset stand and body index.
	data->m_moving_index = 0;
	data->m_stand_index = 0;
	data->m_stand_index2 = 0;
	data->m_body_index = 0;
	data->m_freestanding_index = 0;


	// copy the last record that this player was walking
	// we need it later on because it gives us crucial data.
	std::memcpy(&data->m_walk_record, record, sizeof(LagRecord));
}

void Resolver::StandNS( AimPlayer* data, LagRecord* record ) {
	// get away angles.
	float away = GetAwayAngle( record );

	switch( data->m_stand_index2 % 9 ) {

	case 0:
		record->m_eye_angles.y -= away + record->m_body;
		break;

	case 1:
		record->m_eye_angles.y += away - 12.f - record->m_body;
		break;

	case 2:
		record->m_eye_angles.y -= away + 180.f + record->m_body;
		break;

	case 3:
		record->m_eye_angles.y += away - 30.f - record->m_body;
		break;

	case 4:
		record->m_eye_angles.y -= away + 98.f + record->m_body;
		break;

	case 5:
		record->m_eye_angles.y += away - 2.f - record->m_body;
		break;

	case 6:
		record->m_eye_angles.y -= away + 150.f + record->m_body;
		break;

	case 7:
		record->m_eye_angles.y += away - 119.f - record->m_body;
		break;

	case 8:
		record->m_eye_angles.y -= away + 180.f + record->m_body;
		break;
	default:
		break;
	}
}

void Resolver::ResolveAir( AimPlayer* data, LagRecord* record ) {
	// for no-spread call a seperate resolver.
	if( g_menu.main.config.mode.get( ) == 1 ) {
		AirNS( data, record );
		return;
	}

	// else run our matchmaking air resolver.

	// we have barely any speed. 
	// either we jumped in place or we just left the ground.
	// or someone is trying to fool our resolver.
	if( record->m_velocity.length_2d( ) < 60.f ) {
		// set this for completion.
		// so the shot parsing wont pick the hits / misses up.
		// and process them wrongly.
		record->m_mode = Modes::RESOLVE_STAND;

		// invoke our stand resolver.
		ResolveStand( data, record );

		// we are done.
		return;
	}

	// try to predict the direction of the player based on his velocity direction.
	// this should be a rough estimation of where he is looking.
	float velyaw = math::rad_to_deg( std::atan2( record->m_velocity.y, record->m_velocity.x ) );

	float away = GetAwayAngle(record);

	switch( data->m_shots % 9 ) {
	case 0:
		record->m_eye_angles.y -= away + record->m_body + velyaw;
		break;

	case 1:
		record->m_eye_angles.y += away - 12.f - record->m_body + velyaw;
		break;

	case 2:
		record->m_eye_angles.y -= away + 180.f + record->m_body + velyaw;
		break;

	case 3:
		record->m_eye_angles.y += away - 30.f - record->m_body + velyaw;
		break;

	case 4:
		record->m_eye_angles.y -= away + 98.f + record->m_body + velyaw;
		break;

	case 5:
		record->m_eye_angles.y += away - 2.f - record->m_body + velyaw;
		break;

	case 6:
		record->m_eye_angles.y -= away + 150.f + record->m_body + velyaw;
		break;

	case 7:
		record->m_eye_angles.y += away - 119.f - record->m_body + velyaw;
		break;

	case 8:
		record->m_eye_angles.y -= away + 180.f + record->m_body + velyaw;
		break;
	default:
		break;
	}
}

void Resolver::AirNS( AimPlayer* data, LagRecord* record ) {
	// get away angles.
	float away = GetAwayAngle( record );

	switch( data->m_shots % 9 ) {
	case 0:
		record->m_eye_angles.y = away + 180.f;
		break;

	case 1:
		record->m_eye_angles.y = away + 150.f;
		break;
	case 2:
		record->m_eye_angles.y = away - 150.f;
		break;

	case 3:
		record->m_eye_angles.y = away + 165.f;
		break;
	case 4:
		record->m_eye_angles.y = away - 165.f;
		break;

	case 5:
		record->m_eye_angles.y = away + 135.f;
		break;
	case 6:
		record->m_eye_angles.y = away - 135.f;
		break;

	case 7:
		record->m_eye_angles.y = away + 90.f;
		break;
	case 8:
		record->m_eye_angles.y = away - 90.f;
		break;

	default:
		break;
	}
}

void Resolver::ResolvePoses( Player* player, LagRecord* record ) {
	AimPlayer* data = &g_aimbot.m_players[ player->index( ) - 1 ];


	// only do this bs when in air.
	if( record->m_mode == Modes::RESOLVE_AIR ) {
		// ang = pose min + pose val x ( pose range )

		// lean_yaw
		player->m_flPoseParameter( )[ 2 ]  = g_csgo.RandomInt( 0, 4 ) * 0.25f;   

		// body_yaw
		player->m_flPoseParameter( )[ 11 ] = g_csgo.RandomInt( 1, 3 ) * 0.25f;
	}
}