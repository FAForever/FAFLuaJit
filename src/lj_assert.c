/*
** Internal assertions.
** Copyright (C) 2005-2022 Mike Pall. See Copyright Notice in luajit.h
*/

#define lj_assert_c
#define LUA_CORE

#if defined(LUA_USE_ASSERT) || defined(LUA_USE_APICHECK)

#include <stdio.h>

#include "lj_obj.h"

void lj_assert_fail(global_State *g, const char *file, int line,
		    const char *func, const char *fmt, ...)
{
  va_list argp;
  va_start(argp, fmt);
  fprintf(stderr, "LuaJIT ASSERT %s:%d: %s: ", file, line, func);
  vfprintf(stderr, fmt, argp);
  fputc('\n', stderr);

  if (g->throwException) {
    int len, len2;
    char* str;
    len = snprintf(0, 0, "LuaJIT ASSERT %s:%d: %s: ", file, line, func);
    len2 = vsnprintf(0, 0, fmt, argp) + 1;
    str = malloc(len + len2);
    snprintf(str, len + 1, "LuaJIT ASSERT %s:%d: %s: ", file, line, func);
    vsnprintf(str + len, len2, fmt, argp);
    typedef int (*_FileWrite)(int fileIndex, const char *str, int strlen);
    const _FileWrite FileWrite = (_FileWrite)0xA9B4E6;
    FileWrite(3, str, len + len2 - 1);
  }

  va_end(argp);
  UNUSED(g);  /* May be NULL. TODO: optionally dump state. */
  abort();
}

#endif
