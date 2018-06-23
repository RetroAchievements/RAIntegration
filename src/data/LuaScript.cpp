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
    Close();
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

static const struct luaL_Reg retroachievements_lua_lib[] = {
    {"byte", lua_import_byte},
    {nullptr, nullptr}
};

static int luaopen_retroachievements(lua_State *state)
{
    luaL_newlib(state, retroachievements_lua_lib);
    return 1;
}

static void dump_lua_table(lua_State* state)
{
    lua_pushvalue(state, -1); // duplicate the table reference
    lua_pushnil(state);       // placeholder for indexer
    while (lua_next(state, -2))
    {
        // stack now contains: value, key, table
        lua_pushvalue(state, -2); // second copy of key to convert to string
        const char* key = lua_tostring(state, -1);
        const char* value = lua_tostring(state, -2);
        RA_LOG("lua_table[%s]: %s\n", key, value);
        lua_pop(state, 2); // pop value and second copy of key, leaving "key, table" on the stack
    }
    // when lua_next returns false, the indexer is popped
    lua_pop(state, 1); // pop the table reference, stack should now match what it was when dump() was called.
}

bool LuaScript::Load(const std::string& sFilename)
{
    Close();

    state = luaL_newstate();

    // load the common libraries
    luaL_openlibs(state); // base, pacakage, table, string, math, utf8 - https://www.lua.org/source/5.3/linit.c.html

    // load our library
    luaL_requiref(state, "retroachievements", luaopen_retroachievements, 1);
    //lua_pop(state, 1); // remove lib

    // load the script
    if (luaL_loadfile(state, sFilename.c_str()) == LUA_OK && lua_pcall(state, 0, 1, 0) == LUA_OK)
    {
        nScriptTableRef = luaL_ref(state, LUA_REGISTRYINDEX);
        return true;
    }

    // the error message is on top of the stack. fetch it, log it, then pop it off the stack.
    const char* message = lua_tostring(state, -1);
    RA_LOG("error loading lua file %s: %s\n", sFilename.c_str(), message);
    lua_pop(state, 1);

    // load failed, release state
    lua_close(state);
    state = nullptr;
    return false;
}

void LuaScript::Close()
{
    if (state != nullptr)
    {
        luaL_unref(state, LUA_REGISTRYINDEX, nScriptTableRef);
        lua_close(state);
        state = nullptr;
    }
}

bool LuaScript::Invoke(const char* sFunctionName) const
{
    if (state == nullptr)
        return false;

    // push the function table onto the stack
    lua_rawgeti(state, LUA_REGISTRYINDEX, nScriptTableRef);

    // find the function in the table
    lua_getfield(state, -1, sFunctionName);
    if (lua_isnil(state, -1))
    {
        RA_LOG("lua function not found: %s\n", sFunctionName);
        lua_pop(state, 2); // pop nil and function table

        dump_lua_table(state);

        return false;
    }

    lua_call(state, 0, 1); // 0 input parameters, 1 return value

    int result = (int)lua_toboolean(state, -1);
    lua_pop(state, 2); // pop return value and function table

    return (result != 0);
}

} // namespace data
} // namespace ra
