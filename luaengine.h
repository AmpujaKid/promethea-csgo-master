#pragma once
#include "includes.h"

#define LOCKLUA() std::lock_guard<std::mutex> lock(g_pLuaEngine->m)

class LuaEngine
{
public:
	LuaEngine() : m_L(luaL_newstate()) { luaL_openlibs(m_L); }
	LuaEngine(const LuaEngine& other);
	LuaEngine& operator=(const LuaEngine&);
	~LuaEngine() { lua_close(m_L); }

	lua_State* L();

	void ExecuteFile(const char* file);

	void ExecuteString(const char* expression);

	void Reset()
	{
		if (m_L)
			lua_close(m_L);
		m_L = luaL_newstate();
		luaL_openlibs(m_L);
	}

	std::mutex m;
private:
	lua_State* m_L;
	void report_errors(int state);
};
class ExportedEngine
{
public:
	ExportedEngine(IVEngineClient* engine) : m_pEngine(engine) {}
	int GetLocalPlayer();
	void ExecuteCommand(const char* str);
private:
	IVEngineClient* m_pEngine;
};
class ExportedInterfaces
{
public:
	ExportedEngine GetEngine()
	{
		static ExportedEngine engine(g_csgo.m_engine);
		return engine;
	}
};

extern LuaEngine* g_pLuaEngine;