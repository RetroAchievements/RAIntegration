#include "LuaScript.h"

#include "RA_Defs.h"
#include "RA_MemManager.h"

namespace ra {
namespace data {

LuaScript::LuaScript()
{
}

LuaScript::~LuaScript()
{
    if (state != nullptr)
        lua_close(state);
}

static int read_memory(lua_State* state, ComparisonVariableSize size)
{
    int result = 0;

    int nParameterCount = lua_gettop(state);
    if (nParameterCount == 1)
    {
        // read address from stack
        int nAddress = lua_tointeger(state, 1);

        // do work
        result = g_MemManager.ActiveBankRAMRead(nAddress, size);
    }

    // push result onto stack
    lua_pushinteger(state, result);

    // one value being returned
    return 1;
}

static int lua_import_byte(lua_State *state)
{
    return read_memory(state, EightBit);
}

bool LuaScript::Load(const std::string& sFilename)
{
    state = luaL_newstate();

    if (luaL_loadfile(state, sFilename.c_str()) == LUA_OK && lua_pcall(state, 0, 0, 0) == LUA_OK)
    {
        // load the common libraries
        luaL_openlibs(state);

        lua_register(state, "byte", lua_import_byte);

        return true;
    }

    // the error message is on top of the stack. fetch it, log it, then pop it off the stack.
    const char* message = lua_tostring(state, -1);
    RA_LOG("error loading lua file %s: %s", sFilename.c_str(), message);
    lua_pop(state, 1);

    // load failed, release state
    lua_close(state);
    state = nullptr;
    return false;
}

bool LuaScript::Invoke(const char* sFunctionName)
{
    if (state == nullptr)
        return false;

    lua_getglobal(state, sFunctionName);
    lua_call(state, 0, 1); // 0 input parameters, 1 return value

    int result = (int)lua_toboolean(state, -1);
    lua_pop(state, 1);

    return (result != 0);
}

} // namespace data
} // namespace ra
