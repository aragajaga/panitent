#include "precomp.h"

#include <lua/lua.h>
#include <lua/lualib.h>
#include <lua/lauxlib.h>

#include "lua_scripting.h"

PWSTR lua_string_to_wstring(lua_State* L, int index)
{
    // Check that the value at the given index is a string
    if (!lua_isstring(L, index))
    {
        return NULL;
    }

    // Get the UTF-8 string from Lua
    size_t len;
    const char* utf8_str = lua_tolstring(L, index, &len);

    // Convert UTF-8 to UTF-16
    int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8_str, -1, NULL, 0);
    if (!wlen)
    {
        return NULL;    // Conversion failed
    }

    PWSTR wstr = (PWSTR)malloc(wlen * sizeof(WCHAR));
    if (!wstr)
    {
        return NULL;    // Memory allocation failed
    }

    // Perform the conversion
    MultiByteToWideChar(CP_UTF8, 0, utf8_str, -1, wstr, wlen);

    return wstr;
}

int panitent_lua_messagebox(lua_State* L)
{
    // Check the number of arguments
    int n = lua_gettop(L);
    if (n != 1)
    {
        lua_pushstring(L, "Function requires one argument");
        lua_error(L);
    }

    // Check if the argument is a string
    if (!lua_isstring(L, 1))
    {
        lua_pushstring(L, "Argument must be string");
        lua_error(L);
    }

    // Get the string argument
    PWSTR message = lua_string_to_wstring(L, 1);
    // Print the message
    MessageBox(NULL, message, L"Info", MB_OK | MB_ICONINFORMATION);
    free(message);

    // Return no results
    return 0;
}

int register_c_functions(lua_State* L)
{
    // Register the C function with Lua
    lua_pushcfunction(L, panitent_lua_messagebox);
    lua_setglobal(L, "panitent_lua_messagebox");

    // Return the table
    return 1;
}

char* Utf16ToUtf8(PCWSTR utf16_str)
{
    // Calculate the required buffer size for UTF-8 string
    int utf8_len = WideCharToMultiByte(CP_UTF8, 0, utf16_str, -1, NULL, 0, NULL, NULL);
    if (!utf8_len)
    {
        return NULL;
    }

    // Allocate buffer of UTF-8 string
    char* utf8_str = (char*)malloc(utf8_len);
    if (!utf8_str)
    {
        return NULL;
    }

    // Perform the conversion
    WideCharToMultiByte(CP_UTF8, 0, utf16_str, -1, utf8_str, utf8_len, NULL, NULL);

    return utf8_str;
}

int Lua_RunScript(PCWSTR pcszPath)
{
    lua_State* L = luaL_newstate();
    if (!L)
    {
        MessageBox(NULL, L"Cannot create Lua state", NULL, MB_OK | MB_ICONERROR);
        return EXIT_FAILURE;
    }

    // Open Lua libraries
    luaL_openlibs(L);

    // Register C functions with Lua
    register_c_functions(L);

    // Load and execute the Lua script
    const char* filename = Utf16ToUtf8(pcszPath);
    if (luaL_dofile(L, filename) != LUA_OK)
    {
        char errorMessage[1024];
        sprintf_s(errorMessage, ARRAYSIZE(errorMessage), "Error: %s\n", lua_tostring(L, -1));
        MessageBoxA(NULL, errorMessage, "Panit.ent Crash Handler", MB_OK | MB_ICONERROR);
        lua_pop(L, 1);  // Remove error message from stack
    }
    else {
        // Script executed successfully
        // If the script returns a value, it will ve on the stack
        if (lua_isnumber(L, -1))
        {
            int result = lua_tointeger(L, -1);
            printf("Lua script returned: %d\n", result);
        }
    }

    // Close Lua
    lua_close(L);

    return 0;
}
