#pragma once

class InputPrediction {
public:
	float m_curtime;
	float m_frametime;
	struct Variables_t {
		float m_flFrametime;
		float m_flCurtime;
		float m_flVelocityModifier;

		vec3_t m_vecVelocity;
		vec3_t m_vecOrigin;
		int m_fFlags;
	} m_stored_variables;
public:
	void update( );
	void run( );
	void restore( );
};

extern InputPrediction g_inputpred;