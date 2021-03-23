#pragma once

class ShotRecord;

class Resolver {
public:
	enum Modes : size_t {
		RESOLVE_NONE = 0,
		RESOLVE_WALK,
		RESOLVE_EXPLOIT,
		RESOLVE_STAND,
		RESOLVE_STAND1,
		RESOLVE_STAND2,
		RESOLVE_AIR,
		RESOLVE_LASTMOVE,
		RESOLVE_BODY,
		RESOLVE_STOPPED_MOVING,
		RESOLVE_LBY_UPDATE,
		RESOLVE_OVERRIDE,
		RESOLVE_LAST_LBY,
		RESOLVE_BRUTEFORCE,
		RESOLVE_FREESTAND
	};

public:
	LagRecord* FindIdealRecord(AimPlayer* data);
	LagRecord* FindLastRecord(AimPlayer* data);

	void OnBodyUpdate(Player* player, float value);
	float GetDirectionAngle(int index, Player* player);
	float GetAwayAngle(LagRecord* record);

	void MatchShot(AimPlayer* data, LagRecord* record);
	void Override(LagRecord* record);
	void SetMode(AimPlayer* data, LagRecord* record);

	void ResolveAngles(Player* player, LagRecord* record);
	void ResolveWalk(AimPlayer* data, LagRecord* record);
	void ResetNiggaShit(AimPlayer* data, bool printDebug);
	float GetLBYRotatedYaw(float lby, float yaw);
	bool IsYawSideways(Player* entity, float yaw);
	void LastMoveLby(LagRecord* record, AimPlayer* data, Player* player);
	void ResolveStand(AimPlayer* data, LagRecord* record);
	void ExploitFix(AimPlayer* data, LagRecord* record);
	void StandNS(AimPlayer* data, LagRecord* record);
	void ResolveAir(AimPlayer* data, LagRecord* record);

	void AirNS(AimPlayer* data, LagRecord* record);
	void AntiFreestand(LagRecord* record);
	void ResolvePoses(Player* player, LagRecord* record);

public:
	std::array< vec3_t, 64 > m_impacts;
	int value = 0;
};

extern Resolver g_resolver;