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
    virtual ~LuaScript();

    bool Load(const std::string& sFilename);
    void Close();

    bool Invoke(const char* sFunctionName) const;

private:
    lua_State* state = nullptr;
    int nScriptTableRef = 0;
};

} // namespace data
} // namespace ra

#endif // !LUA_SCRIPT_H
