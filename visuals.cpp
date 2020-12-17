#include "includes.h"

Visuals g_visuals{ };;

void Visuals::ModulateWorld( ) {

	std::vector< IMaterial* > world, skybox, props;

	// iterate material handles.
	for( uint16_t h{ g_csgo.m_material_system->FirstMaterial( ) }; h != g_csgo.m_material_system->InvalidMaterial( ); h = g_csgo.m_material_system->NextMaterial( h ) ) {
		// get material from handle.
		IMaterial* mat = g_csgo.m_material_system->GetMaterial( h );
		if( !mat )
			continue;

		if( mat->IsErrorMaterial( ) )
			continue;

		// store skybox mats
		if (FNV1a::get( mat->GetTextureGroupName( ) ) == HASH( "SkyBox textures" ) )
			skybox.push_back( mat );

		// store world materials.
		else if( FNV1a::get( mat->GetTextureGroupName( ) ) == HASH( "World textures" ) )
			world.push_back( mat );

		// store props.
		else if( FNV1a::get( mat->GetTextureGroupName( ) ) == HASH( "StaticProp textures" ) )
			props.push_back( mat );

	}

	// night.
	if (g_menu.main.visuals.world.get() == 1) {

		for (const auto& w : world)
			w->ColorModulate(0.15f, 0.15f, 0.15f);

		// IsUsingStaticPropDebugModes my nigga
		if (g_csgo.r_DrawSpecificStaticProp->GetInt() != 0) {
			g_csgo.r_DrawSpecificStaticProp->SetValue(0);
		}

		for (const auto& p : props)
			p->ColorModulate(0.15f, 0.15f, 0.15f);

		for (const auto& s : skybox)
			s->ColorModulate( 228.f / 255.f, 35.f / 255.f, 157.f / 255.f );
			

		// change skybox.
		game::SetSkybox(XOR("sky_csgo_night02"));
	}

	// disable night.
	else {
		for( const auto& w : world )
			w->ColorModulate( 1.f, 1.f, 1.f );

		// restore r_DrawSpecificStaticProp.
		if( g_csgo.r_DrawSpecificStaticProp->GetInt( ) != -1 ) {
			g_csgo.r_DrawSpecificStaticProp->SetValue( -1 );
		}

		for( const auto& p : props )
			p->ColorModulate( 1.f, 1.f, 1.f );

		for (const auto& s : skybox)
			s->ColorModulate(1.f, 1.f, 1.f);

		// vertigo!
		game::SetSkybox(XOR("vertigoblue_hdr"));
	}

	// transparent props.
	if( g_menu.main.visuals.transparent_props.get( ) ) {

		// IsUsingStaticPropDebugModes my nigga
		if( g_csgo.r_DrawSpecificStaticProp->GetInt( ) != 0 ) {
			g_csgo.r_DrawSpecificStaticProp->SetValue( 0 );
		}

		for( const auto& p : props )
			p->AlphaModulate( 0.85f );
	}

	// disable transparent props.
	else {

		// restore r_DrawSpecificStaticProp.
		if( g_csgo.r_DrawSpecificStaticProp->GetInt( ) != -1 ) {
			g_csgo.r_DrawSpecificStaticProp->SetValue( -1 );
		}

		for( const auto& p : props )
			p->AlphaModulate( 1.0f );
	}
}

void Visuals::ThirdpersonThink( ) {
	ang_t                          offset;
	vec3_t                         origin, forward;
	static CTraceFilterSimple_game filter{ };
	CGameTrace                     tr;
	static float				   progress{ };

	// for whatever reason overrideview also gets called from the main menu.
	if( !g_csgo.m_engine->IsInGame( ) )
		return;

	// check if we have a local player and he is alive.
	bool alive = g_cl.m_local && g_cl.m_local->alive( );

	// camera should be in thirdperson.
	if( m_thirdperson || !g_cl.m_processing) {

		// if alive and not in thirdperson already switch to thirdperson.
		if( alive && !g_csgo.m_input->CAM_IsThirdPerson( ) )
			g_csgo.m_input->CAM_ToThirdPerson( );

		// if dead and spectating in firstperson switch to thirdperson.
		else if( g_cl.m_local->m_iObserverMode( ) == 4 ) {

			// if in thirdperson, switch to firstperson.
			// we need to disable thirdperson to spectate properly.
			if( g_csgo.m_input->CAM_IsThirdPerson( ) ) {
				g_csgo.m_input->CAM_ToFirstPerson( );
				g_csgo.m_input->m_camera_offset.z = 0.f;
			}

			g_cl.m_local->m_iObserverMode( ) = 5;
		}
	}

	// camera should be in firstperson.
	else if( g_csgo.m_input->CAM_IsThirdPerson( ) ) {
		// animate backwards.
		progress -= g_csgo.m_globals->m_frametime * 7.f + (progress / 100);

		// clamp.
		math::clamp(progress, 0.f, 1.f);
		g_csgo.m_input->m_camera_offset.z = g_menu.main.visuals.thirdperson_distance.get() * progress;

		// set to first person.
		if (!progress)
		g_csgo.m_input->CAM_ToFirstPerson( );
	}

	// if after all of this we are still in thirdperson.
	if( g_csgo.m_input->CAM_IsThirdPerson( ) ) {
		// get camera angles.
		g_csgo.m_engine->GetViewAngles( offset );

		// get our viewangle's forward directional vector.
		math::AngleVectors( offset, &forward );

		// start the animation.
		if (m_thirdperson)
			progress += g_csgo.m_globals->m_frametime * 7.f + (progress / 100);

		// clamp. 
		math::clamp(progress, 0.f, 1.f);

		// cam_idealdist convar.
		offset.z = g_menu.main.visuals.thirdperson_distance.get( ) * progress;

		// start pos.
		origin = g_cl.m_shoot_pos;

		// setup trace filter and trace.
		filter.SetPassEntity( g_cl.m_local );

		g_csgo.m_engine_trace->TraceRay(
			Ray( origin, origin - ( forward * offset.z ), { -16.f, -16.f, -16.f }, { 16.f, 16.f, 16.f } ),
			MASK_NPCWORLDSTATIC,
			( ITraceFilter* ) &filter,
			&tr
		);

		// adapt distance to travel time.
		math::clamp( tr.m_fraction, 0.f, 1.f );
		offset.z *= tr.m_fraction;

		// override camera angles.
		g_csgo.m_input->m_camera_offset = { offset.x, offset.y, offset.z };
	}

	// update the old value.
	m_old_thirdperson = m_thirdperson;
}

void Visuals::Hitmarker( ) {
	if (g_menu.main.misc.hitmarker.get(0)) {

		if (g_csgo.m_globals->m_curtime > m_hit_end)
			return;

		if (m_hit_duration <= 0.f)
			return;

		float complete = (g_csgo.m_globals->m_curtime - m_hit_start) / m_hit_duration;
		int x = g_cl.m_width,
			y = g_cl.m_height,
			alpha = (1.f - complete) * 255;

		constexpr int line{ 11 };
		constexpr int linesize{ 6 };

		render::line(x / 2 - linesize, y / 2 - linesize, x / 2 - line, y / 2 - line, { 200, 200, 200, alpha });
		render::line(x / 2 - linesize, y / 2 + linesize, x / 2 - line, y / 2 + line, { 200, 200, 200, alpha });
		render::line(x / 2 + linesize, y / 2 + linesize, x / 2 + line, y / 2 + line, { 200, 200, 200, alpha });
		render::line(x / 2 + linesize, y / 2 - linesize, x / 2 + line, y / 2 - line, { 200, 200, 200, alpha });
	}
}

void Visuals::PlayerHurt(IGameEvent* pEvent) {
	Player* attacker = g_csgo.m_entlist->GetClientEntity<Player*>(g_csgo.m_engine->GetPlayerForUserID(pEvent->m_keys->FindKey(HASH("attacker"))->GetInt()));
	Player* victim = g_csgo.m_entlist->GetClientEntity<Player*>(g_csgo.m_engine->GetPlayerForUserID(pEvent->m_keys->FindKey(HASH("userid"))->GetInt()));

	if (!attacker || !victim || attacker != g_cl.m_local)
		return;

	vec3_t enemy_pos = victim->m_vecOrigin();

	ImpactInfo_t best_impact;

	float best_impact_distance = 9999999999.f;

	long long time = util::epoch_time();

	std::vector<ImpactInfo_t>::iterator iter;

	for (iter = m_impacts.begin(); iter != m_impacts.end(); ) {
		if (time > iter->time + 25) {
			iter = m_impacts.erase(iter);
			continue;
		}

		vec3_t position = vec3_t(iter->x, iter->y, iter->z);

		float distance = position.dist_to(enemy_pos);

		if (distance < best_impact_distance) {
			best_impact_distance = distance;
			best_impact = *iter;
		}
		iter++;
	}

	if (best_impact_distance == 9999999999.f)
		return;

	HitmarkerInfo_t info;
	info.impact = best_impact;
	info.alpha = 255;
	info.damage = pEvent->m_keys->FindKey(HASH("dmg_health"))->GetInt();
	info.kill = pEvent->m_keys->FindKey(HASH("health"))->GetInt() <= 0;
	info.didhs = pEvent->m_keys->FindKey(HASH("hitgroup"))->GetInt() == 1;
	m_hitmarkers.push_back(info);
}

void Visuals::BulletImpact(IGameEvent* pEvent) {
	Player* shooter = g_csgo.m_entlist->GetClientEntity<Player*>(g_csgo.m_engine->GetPlayerForUserID(pEvent->m_keys->FindKey(HASH("userid"))->GetInt()));

	if (!shooter || shooter != g_cl.m_local)
		return;

	ImpactInfo_t info;
	info.x = pEvent->m_keys->FindKey(HASH("x"))->GetFloat();
	info.y = pEvent->m_keys->FindKey(HASH("y"))->GetFloat();
	info.z = pEvent->m_keys->FindKey(HASH("z"))->GetFloat();
	info.time = util::epoch_time();
	m_impacts.push_back(info);
}

void Visuals::HitmarkerWorld() {
	if (!g_cl.m_processing || !g_csgo.m_engine->IsInGame()) {
		if (!m_impacts.empty())
			m_impacts.clear();

		if (!m_hitmarkers.empty())
			m_hitmarkers.clear();

		return;
	}

	long long time = util::epoch_time();

	std::vector<HitmarkerInfo_t>::iterator iter;

	int m_actual_size = m_hitmarkers.size() - 1;

	for (iter = m_hitmarkers.begin(); iter != m_hitmarkers.end(); ) {
		bool expired = time > iter->impact.time + 4000; // 2000

		if (expired)
			iter->alpha -= 255 / 75;

		if (expired && iter->alpha <= 0) {
			iter = m_hitmarkers.erase(iter);
			m_actual_size--;
			continue;
		}

		auto m_end = m_hitmarkers[m_actual_size];

		vec3_t m_end_pos3D = vec3_t(m_end.impact.x, m_end.impact.y, m_end.impact.z);
		vec3_t m_pos3D = vec3_t(iter->impact.x, iter->impact.y, iter->impact.z);

		vec2_t pos2D;

		if (!render::WorldToScreen(m_pos3D, pos2D)) {
			++iter;
			continue;
		}

		int m_line_size = 8;

		std::string damage_text;
		damage_text += std::to_string(iter->damage);

		const auto color = iter->kill ? Color(255, 0, 0, iter->alpha) : Color(255, 255, 255, iter->alpha);

		if (g_menu.main.misc.hitmarker.get(1)) {
			render::line(vec2_t(int(pos2D.x - m_line_size / 2), int(pos2D.y - m_line_size / 2)), vec2_t(int(pos2D.x - m_line_size), int(pos2D.y - m_line_size)), Color(255, 255, 255, iter->alpha));
			render::line(vec2_t(int(pos2D.x - m_line_size / 2), int(pos2D.y + m_line_size / 2)), vec2_t(int(pos2D.x - m_line_size), int(pos2D.y + m_line_size)), Color(255, 255, 255, iter->alpha));
			render::line(vec2_t(int(pos2D.x + m_line_size / 2), int(pos2D.y + m_line_size / 2)), vec2_t(int(pos2D.x + m_line_size), int(pos2D.y + m_line_size)), Color(255, 255, 255, iter->alpha));
			render::line(vec2_t(int(pos2D.x + m_line_size / 2), int(pos2D.y - m_line_size / 2)), vec2_t(int(pos2D.x + m_line_size), int(pos2D.y - m_line_size)), Color(255, 255, 255, iter->alpha));
		}

		if (g_menu.main.misc.hitmarker.get(2) && m_pos3D == m_end_pos3D)
			render::hitmarker_text.string(pos2D.x, g_menu.main.misc.hitmarker.get(1) ? pos2D.y - 26 : pos2D.y, Color(255, 255, 255, iter->alpha), damage_text.c_str(), render::StringFlags_t::ALIGN_CENTER);

		++iter;
	}
}

void Visuals::NoSmoke( ) {
	if( !smoke1 )
		smoke1 = g_csgo.m_material_system->FindMaterial( XOR( "particle/vistasmokev1/vistasmokev1_fire" ), XOR( "Other textures" ) );

	if( !smoke2 )
		smoke2 = g_csgo.m_material_system->FindMaterial( XOR( "particle/vistasmokev1/vistasmokev1_smokegrenade" ), XOR( "Other textures" ) );

	if( !smoke3 )
		smoke3 = g_csgo.m_material_system->FindMaterial( XOR( "particle/vistasmokev1/vistasmokev1_emods" ), XOR( "Other textures" ) );

	if( !smoke4 )
		smoke4 = g_csgo.m_material_system->FindMaterial( XOR( "particle/vistasmokev1/vistasmokev1_emods_impactdust" ), XOR( "Other textures" ) );

	if( g_menu.main.visuals.nosmoke.get( ) ) {
		if( !smoke1->GetFlag( MATERIAL_VAR_NO_DRAW ) )
			smoke1->SetFlag( MATERIAL_VAR_NO_DRAW, true );

		if( !smoke2->GetFlag( MATERIAL_VAR_NO_DRAW ) )
			smoke2->SetFlag( MATERIAL_VAR_NO_DRAW, true );

		if( !smoke3->GetFlag( MATERIAL_VAR_NO_DRAW ) )
			smoke3->SetFlag( MATERIAL_VAR_NO_DRAW, true );

		if( !smoke4->GetFlag( MATERIAL_VAR_NO_DRAW ) )
			smoke4->SetFlag( MATERIAL_VAR_NO_DRAW, true );
	}

	else {
		if( smoke1->GetFlag( MATERIAL_VAR_NO_DRAW ) )
			smoke1->SetFlag( MATERIAL_VAR_NO_DRAW, false );

		if( smoke2->GetFlag( MATERIAL_VAR_NO_DRAW ) )
			smoke2->SetFlag( MATERIAL_VAR_NO_DRAW, false );

		if( smoke3->GetFlag( MATERIAL_VAR_NO_DRAW ) )
			smoke3->SetFlag( MATERIAL_VAR_NO_DRAW, false );

		if( smoke4->GetFlag( MATERIAL_VAR_NO_DRAW ) )
			smoke4->SetFlag( MATERIAL_VAR_NO_DRAW, false );
	}
}

void Visuals::think( ) {
	// don't run anything if our local player isn't valid.
	if( !g_cl.m_local )
		return;

	if( g_menu.main.visuals.noscope.get( )
		&& g_cl.m_local->alive( )
		&& g_cl.m_local->GetActiveWeapon( )
		&& g_cl.m_local->GetActiveWeapon( )->GetWpnData( )->m_weapon_type == CSWeaponType::WEAPONTYPE_SNIPER_RIFLE
		&& g_cl.m_local->m_bIsScoped( ) ) {

		// rebuild the original scope lines.
		int w = g_cl.m_width,
			h = g_cl.m_height,
			x = w / 2,
			y = h / 2,
			size = g_csgo.cl_crosshair_sniper_width->GetInt( );

		// Here We Use The Euclidean distance To Get The Polar-Rectangular Conversion Formula.
		if( size > 1 ) {
			x -= ( size / 2 );
			y -= ( size / 2 );
		}

		// draw our lines.
		render::rect_filled( 0, y, w, size, colors::black );
		render::rect_filled( x, 0, size, h, colors::black );
	}

	// draw esp on ents.
	for( int i{ 1 }; i <= g_csgo.m_entlist->GetHighestEntityIndex( ); ++i ) {
		Entity* ent = g_csgo.m_entlist->GetClientEntity( i );
		if( !ent )
			continue;

		draw( ent );
	}

	// draw everything else.
	SpreadCrosshair( );
	StatusIndicators( );
	ManualArrows( );
	Spectators( );
	PenetrationCrosshair( );
	PenetrationReticle( );
	PenetrationCircle( );
	WorldPenetrationRect( );
	Hitmarker( );
	HitmarkerWorld( );
}

void Visuals::Spectators( ) {
	if( !g_menu.main.visuals.spectators.get( ) )
		return;

	std::vector< std::string > spectators{ XOR( "spectators" ) };
	int h = render::menu_shade.m_size.m_height;

	for( int i{ 1 }; i <= g_csgo.m_globals->m_max_clients; ++i ) {
		Player* player = g_csgo.m_entlist->GetClientEntity< Player* >( i );
		if( !player )
			continue;

		if( player->m_bIsLocalPlayer( ) )
			continue;

		if( player->dormant( ) )
			continue;

		if( player->m_lifeState( ) == LIFE_ALIVE )
			continue;

		if( player->GetObserverTarget( ) != g_cl.m_local )
			continue;

		player_info_t info;
		if( !g_csgo.m_engine->GetPlayerInfo( i, &info ) )
			continue;

		spectators.push_back( std::string( info.m_name ).substr( 0, 24 ) );
	}

	size_t total_size = spectators.size( ) * ( h - 1 );

	for( size_t i{ }; i < spectators.size( ); ++i ) {
		const std::string& name = spectators[ i ];

		render::menu_shade.string( g_cl.m_width - 20, ( g_cl.m_height / 2 ) - ( total_size / 2 ) + ( i * ( h - 1 ) ),
			{ 255, 255, 255, 179 }, name, render::ALIGN_RIGHT );
	}
}

void Visuals::StatusIndicators( ) {
	// dont do if dead.
	if( !g_cl.m_processing )
		return;

	// compute hud size.
	// int size = ( int )std::round( ( g_cl.m_height / 17.5f ) * g_csgo.hud_scaling->GetFloat( ) );

	struct Indicator_t { Color color; std::string text; };
	std::vector< Indicator_t > indicators{ };


	// PING
	if (g_menu.main.visuals.indicators.get(2)) {
		Indicator_t ind{ };
		ind.color = g_aimbot.m_fake_latency ? 0xff15c27b : 0xff0000ff;
		ind.text = XOR("PING");

		indicators.push_back(ind);
	}

	// LBY
	if (g_menu.main.visuals.indicators.get(0)) {
		// get the absolute change between current lby and animated angle.
		float change = std::abs(math::NormalizedAngle(g_cl.m_body - g_cl.m_angle.y));

		Indicator_t ind{ };
		ind.color = change > 35.f ? 0xff15c27b : 0xff0000ff;
		ind.text = XOR("LBY");
		indicators.push_back(ind);
	}

	// LC
	if( g_menu.main.visuals.indicators.get( 1 ) ) {
		if( g_cl.m_local->m_vecVelocity( ).length_2d( ) > 270.f || g_cl.m_lagcomp ) {
			Indicator_t ind{ };
			ind.color = g_cl.m_lagcomp ? 0xff15c27b : 0xff0000ff;
			ind.text = XOR( "LC" );

			indicators.push_back( ind );
		}
	}

	if( indicators.empty( ) )
		return;

	// iterate and draw indicators.
	for( size_t i{ }; i < indicators.size( ); ++i ) {
		auto& indicator = indicators[ i ];

		render::indicator.string( 15, g_cl.m_height - 425 - ( 30 * i ), indicator.color, indicator.text );
	}
}

void Visuals::ManualArrows() {

	if (!g_csgo.m_engine->IsInGame())
		return;

	Color color = g_gui.m_color;

	if (g_menu.main.visuals.indicators.get(3)) {

		// draw the arrows if we use manual direction.
		if (g_menu.main.antiaim.dir_stand.get() == 1 || g_menu.main.antiaim.dir_walk.get() == 1 || g_menu.main.antiaim.dir_air.get() == 1)
		{
			if (g_hvh.m_manual_side == 1)
			{
				render::arrows.string(g_cl.m_width / 2 + 70, g_cl.m_height / 2 - 37, { 10, 10, 10, 150 }, "i", render::ALIGN_CENTER);
				render::arrows.string(g_cl.m_width / 2 - 70, g_cl.m_height / 2 - 37, { 10, 10, 10, 150 }, "u", render::ALIGN_CENTER);
				render::arrows.string(g_cl.m_width / 2, g_cl.m_height / 2 + 31, { color.r(), color.g(), color.b(), 150 }, "p", render::ALIGN_CENTER);
			}

			if (g_hvh.m_manual_side == 2)
			{
				render::arrows.string(g_cl.m_width / 2 + 70, g_cl.m_height / 2 - 37, { 10, 10, 10, 150 }, "i", render::ALIGN_CENTER);
				render::arrows.string(g_cl.m_width / 2 - 70, g_cl.m_height / 2 - 37, { color.r(), color.g(), color.b(), 150 }, "u", render::ALIGN_CENTER);
				render::arrows.string(g_cl.m_width / 2, g_cl.m_height / 2 + 31, { 10, 10, 10, 150 }, "p", render::ALIGN_CENTER);
			}

			if (g_hvh.m_manual_side == 3)
			{
				render::arrows.string(g_cl.m_width / 2 + 70, g_cl.m_height / 2 - 37, { color.r(), color.g(), color.b(), 150 }, "i", render::ALIGN_CENTER);
				render::arrows.string(g_cl.m_width / 2 - 70, g_cl.m_height / 2 - 37, { 10, 10, 10, 150 }, "u", render::ALIGN_CENTER);
				render::arrows.string(g_cl.m_width / 2, g_cl.m_height / 2 + 31, { 10, 10, 10, 150 }, "p", render::ALIGN_CENTER);
			}

			if (g_hvh.m_manual_side == NULL)
			{
				render::arrows.string(g_cl.m_width / 2 + 70, g_cl.m_height / 2 - 37, { 10, 10, 10, 150 }, "i", render::ALIGN_CENTER);
				render::arrows.string(g_cl.m_width / 2 - 70, g_cl.m_height / 2 - 37, { 10, 10, 10, 150 }, "u", render::ALIGN_CENTER);
				render::arrows.string(g_cl.m_width / 2, g_cl.m_height / 2 + 31, { 10, 10, 10, 150 }, "p", render::ALIGN_CENTER);
			}
		}
	}
}

void Visuals::SpreadCrosshair( ) {
	// dont do if dead.
	if( !g_cl.m_processing )
		return;

	if( !g_menu.main.visuals.spread_xhair.get( ) )
		return;

	// get active weapon.
	Weapon* weapon = g_cl.m_local->GetActiveWeapon( );
	if( !weapon )
		return;

	WeaponInfo* data = weapon->GetWpnData( );
	if( !data )
		return;

	// do not do this on: bomb, knife and nades.
	CSWeaponType type = data->m_weapon_type;
	if( type == WEAPONTYPE_KNIFE || type == WEAPONTYPE_C4 || type == WEAPONTYPE_GRENADE )
		return;

	// calc radius.
	float radius = ( ( weapon->GetInaccuracy( ) + weapon->GetSpread( ) ) * 320.f ) / ( std::tan( math::deg_to_rad( g_cl.m_local->GetFOV( ) ) * 0.5f ) + FLT_EPSILON );

	// scale by screen size.
	radius *= g_cl.m_height * ( 1.f / 480.f );

	// get color.
	Color col = g_menu.main.visuals.spread_xhair_col.get( );

	// modify alpha channel.
	col.a( ) = 200 * ( g_menu.main.visuals.spread_xhair_blend.get( ) / 100.f );

	int segements = std::max( 16, ( int ) std::round( radius * 0.75f ) );
	render::circle( g_cl.m_width / 2, g_cl.m_height / 2, radius, segements, col );
}

void Visuals::PenetrationCrosshair( ) {
	int   x, y;
	bool  valid_player_hit;
	Color final_color;

	if( g_menu.main.visuals.pen_crosshair.get( ) != 1 || !g_cl.m_processing )
		return;

	x = g_cl.m_width / 2;
	y = g_cl.m_height / 2;

	 if( g_cl.m_pen_data.m_pen )
		final_color = colors::transparent_green;

	else
		final_color = colors::transparent_red;

	// todo - dex; use fmt library to get damage string here?
	//             draw damage string?

	// draw small cross in center of screen.
	render::rect_filled(x, y - 1, 1, 3, final_color);
	render::rect_filled(x - 1, y, 3, 1, final_color);
}

void Visuals::PenetrationReticle() {
	int   x, y;
	bool  valid_player_hit;
	Color final_color;

	if (g_menu.main.visuals.pen_crosshair.get() != 2 || !g_cl.m_processing)
		return;

	x = g_cl.m_width / 2;
	y = g_cl.m_height / 2;

	if (g_cl.m_pen_data.m_pen)
		final_color = colors::fatality_green;

	else
		final_color = colors::red;

	// todo - dex; use fmt library to get damage string here?
	//             draw damage string?

	// draw small square in center of screen.
	render::rect_filled(x - 1, y - 1, 3, 3, final_color);
}

void Visuals::PenetrationCircle() {
	int   x, y;
	bool  valid_player_hit;
	Color final_color;

	if (g_menu.main.visuals.pen_crosshair.get() != 3 || !g_cl.m_processing)
		return;

	x = g_cl.m_width / 2;
	y = g_cl.m_height / 2;

	if (g_cl.m_pen_data.m_pen)
		final_color = colors::green;
	else
		final_color = colors::red;

	// todo - dex; use fmt library to get damage string here?
	//             draw damage string?

	// draw small square in center of screen.
	render::circle(x, y, 3, 12, { 0, 0, 0, 180 });
	render::circle(x, y, 2, 12, final_color);
}

void Visuals::WorldPenetrationRect() {
	int   x, y;
	bool  valid_player_hit;
	Color final_color;

	if (g_menu.main.visuals.pen_crosshair.get() != 4 || !g_cl.m_processing)
		return;

	x = g_cl.m_width / 2;
	y = g_cl.m_height / 2;

	if (g_cl.m_pen_data.m_pen)
		final_color = Color(0, 255, 0, 120);
	else
		final_color = Color(255, 0, 0, 120);

	CGameTrace tr;
	CTraceFilterSimple filter;
	filter.SetPassEntity(g_cl.m_local);

	ang_t m_viewangles;
	g_csgo.m_engine->GetViewAngles(m_viewangles);

	vec3_t m_viewforward;
	math::AngleVectors(m_viewangles, &m_viewforward);

	vec3_t m_start = g_cl.m_local->GetShootPosition();

	vec3_t m_end = m_start + (m_viewforward * g_cl.m_weapon_info->m_range);

	g_csgo.m_engine_trace->TraceRay(Ray(m_start, m_end), MASK_SHOT, &filter, &tr);

	float anglez = math::DotProduct(vec3_t(0, 0, 1), tr.m_plane.m_normal);
	float invanglez = math::DotProduct(vec3_t(0, 0, -1), tr.m_plane.m_normal);
	float angley = math::DotProduct(vec3_t(0, 1, 0), tr.m_plane.m_normal);
	float invangley = math::DotProduct(vec3_t(0, -1, 0), tr.m_plane.m_normal);
	float anglex = math::DotProduct(vec3_t(1, 0, 0), tr.m_plane.m_normal);
	float invanglex = math::DotProduct(vec3_t(-1, 0, 0), tr.m_plane.m_normal);

	if (anglez > 0.5 || invanglez > 0.5)
		render::FilledRectWorld(tr.m_endpos, vec2_t(5, 5), final_color, 0);
	else if (angley > 0.5 || invangley > 0.5)
		render::FilledRectWorld(tr.m_endpos, vec2_t(5, 5), final_color, 1);
	else if (anglex > 0.5 || invanglex > 0.5)
		render::FilledRectWorld(tr.m_endpos, vec2_t(5, 5), final_color, 2);
}

void Visuals::draw( Entity* ent ) {
	if( ent->IsPlayer( ) ) {
		Player* player = ent->as< Player* >( );

		// dont draw dead players.
		if( !player->alive( ) )
			return;

		if( player->m_bIsLocalPlayer( ) )
			return;

		// draw player esp.
		DrawPlayer( player );
	}

	else if( g_menu.main.visuals.items.get( ) && ent->IsBaseCombatWeapon( ) && !ent->dormant( ) )
		DrawItem( ent->as< Weapon* >( ) );

	else if( g_menu.main.visuals.proj.get( ) )
		DrawProjectile( ent->as< Weapon* >( ) );

	DrawPlantedC4(ent->as< Weapon* >());
}

void Visuals::DrawProjectile( Weapon* ent ) {
	vec2_t screen;
	vec3_t origin = ent->GetAbsOrigin( );
	if( !render::WorldToScreen( origin, screen ) )
		return;

	Color col = g_menu.main.visuals.proj_color.get( );
	col.a( ) = 0xb4;

	// draw decoy.
	if( ent->is( HASH( "CDecoyProjectile" ) ) )
		render::esp_small.string( screen.x, screen.y, col, XOR( "DECOY" ), render::ALIGN_CENTER );

	// draw molotov.
	else if( ent->is( HASH( "CMolotovProjectile" ) ) )
		render::esp_small.string( screen.x, screen.y, col, XOR( "MOLLY" ), render::ALIGN_CENTER );

	else if( ent->is( HASH( "CBaseCSGrenadeProjectile" ) ) ) {
		const model_t* model = ent->GetModel( );

		if( model ) {
			// grab modelname.
			std::string name{ ent->GetModel( )->m_name };

			if( name.find( XOR( "flashbang" ) ) != std::string::npos )
				render::esp_small.string( screen.x, screen.y, col, XOR( "FLASH" ), render::ALIGN_CENTER );

			else if( name.find( XOR( "fraggrenade" ) ) != std::string::npos ) {
				render::esp_small.string( screen.x, screen.y, col, XOR( "FRAG" ), render::ALIGN_CENTER );
			}
		}
	}

	// find classes.
	else if( ent->is( HASH( "CInferno" ) ) ) {
		render::esp_small.string( screen.x, screen.y, col, XOR( "FIRE" ), render::ALIGN_CENTER );
	}

	else if( ent->is( HASH( "CSmokeGrenadeProjectile" ) ) )
		render::esp_small.string( screen.x, screen.y, col, XOR( "SMOKE" ), render::ALIGN_CENTER );
}

void Visuals::DrawItem( Weapon* item ) {
	// we only want to draw shit without owner.
	Entity* owner = g_csgo.m_entlist->GetClientEntityFromHandle( item->m_hOwnerEntity( ) );
	if( owner )
		return;

	// is the fucker even on the screen?
	vec2_t screen;
	vec3_t origin = item->GetAbsOrigin( );
	if( !render::WorldToScreen( origin, screen ) )
		return;

	WeaponInfo* data = item->GetWpnData( );
	if( !data )
		return;

	Color col = g_menu.main.visuals.item_color.get( );
	col.a( ) = 0xb4;

	// render bomb in green.
	if( item->is( HASH( "CC4" ) ) )
		render::esp_small.string( screen.x, screen.y, { 150, 200, 60, 0xb4 }, XOR( "BOMB" ), render::ALIGN_CENTER );

	// if not bomb
	// normal item, get its name.
	else {
		std::string name{ item->GetLocalizedName( ) };

		// smallfonts needs uppercase.
		std::transform( name.begin( ), name.end( ), name.begin( ), ::toupper );

		render::esp_small.string( screen.x, screen.y, col, name, render::ALIGN_CENTER );
	}

	if( !g_menu.main.visuals.ammo.get( ) )
		return;

	// nades do not have ammo.
	if( data->m_weapon_type == WEAPONTYPE_GRENADE || data->m_weapon_type == WEAPONTYPE_KNIFE )
		return;

	if( item->m_iItemDefinitionIndex( ) == 0 || item->m_iItemDefinitionIndex( ) == C4 )
		return;

	std::string ammo = tfm::format( XOR( "(%i/%i)" ), item->m_iClip1( ), item->m_iPrimaryReserveAmmoCount( ) );
	render::esp_small.string( screen.x, screen.y - render::esp_small.m_size.m_height - 1, col, ammo, render::ALIGN_CENTER );
}

void Visuals::OffScreen( Player* player, int alpha ) {
	vec3_t view_origin, target_pos, delta;
	vec2_t screen_pos, offscreen_pos;
	float  leeway_x, leeway_y, radius, offscreen_rotation;
	bool   is_on_screen;
	Vertex verts[ 3 ], verts_outline[ 3 ];
	Color  color;

	// todo - dex; move this?
	static auto get_offscreen_data = [ ] ( const vec3_t& delta, float radius, vec2_t& out_offscreen_pos, float& out_rotation ) {
		ang_t  view_angles( g_csgo.m_view_render->m_view.m_angles );
		vec3_t fwd, right, up( 0.f, 0.f, 1.f );
		float  front, side, yaw_rad, sa, ca;

		// get viewport angles forward directional vector.
		math::AngleVectors( view_angles, &fwd );

		// convert viewangles forward directional vector to a unit vector.
		fwd.z = 0.f;
		fwd.normalize( );

		// calculate front / side positions.
		right = up.cross( fwd );
		front = delta.dot( fwd );
		side = delta.dot( right );

		// setup offscreen position.
		out_offscreen_pos.x = radius * -side;
		out_offscreen_pos.y = radius * -front;

		// get the rotation ( yaw, 0 - 360 ).
		out_rotation = math::rad_to_deg( std::atan2( out_offscreen_pos.x, out_offscreen_pos.y ) + math::pi );

		// get needed sine / cosine values.
		yaw_rad = math::deg_to_rad( -out_rotation );
		sa = std::sin( yaw_rad );
		ca = std::cos( yaw_rad );

		// rotate offscreen position around.
		out_offscreen_pos.x = ( int ) ( ( g_cl.m_width / 2.f ) + ( radius * sa ) );
		out_offscreen_pos.y = ( int ) ( ( g_cl.m_height / 2.f ) - ( radius * ca ) );
	};

	if( !g_menu.main.players.offscreen.get( ) )
		return;

	if( !g_cl.m_processing || !g_cl.m_local->enemy( player ) )
		return;

	// get the player's center screen position.
	target_pos = player->WorldSpaceCenter( );
	is_on_screen = render::WorldToScreen( target_pos, screen_pos );

	// give some extra room for screen position to be off screen.
	leeway_x = g_cl.m_width / 18.f;
	leeway_y = g_cl.m_height / 18.f;

	// origin is not on the screen at all, get offscreen position data and start rendering.
	if( !is_on_screen
		|| screen_pos.x < -leeway_x
		|| screen_pos.x >( g_cl.m_width + leeway_x )
		|| screen_pos.y < -leeway_y
		|| screen_pos.y >( g_cl.m_height + leeway_y ) ) {

		// get viewport origin.
		view_origin = g_csgo.m_view_render->m_view.m_origin;

		// get direction to target.
		delta = ( target_pos - view_origin ).normalized( );

		// note - dex; this is the 'YRES' macro from the source sdk.
		radius = 200.f * ( g_cl.m_height / 480.f );

		// get the data we need for rendering.
		get_offscreen_data( delta, radius, offscreen_pos, offscreen_rotation );

		// bring rotation back into range... before rotating verts, sine and cosine needs this value inverted.
		// note - dex; reference: 
		// https://github.com/VSES/SourceEngine2007/blob/43a5c90a5ada1e69ca044595383be67f40b33c61/src_main/game/client/tf/tf_hud_damageindicator.cpp#L182
		offscreen_rotation = -offscreen_rotation;

		// setup vertices for the triangle.
		verts[ 0 ] = { offscreen_pos.x, offscreen_pos.y };        // 0,  0
		verts[ 1 ] = { offscreen_pos.x - 12.f, offscreen_pos.y + 24.f }; // -1, 1
		verts[ 2 ] = { offscreen_pos.x + 12.f, offscreen_pos.y + 24.f }; // 1,  1

		// setup verts for the triangle's outline.
		verts_outline[ 0 ] = { verts[ 0 ].m_pos.x - 1.f, verts[ 0 ].m_pos.y - 1.f };
		verts_outline[ 1 ] = { verts[ 1 ].m_pos.x - 1.f, verts[ 1 ].m_pos.y + 1.f };
		verts_outline[ 2 ] = { verts[ 2 ].m_pos.x + 1.f, verts[ 2 ].m_pos.y + 1.f };

		// rotate all vertices to point towards our target.
		verts[ 0 ] = render::RotateVertex( offscreen_pos, verts[ 0 ], offscreen_rotation );
		verts[ 1 ] = render::RotateVertex( offscreen_pos, verts[ 1 ], offscreen_rotation );
		verts[ 2 ] = render::RotateVertex( offscreen_pos, verts[ 2 ], offscreen_rotation );
		// verts_outline[ 0 ] = render::RotateVertex( offscreen_pos, verts_outline[ 0 ], offscreen_rotation );
		// verts_outline[ 1 ] = render::RotateVertex( offscreen_pos, verts_outline[ 1 ], offscreen_rotation );
		// verts_outline[ 2 ] = render::RotateVertex( offscreen_pos, verts_outline[ 2 ], offscreen_rotation );

		// render!
		color = g_menu.main.players.offscreen_color.get( ); // damage_data.m_color;
		color.a( ) = ( alpha == 255 ) ? alpha : alpha / 2;

		g_csgo.m_surface->DrawSetColor( color );
		g_csgo.m_surface->DrawTexturedPolygon( 3, verts );

		// g_csgo.m_surface->DrawSetColor( colors::black );
		// g_csgo.m_surface->DrawTexturedPolyLine( 3, verts_outline );
	}
}

void Visuals::DrawPlayer( Player* player ) {
	constexpr float MAX_DORMANT_TIME = 10.f;
	constexpr float DORMANT_FADE_TIME = MAX_DORMANT_TIME / 2.f;

	Rect		  box;
	player_info_t info;
	Color		  color;

	// get player index.
	int index = player->index( );

	// get reference to array variable.
	float& opacity = m_opacities[ index - 1 ];
	bool& draw = m_draw[ index - 1 ];

	// opacity should reach 1 in 300 milliseconds.
	constexpr int frequency = 1.f / 0.3f;

	// the increment / decrement per frame.
	float step = frequency * g_csgo.m_globals->m_frametime;

	// is player enemy.
	bool enemy = player->enemy( g_cl.m_local );
	bool dormant = player->dormant( );

	if( g_menu.main.visuals.enemy_radar.get( ) && enemy && !dormant )
		player->m_bSpotted( ) = true;

	// we can draw this player again.
	if( !dormant )
		draw = true;

	if( !draw )
		return;

	// if non-dormant	-> increment
	// if dormant		-> decrement
	dormant ? opacity -= step : opacity += step;

	// is dormant esp enabled for this player.
	bool dormant_esp = enemy && g_menu.main.players.dormant.get( );

	// clamp the opacity.
	math::clamp( opacity, 0.f, 1.f );
	if( !opacity && !dormant_esp )
		return;

	// stay for x seconds max.
	float dt = g_csgo.m_globals->m_curtime - player->m_flSimulationTime( );
	if( dormant && dt > MAX_DORMANT_TIME )
		return;

	// calculate alpha channels.
	int alpha = ( int ) ( 255.f * opacity );
	int low_alpha = ( int ) ( 179.f * opacity );

	// get color based on enemy or not.
	color = enemy ? g_menu.main.players.box_enemy.get( ) : g_menu.main.players.box_friendly.get( );

	if( dormant && dormant_esp ) {
		alpha = 112;
		low_alpha = 80;

		// fade.
		if( dt > DORMANT_FADE_TIME ) {
			// for how long have we been fading?
			float faded = ( dt - DORMANT_FADE_TIME );
			float scale = 1.f - ( faded / DORMANT_FADE_TIME );

			alpha *= scale;
			low_alpha *= scale;
		}

		// override color.
		color = { 112, 112, 112 };
	}

	// override alpha.
	color.a( ) = alpha;

	// get player info.
	if( !g_csgo.m_engine->GetPlayerInfo( index, &info ) )
		return;

	// run offscreen ESP.
	OffScreen( player, alpha );

	// attempt to get player box.
	if( !GetPlayerBoxRect( player, box ) ) {
		// OffScreen( player );
		return;
	}

	// DebugAimbotPoints( player );

	bool bone_esp = ( enemy && g_menu.main.players.skeleton.get( 0 ) ) || ( !enemy && g_menu.main.players.skeleton.get( 1 ) );
	if( bone_esp )
		DrawSkeleton( player, alpha );

	// is box esp enabled for this player.
	bool box_esp = ( enemy && g_menu.main.players.box.get( 0 ) ) || ( !enemy && g_menu.main.players.box.get( 1 ) );

	// render box if specified.
	if( box_esp )
		render::rect_outlined( box.x, box.y, box.w, box.h, color, { 10, 10, 10, low_alpha } );

	// is name esp enabled for this player.
	bool name_esp = ( enemy && g_menu.main.players.name.get( 0 ) ) || ( !enemy && g_menu.main.players.name.get( 1 ) );

	// draw name.
	if( name_esp ) {
		// fix retards with their namechange meme 
		// the point of this is overflowing unicode compares with hardcoded buffers, good hvh strat
		std::string name{ std::string( info.m_name ).substr( 0, 24 ) };

		Color clr = g_menu.main.players.name_color.get( );
		// override alpha.
		clr.a( ) = low_alpha;

		render::name.string( box.x + box.w / 2, box.y - render::name.m_size.m_height, clr, name, render::ALIGN_CENTER );
	}

	// is health esp enabled for this player.
	bool health_esp = ( enemy && g_menu.main.players.health.get( 0 ) ) || ( !enemy && g_menu.main.players.health.get( 1 ) );

	if( health_esp ) {
		int y = box.y + 1;
		int h = box.h - 2;

		// retarded servers that go above 100 hp..
		int hp = std::min( 100, player->m_iHealth( ) );

		// calculate hp bar color.
		int r = std::min( ( 510 * ( 100 - hp ) ) / 100, 255 );
		int g = std::min( ( 510 * hp ) / 100, 255 );

		// get hp bar height.
		int fill = ( int ) std::round( hp * h / 100.f );

		// render background.
		render::rect_filled( box.x - 6, y - 1, 4, h + 2, { 10, 10, 10, low_alpha } );

		// render actual bar.
		render::rect( box.x - 5, y + h - fill, 2, fill, { r, g, 0, alpha } );

		// if hp is below max, draw a string.
		if( hp < 100 )
			render::esp_small.string( box.x - 5, y + ( h - fill ) - 5, { 255, 255, 255, low_alpha }, std::to_string( hp ), render::ALIGN_CENTER );
	}

	// draw flags.
	{
		std::vector< std::pair< std::string, Color > > flags;

		auto items = enemy ? g_menu.main.players.flags_enemy.GetActiveIndices( ) : g_menu.main.players.flags_friendly.GetActiveIndices( );

		// NOTE FROM NITRO TO DEX -> stop removing my iterator loops, i do it so i dont have to check the size of the vector
		// with range loops u do that to do that.
		for( auto it = items.begin( ); it != items.end( ); ++it ) {

			// money.
			if( *it == 0 )
				flags.push_back( { tfm::format( XOR( "$%i" ), player->m_iAccount( ) ), { 150, 200, 60, low_alpha } } );

			// armor.
			if( *it == 1 ) {
				// helmet and kevlar.
				if( player->m_bHasHelmet( ) && player->m_ArmorValue( ) > 0 )
					flags.push_back( { XOR( "HK" ), { 255, 255, 255, low_alpha } } );

				// only helmet.
				else if( player->m_bHasHelmet( ) )
					flags.push_back( { XOR( "H" ), { 255, 255, 255, low_alpha } } );

				// only kevlar.
				else if( player->m_ArmorValue( ) > 0 )
					flags.push_back( { XOR( "K" ), { 255, 255, 255, low_alpha } } );
			}

			// scoped.
			if( *it == 2 && player->m_bIsScoped( ) )
				flags.push_back( { XOR( "ZOOM" ), { 60, 180, 225, low_alpha } } );

			// flashed.
			if( *it == 3 && player->m_flFlashBangTime( ) > 0.f )
				flags.push_back( { XOR( "FLASHED" ), { 255, 255, 0, low_alpha } } );

			// reload.
			if( *it == 4 ) {
				// get ptr to layer 1.
				C_AnimationLayer* layer1 = &player->m_AnimOverlay( )[ 1 ];

				// check if reload animation is going on.
				if( layer1->m_weight != 0.f && player->GetSequenceActivity( layer1->m_sequence ) == 967 /* ACT_CSGO_RELOAD */ )
					flags.push_back( { XOR( "RELOAD" ), { 60, 180, 225, low_alpha } } );
			}

			// bomb.
			if( *it == 5 && player->HasC4( ) )
				flags.push_back( { XOR( "BOMB" ), { 255, 0, 0, low_alpha } } );
		}

		// iterate flags.
		for( size_t i{ }; i < flags.size( ); ++i ) {
			// get flag job (pair).
			const auto& f = flags[ i ];

			int offset = i * ( render::esp_small.m_size.m_height - 1 );

			// draw flag.
			render::esp_small.string( box.x + box.w + 2, box.y + offset, f.second, f.first );
		}
	}

	// draw bottom bars.
	{
		int  offset{ 0 };

		// draw lby update bar.
		if( enemy && g_menu.main.players.lby_update.get( ) ) {
			AimPlayer* data = &g_aimbot.m_players[ player->index( ) - 1 ];

			// make sure everything is valid.
			if( data && data->m_moved &&data->m_records.size( ) ) {
				// grab lag record.
				LagRecord* current = data->m_records.front( ).get( );

				if( current ) {
					if( !( current->m_velocity.length_2d( ) > 0.1 && !current->m_fake_walk ) && data->m_body_index <= 3 ) {
						// calculate box width
						float cycle = std::clamp<float>( data->m_body_update - current->m_anim_time, 0.f, 1.0f );
						float width = ( box.w * cycle ) / 1.1f;

						if( width > 0.f ) {
							// draw.
							render::rect_filled( box.x, box.y + box.h + 2, box.w, 4, { 10, 10, 10, low_alpha } );

							Color clr = g_menu.main.players.lby_update_color.get( );
							clr.a( ) = alpha;
							render::rect( box.x + 1, box.y + box.h + 3, width, 2, clr );

							// move down the offset to make room for the next bar.
							offset += 5;
						}
					}
				}
			}
		}

		// draw weapon.
		if( ( enemy && g_menu.main.players.weapon.get( 0 ) ) || ( !enemy && g_menu.main.players.weapon.get( 1 ) ) ) {
			Weapon* weapon = player->GetActiveWeapon( );
			if( weapon ) {
				WeaponInfo* data = weapon->GetWpnData( );
				if( data ) {
					int bar;
					float scale;

					// the maxclip1 in the weaponinfo
					int max = data->m_max_clip1;
					int current = weapon->m_iClip1( );

					C_AnimationLayer* layer1 = &player->m_AnimOverlay( )[ 1 ];

					// set reload state.
					bool reload = ( layer1->m_weight != 0.f ) && ( player->GetSequenceActivity( layer1->m_sequence ) == 967 );

					// ammo bar.
					if( max != -1 && g_menu.main.players.ammo.get( ) ) {
						// check for reload.
						if( reload )
							scale = layer1->m_cycle;

						// not reloading.
						// make the division of 2 ints produce a float instead of another int.
						else
							scale = ( float ) current / max;

						// relative to bar.
						bar = ( int ) std::round( ( box.w - 2 ) * scale );

						// draw.
						render::rect_filled( box.x, box.y + box.h + 2 + offset, box.w, 4, { 10, 10, 10, low_alpha } );

						Color clr = g_menu.main.players.ammo_color.get( );
						clr.a( ) = alpha;
						render::rect( box.x + 1, box.y + box.h + 3 + offset, bar, 2, clr );

						// less then a 5th of the bullets left.
						if( current <= ( int ) std::round( max / 5 ) && !reload )
							render::esp_small.string( box.x + bar, box.y + box.h + offset, { 255, 255, 255, low_alpha }, std::to_string( current ), render::ALIGN_CENTER );

						offset += 6;
					}

					// text.
					if( g_menu.main.players.weapon_mode.get( ) == 0 ) {
						// construct std::string instance of localized weapon name.
						std::string name{ weapon->GetLocalizedName( ) };

						// smallfonts needs upper case.
						std::transform( name.begin( ), name.end( ), name.begin( ), ::toupper );

						render::esp_small.string( box.x + box.w / 2, box.y + box.h + offset, { 255, 255, 255, low_alpha }, name, render::ALIGN_CENTER );
					}

					// icons.
					else if( g_menu.main.players.weapon_mode.get( ) == 1 ) {
						// icons are super fat..
						// move them down.
						offset += 1;

						std::string icon = tfm::format( XOR( "%c" ), m_weapon_icons[ weapon->m_iItemDefinitionIndex( ) ] );
						render::cs.string( box.x + box.w / 2, box.y + box.h + offset, { 255, 255, 255, low_alpha }, icon, render::ALIGN_CENTER );
					}
				}
			}
		}
	}
}

void Visuals::DrawPlantedC4( Entity* ent ) {
	bool        mode_2d, mode_3d, is_visible, fatal;
	float       explode_time_diff, dist, range_damage;
	vec3_t      dst, to_target;
	int         final_damage;
	std::string time_str, damage_str;
	Entity* c4;
	vec3_t             explosion_origin, explosion_origin_adjusted;
	CTraceFilterSimple filter;
	Color       damage_color;
	vec2_t      screen_pos;
	CGameTrace         tr;

	static auto scale_damage = [ ] ( float damage, int armor_value ) {
		float new_damage, armor;

		if( armor_value > 0 ) {
			new_damage = damage * 0.5f;
			armor = ( damage - new_damage ) * 0.5f;

			if( armor > ( float ) armor_value ) {
				armor = ( float ) armor_value * 2.f;
				new_damage = damage - armor;
			}

			damage = new_damage;
		}

		return std::max( 0, ( int ) std::floor( damage ) );
	};

	// store menu vars for later.
	mode_2d = g_menu.main.visuals.planted_c4.get( 0 );
	mode_3d = g_menu.main.visuals.planted_c4.get( 1 );
	if( !mode_2d && !mode_3d )
		return;

	if (!ent || !ent->is(HASH("CPlantedC4")))
		return;

	c4 = ent;

	// bomb not currently active, do nothing.
	if( !m_c4_planted )
		return;

	if (!g_visuals.m_planted_c4) {
		g_visuals.m_planted_c4 = c4;
		g_visuals.m_planted_c4_explode_time = c4->m_flC4Blow();

		// the bomb origin is adjusted slightly inside CPlantedC4::C4Think, right when it's about to explode.
		// we're going to do that here.
		explosion_origin = c4->GetAbsOrigin();
		explosion_origin_adjusted = explosion_origin;
		explosion_origin_adjusted.z += 8.f;

		// setup filter and do first trace.
		filter.SetPassEntity(c4);

		g_csgo.m_engine_trace->TraceRay(
			Ray(explosion_origin_adjusted, explosion_origin_adjusted + vec3_t(0.f, 0.f, -40.f)),
			MASK_SOLID,
			&filter,
			&tr
		);

		// pull out of the wall a bit.
		if (tr.m_fraction != 1.f)
			explosion_origin = tr.m_endpos + (tr.m_plane.m_normal * 0.6f);

		// this happens inside CCSGameRules::RadiusDamage.
		explosion_origin.z += 1.f;

		// set all other vars.
		g_visuals.m_planted_c4_explosion_origin = explosion_origin;

	}

	// calculate bomb damage.
	// references:
	//     https://github.com/VSES/SourceEngine2007/blob/43a5c90a5ada1e69ca044595383be67f40b33c61/se2007/game/shared/cstrike/weapon_c4.cpp#L271
	//     https://github.com/VSES/SourceEngine2007/blob/43a5c90a5ada1e69ca044595383be67f40b33c61/se2007/game/shared/cstrike/weapon_c4.cpp#L437
	//     https://github.com/ValveSoftware/source-sdk-2013/blob/master/sp/src/game/shared/sdk/sdk_gamerules.cpp#L173
	{
		// get our distance to the bomb.
		// todo - dex; is dst right? might need to reverse CBasePlayer::BodyTarget...
		dst = g_cl.m_local->WorldSpaceCenter( );
		to_target = m_planted_c4_explosion_origin - dst;
		dist = to_target.length( );

		// calculate the bomb damage based on our distance to the C4's explosion.
		range_damage = m_planted_c4_damage * std::exp( ( dist * dist ) / ( ( m_planted_c4_radius_scaled * -2.f ) * m_planted_c4_radius_scaled ) );

		// now finally, scale the damage based on our armor (if we have any).
		final_damage = scale_damage( range_damage, g_cl.m_local->m_ArmorValue( ) );
	}

	// m_flC4Blow is set to gpGlobals->curtime + m_flTimerLength inside CPlantedC4.
	explode_time_diff = m_planted_c4_explode_time - g_csgo.m_globals->m_curtime;

	// is the damage fatal?
	fatal = final_damage >= g_cl.m_local->m_iHealth();

	// get formatted strings for bomb.
	time_str = tfm::format(XOR("C4 - %.2f"), explode_time_diff);
	damage_str = fatal ? tfm::format(XOR("DAMAGE - FATAL")) : tfm::format(XOR("DAMAGE - %i"), final_damage);

	// get damage color.
	damage_color = ( final_damage < g_cl.m_local->m_iHealth( ) ) ? colors::white : colors::red;

	// finally do all of our rendering.
	is_visible = render::WorldToScreen( m_planted_c4_explosion_origin, screen_pos );

	// 'on screen (2D)'.
	if( mode_2d ) {
		if (explode_time_diff > 0.f)
			render::esp_small.string(g_cl.m_width * 0.5f, 65, colors::white, time_str, render::ALIGN_CENTER);

		if (g_cl.m_local->alive())
			render::esp_small.string(g_cl.m_width * 0.5f, 65 + render::esp_small.m_size.m_height, damage_color, damage_str, render::ALIGN_CENTER);
	}

	// 'on bomb (3D)'.
	if( mode_3d && is_visible ) {
		if( explode_time_diff > 0.f )
			render::esp_small.string( screen_pos.x, screen_pos.y, colors::white, time_str, render::ALIGN_CENTER );

		// only render damage string if we're alive.
		if( g_cl.m_local->alive( ) )
			render::esp_small.string( screen_pos.x, ( int ) screen_pos.y + render::esp_small.m_size.m_height, damage_color, damage_str, render::ALIGN_CENTER );
	}
}

bool Visuals::GetPlayerBoxRect( Player* player, Rect& box ) {
	vec3_t origin, mins, maxs;
	vec2_t bottom, top;

	// get interpolated origin.
	origin = player->GetAbsOrigin( );

	// get hitbox bounds.
	player->ComputeHitboxSurroundingBox( &mins, &maxs );

	// correct x and y coordinates.
	mins = { origin.x, origin.y, mins.z };
	maxs = { origin.x, origin.y, maxs.z + 8.f };

	if( !render::WorldToScreen( mins, bottom ) || !render::WorldToScreen( maxs, top ) )
		return false;

	box.h = bottom.y - top.y;
	box.w = box.h / 2.f;
	box.x = bottom.x - ( box.w / 2.f );
	box.y = bottom.y - box.h;

	return true;
}

void Visuals::DrawHistorySkeleton( Player* player, int opacity ) {
	const model_t* model;
	studiohdr_t* hdr;
	mstudiobone_t* bone;
	AimPlayer* data;
	LagRecord* record;
	int           parent;
	vec3_t        bone_pos, parent_pos;
	vec2_t        bone_pos_screen, parent_pos_screen;

	if( !g_menu.main.misc.fake_latency.get( ) )
		return;

	// get player's model.
	model = player->GetModel( );
	if( !model )
		return;

	// get studio model.
	hdr = g_csgo.m_model_info->GetStudioModel( model );
	if( !hdr )
		return;

	data = &g_aimbot.m_players[ player->index( ) - 1 ];
	if( !data )
		return;

	record = g_resolver.FindLastRecord( data );
	if( !record )
		return;

	for( int i{ }; i < hdr->m_num_bones; ++i ) {
		// get bone.
		bone = hdr->GetBone( i );
		if( !bone || !( bone->m_flags & BONE_USED_BY_HITBOX ) )
			continue;

		// get parent bone.
		parent = bone->m_parent;
		if( parent == -1 )
			continue;

		// resolve main bone and parent bone positions.
		record->m_bones->get_bone( bone_pos, i );
		record->m_bones->get_bone( parent_pos, parent );

		Color clr = player->enemy( g_cl.m_local ) ? g_menu.main.players.skeleton_enemy.get( ) : g_menu.main.players.skeleton_friendly.get( );
		clr.a( ) = opacity;

		// world to screen both the bone parent bone then draw.
		if( render::WorldToScreen( bone_pos, bone_pos_screen ) && render::WorldToScreen( parent_pos, parent_pos_screen ) )
			render::line( bone_pos_screen.x, bone_pos_screen.y, parent_pos_screen.x, parent_pos_screen.y, clr );
	}
}

void Visuals::DrawSkeleton( Player* player, int opacity ) {
	const model_t* model;
	studiohdr_t* hdr;
	mstudiobone_t* bone;
	int           parent;
	BoneArray     matrix[ 128 ];
	vec3_t        bone_pos, parent_pos;
	vec2_t        bone_pos_screen, parent_pos_screen;

	// get player's model.
	model = player->GetModel( );
	if( !model )
		return;

	// get studio model.
	hdr = g_csgo.m_model_info->GetStudioModel( model );
	if( !hdr )
		return;

	// get bone matrix.
	if( !player->SetupBones( matrix, 128, BONE_USED_BY_ANYTHING, g_csgo.m_globals->m_curtime ) )
		return;

	for( int i{ }; i < hdr->m_num_bones; ++i ) {
		// get bone.
		bone = hdr->GetBone( i );
		if( !bone || !( bone->m_flags & BONE_USED_BY_HITBOX ) )
			continue;

		// get parent bone.
		parent = bone->m_parent;
		if( parent == -1 )
			continue;

		// resolve main bone and parent bone positions.
		matrix->get_bone( bone_pos, i );
		matrix->get_bone( parent_pos, parent );

		Color clr = player->enemy( g_cl.m_local ) ? g_menu.main.players.skeleton_enemy.get( ) : g_menu.main.players.skeleton_friendly.get( );
		clr.a( ) = opacity;

		// world to screen both the bone parent bone then draw.
		if( render::WorldToScreen( bone_pos, bone_pos_screen ) && render::WorldToScreen( parent_pos, parent_pos_screen ) )
			render::line( bone_pos_screen.x, bone_pos_screen.y, parent_pos_screen.x, parent_pos_screen.y, clr );
	}
}

void Visuals::RenderGlow( ) {
	Color   color;
	Player* player;

	if( !g_cl.m_local )
		return;

	if( !g_csgo.m_glow->m_object_definitions.Count( ) )
		return;

	float blend = g_menu.main.players.glow_blend.get( ) / 100.f;

	for( int i{ }; i < g_csgo.m_glow->m_object_definitions.Count( ); ++i ) {
		GlowObjectDefinition_t* obj = &g_csgo.m_glow->m_object_definitions[ i ];

		// skip non-players.
		if( !obj->m_entity || !obj->m_entity->IsPlayer( ) )
			continue;

		// get player ptr.
		player = obj->m_entity->as< Player* >( );

		if( player->m_bIsLocalPlayer( ) )
			continue;

		// get reference to array variable.
		float& opacity = m_opacities[ player->index( ) - 1 ];

		bool enemy = player->enemy( g_cl.m_local );

		if( enemy && !g_menu.main.players.glow.get( 0 ) )
			continue;

		if( !enemy && !g_menu.main.players.glow.get( 1 ) )
			continue;

		// enemy color
		if( enemy )
			color = g_menu.main.players.glow_enemy.get( );

		// friendly color
		else
			color = g_menu.main.players.glow_friendly.get( );

		obj->m_render_occluded = true;
		obj->m_render_unoccluded = false;
		obj->m_render_full_bloom = false;
		obj->m_color = { ( float ) color.r( ) / 255.f, ( float ) color.g( ) / 255.f, ( float ) color.b( ) / 255.f };
		obj->m_alpha = opacity * blend;
	}
}

void Visuals::DrawHitboxMatrix( LagRecord* record, Color col, float time ) {
	const model_t* model;
	studiohdr_t* hdr;
	mstudiohitboxset_t* set;
	mstudiobbox_t* bbox;
	vec3_t             mins, maxs, origin;
	ang_t			   angle;

	model = record->m_player->GetModel( );
	if( !model )
		return;

	hdr = g_csgo.m_model_info->GetStudioModel( model );
	if( !hdr )
		return;

	set = hdr->GetHitboxSet( record->m_player->m_nHitboxSet( ) );
	if( !set )
		return;

	for( int i{ }; i < set->m_hitboxes; ++i ) {
		bbox = set->GetHitbox( i );
		if( !bbox )
			continue;

		// bbox.
		if( bbox->m_radius <= 0.f ) {
			// https://developer.valvesoftware.com/wiki/Rotation_Tutorial

			// convert rotation angle to a matrix.
			matrix3x4_t rot_matrix;
			g_csgo.AngleMatrix( bbox->m_angle, rot_matrix );

			// apply the rotation to the entity input space (local).
			matrix3x4_t matrix;
			math::ConcatTransforms( record->m_bones[ bbox->m_bone ], rot_matrix, matrix );

			// extract the compound rotation as an angle.
			ang_t bbox_angle;
			math::MatrixAngles( matrix, bbox_angle );

			// extract hitbox origin.
			vec3_t origin = matrix.GetOrigin( );

			// draw box.
			g_csgo.m_debug_overlay->AddBoxOverlay( origin, bbox->m_mins, bbox->m_maxs, bbox_angle, col.r( ), col.g( ), col.b( ), 0, time );
		}

		// capsule.
		else {
			// NOTE; the angle for capsules is always 0.f, 0.f, 0.f.

			// create a rotation matrix.
			matrix3x4_t matrix;
			g_csgo.AngleMatrix( bbox->m_angle, matrix );

			// apply the rotation matrix to the entity output space (world).
			math::ConcatTransforms( record->m_bones[ bbox->m_bone ], matrix, matrix );

			// get world positions from new matrix.
			math::VectorTransform( bbox->m_mins, matrix, mins );
			math::VectorTransform( bbox->m_maxs, matrix, maxs );

			g_csgo.m_debug_overlay->AddCapsuleOverlay( mins, maxs, bbox->m_radius, col.r( ), col.g( ), col.b( ), col.a( ), time, 0, 0 );
		}
	}
}

void Visuals::DrawBeams( ) {
	size_t     impact_count;
	float      curtime, dist;
	bool       is_final_impact;
	vec3_t     va_fwd, start, dir, end;
	BeamInfo_t beam_info;
	Beam_t* beam;

	if( !g_cl.m_local )
		return;

	if( !g_menu.main.visuals.impact_beams.get( ) )
		return;

	auto vis_impacts = &g_shots.m_vis_impacts;

	// the local player is dead, clear impacts.
	if( !g_cl.m_processing ) {
		if( !vis_impacts->empty( ) )
			vis_impacts->clear( );
	}

	else {
		impact_count = vis_impacts->size( );
		if( !impact_count )
			return;

		curtime = game::TICKS_TO_TIME( g_cl.m_local->m_nTickBase( ) );

		for( size_t i{ impact_count }; i-- > 0; ) {
			auto impact = &vis_impacts->operator[ ]( i );
			if( !impact )
				continue;

			// impact is too old, erase it.
			if( std::abs( curtime - game::TICKS_TO_TIME( impact->m_tickbase ) ) > g_menu.main.visuals.impact_beams_time.get( ) ) {
				vis_impacts->erase( vis_impacts->begin( ) + i );

				continue;
			}

			// already rendering this impact, skip over it.
			if( impact->m_ignore )
				continue;

			// is this the final impact?
			// last impact in the vector, it's the final impact.
			if( i == ( impact_count - 1 ) )
				is_final_impact = true;

			// the current impact's tickbase is different than the next, it's the final impact.
			else if( ( i + 1 ) < impact_count && impact->m_tickbase != vis_impacts->operator[ ]( i + 1 ).m_tickbase )
				is_final_impact = true;

			else
				is_final_impact = false;

			// is this the final impact?
			// is_final_impact = ( ( i == ( impact_count - 1 ) ) || ( impact->m_tickbase != vis_impacts->at( i + 1 ).m_tickbase ) );

			if( is_final_impact ) {
				// calculate start and end position for beam.
				start = impact->m_shoot_pos;

				dir = ( impact->m_impact_pos - start ).normalized( );
				dist = ( impact->m_impact_pos - start ).length( );

				end = start + ( dir * dist );

				// setup beam info.
				// note - lazarus & dex; possible beam models: sprites/purplelaser1.vmt | sprites/white.vmt
				beam_info.m_vecStart = start;
				beam_info.m_vecEnd = end;
				beam_info.m_nModelIndex = g_csgo.m_model_info->GetModelIndex( XOR( "sprites/physbeam.vmt" ) );
				beam_info.m_pszModelName = XOR( "sprites/physbeam.vmt" );
				beam_info.m_flHaloScale = 0.f;
				beam_info.m_flLife = g_menu.main.visuals.impact_beams_time.get( );
				beam_info.m_flWidth = 3.0f;
				beam_info.m_flEndWidth = 5.0f;
				beam_info.m_flFadeLength = 0.f;
				beam_info.m_flAmplitude = 0.f;   // beam 'jitter'.
				beam_info.m_flBrightness = 255.f;
				beam_info.m_flSpeed = 0.2f;  // seems to control how fast the 'scrolling' of beam is... once fully spawned.
				beam_info.m_nStartFrame = 0;
				beam_info.m_flFrameRate = 1.f;
				beam_info.m_nSegments = 2;     // controls how much of the beam is 'split up', usually makes m_flAmplitude and m_flSpeed much more noticeable.
				beam_info.m_bRenderable = true;  // must be true or you won't see the beam.
				beam_info.m_nFlags = 0x8300;

				beam_info.m_flRed = g_menu.main.visuals.impact_beams_color.get( ).r( );
				beam_info.m_flGreen = g_menu.main.visuals.impact_beams_color.get( ).g( );
				beam_info.m_flBlue = g_menu.main.visuals.impact_beams_color.get( ).b( );
				

				// attempt to render the beam.
				beam = game::CreateGenericBeam( beam_info );
				if( beam ) {
					g_csgo.m_beams->DrawBeam( beam );

					// we only want to render a beam for this impact once.
					impact->m_ignore = true;
				}
			}
		}
	}
}

void Visuals::DebugAimbotPoints( Player* player ) {
	std::vector< vec3_t > p2{ };

	AimPlayer* data = &g_aimbot.m_players.at( player->index( ) - 1 );
	if( !data || data->m_records.empty( ) )
		return;

	LagRecord* front = data->m_records.front( ).get( );
	if( !front || front->dormant( ) )
		return;

	// get bone matrix.
	BoneArray matrix[ 128 ];
	if( !g_bones.setup( player, matrix, front ) )
		return;

	data->SetupHitboxes( front, false );
	if( data->m_hitboxes.empty( ) )
		return;

	for( const auto& it : data->m_hitboxes ) {
		std::vector< vec3_t > p1{ };

		if( !data->SetupHitboxPoints( front, matrix, it.m_index, p1 ) )
			continue;

		for( auto& p : p1 )
			p2.push_back( p );
	}

	if( p2.empty( ) )
		return;

	for( auto& p : p2 ) {
		vec2_t screen;

		if( render::WorldToScreen( p, screen ) )
			render::rect_filled( screen.x, screen.y, 2, 2, { 0, 255, 255, 255 } );
	}
}
