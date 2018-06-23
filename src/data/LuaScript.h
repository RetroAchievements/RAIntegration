#ifndef LUA_SCRIPT_H
#define LUA_SCRIPT_H
#pragma once

#include <lua/lua.hpp>

#include <memory>
#include <string>

namespace ra {
namespace data {

class LuaScript
{
public:
    LuaScript();
    ~LuaScript();

    bool Load(const std::string& sFilename);

    bool Invoke(const char* sFunctionName);

private:
    lua_State* state = nullptr;
};

} // namespace data
} // namespace ra

#endif // !LUA_SCRIPT_H
