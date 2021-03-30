#include "includes.h"
#include <iostream>


LuaEngine* g_pLuaEngine = new LuaEngine();
ExportedInterfaces g_Interfaces;

lua_State* LuaEngine::L()
{
	return m_L;
}

void LuaEngine::report_errors(int state)
{
	if (state)
	{
		std::cerr << "ERR: " << lua_tostring(m_L, -1) << std::endl;
		lua_pop(m_L, 1); //remove error
	}
}

void LuaEngine::ExecuteFile(const char* file)
{
	if (!file || !m_L)
		return;

	int state = luaL_dofile(m_L, file);
	report_errors(state);
}


void LuaEngine::ExecuteString(const char* expression)
{
	if (!expression || !m_L)
	{
		std::cerr << "ERR: null expression passed to script engine!" << std::endl;
		return;
	}

	int state = luaL_dostring(m_L, expression);
	report_errors(state);
}


void RegEverything(lua_State* L)
{
	LOCKLUA();
	lua_State* L = luaL_newstate();
	luaL_openlibs(L);

	//this shits broken lol

	/*luabridge::getGlobalNamespace(L)
		.beginNamespace("Game")
		.addVariable("Interfaces", &g_Interfaces, false)
		.beginClass<ExportedEngine>("EngineInterface")
		.addFunction("GetLocalPlayer", &ExportedEngine::GetLocalPlayer)
		.addFunction("ExecuteCmd", &ExportedEngine::ExecuteCommand)
		.endClass()
		.beginClass<ExportedInterfaces>("InterfaceClass")
		.addFunction("GetEngine", &ExportedInterfaces::GetEngine)
		.endClass()
		.endNamespace();*/
}

void ExportedEngine::ExecuteCommand(const char* str)
{
	g_csgo.m_engine->ExecuteClientCmd(str);
}

int ExportedEngine::GetLocalPlayer()
{
	return g_csgo.m_engine->GetLocalPlayer();
}