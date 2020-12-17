#pragma once

struct OffScreenDamageData_t {
    float m_time, m_color_step;
    Color m_color;

    __forceinline OffScreenDamageData_t( ) : m_time{ 0.f }, m_color{ colors::white } {}
    __forceinline OffScreenDamageData_t( float time, float m_color_step, Color color ) : m_time{ time }, m_color{ color } {}
};

struct ImpactInfo_t {
	float x, y, z;
	long long time;
};

struct HitmarkerInfo_t {
	ImpactInfo_t impact;
	int alpha;
	int damage;
	bool kill;
	bool didhs;
};

class Visuals {
public:
	std::array< bool, 64 >                  m_draw;
	std::array< float, 2048 >               m_opacities;
    std::array< OffScreenDamageData_t, 64 > m_offscreen_damage;
	vec2_t                                  m_crosshair;
	std::vector<ImpactInfo_t>               m_impacts;
	std::vector<HitmarkerInfo_t>            m_hitmarkers;
	bool                                    m_thirdperson;
	bool                                    m_old_thirdperson;
	float					                m_hit_start, m_hit_end, m_hit_duration;

    // info about planted c4.
    bool        m_c4_planted;
    Entity      *m_planted_c4;
    float       m_planted_c4_explode_time;
    vec3_t      m_planted_c4_explosion_origin;
    float       m_planted_c4_damage;
    float       m_planted_c4_radius;
    float       m_planted_c4_radius_scaled;
    std::string m_last_bombsite;

	IMaterial* smoke1;
	IMaterial* smoke2;
	IMaterial* smoke3;
	IMaterial* smoke4;

    std::unordered_map< int, char > m_weapon_icons = {
		{ DEAGLE, 'A' },
		{ ELITE, 'B' },
		{ FIVESEVEN, 'C' },
		{ GLOCK, 'D' },
		{ AK47, 'W' },
		{ AUG, 'U' },
		{ AWP, 'Z' },
		{ FAMAS, 'R' },
		{ G3SG1, 'X' },
		{ GALIL, 'Q' },
		{ M249, 'f' },
		{ M4A4, 'S' },
		{ MAC10, 'K' },
		{ P90, 'P' },
		{ UMP45, 'L' },
		{ XM1014, 'b' },
		{ BIZON, 'M' },
		{ MAG7, 'd' },
		{ NEGEV, 'g' },
		{ SAWEDOFF, 'c' },
		{ TEC9, 'H' },
		{ ZEUS, 'h' },
		{ P2000, 'F' },
		{ MP7, 'N' },
		{ MP9, 'O' },
		{ NOVA, 'e' },
		{ P250, 'E' },
		{ SCAR20, 'Y' },
		{ SG553, 'V' },
		{ SSG08, 'a' },
		{ KNIFE_CT, '1' },
		{ FLASHBANG, 'i' },
		{ HEGRENADE, 'j' },
		{ SMOKE, 'k' },
		{ MOLOTOV, 'l' },
		{ DECOY, 'm' },
		{ FIREBOMB, 'n' },
		{ C4, 'o' },
		{ KNIFE_T, '1' },
		{ M4A1S, 'T' },
		{ USPS, 'G' },
		{ CZ75A, 'I' },
		{ REVOLVER, 'J' },
		{ KNIFE_BAYONET, '1' },
		{ KNIFE_FLIP, '2' },
		{ KNIFE_GUT, '3' },
		{ KNIFE_KARAMBIT, '4' },
		{ KNIFE_M9_BAYONET, '5' },
		{ KNIFE_HUNTSMAN, '6' },
		{ KNIFE_FALCHION, '7' },
		{ KNIFE_BOWIE, '7' },
		{ KNIFE_BUTTERFLY, '8' },
		{ KNIFE_SHADOW_DAGGERS, '9' },
    };

public:
	static void ModulateWorld( );
	void ThirdpersonThink( );
	void Hitmarker( );
	void PlayerHurt(IGameEvent * pEvent);
	void BulletImpact(IGameEvent * pEvent);
	void HitmarkerWorld();
	void NoSmoke( );
	void think( );
	void Spectators( );
	void StatusIndicators( );
	void ManualArrows();
	void SpreadCrosshair( );
    void PenetrationCrosshair( );
	void PenetrationReticle();
	void PenetrationCircle();
	void WorldPenetrationRect();
	void draw( Entity* ent );
	void DrawProjectile( Weapon* ent );
	void DrawItem( Weapon* item );
	void OffScreen( Player* player, int alpha );
	void DrawPlayer( Player* player );
	void DrawPlantedC4(Entity * ent);
	bool GetPlayerBoxRect( Player* player, Rect& box );
	void DrawHistorySkeleton( Player* player, int opacity );
	void DrawSkeleton( Player* player, int opacity );
	void RenderGlow( );
	void DrawHitboxMatrix( LagRecord* record, Color col, float time );
    void DrawBeams( );
	void DebugAimbotPoints( Player* player );
};

extern Visuals g_visuals;