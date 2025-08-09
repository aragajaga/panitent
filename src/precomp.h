/*
 *  THIS FILE IS A PART OF PANIT.ENT PROJECT
 *
 *   /$$$$$$$                     /$$   /$$                              /$$
 *  | $$__  $$                   |__/  | $$                             | $$
 *  | $$  \ $$ /$$$$$$  /$$$$$$$  /$$ /$$$$$$       /$$$$$$  /$$$$$$$  /$$$$$$
 *  | $$$$$$$/|____  $$| $$__  $$| $$|_  $$_/      /$$__  $$| $$__  $$|_  $$_/
 *  | $$____/  /$$$$$$$| $$  \ $$| $$  | $$       | $$$$$$$$| $$  \ $$  | $$
 *  | $$      /$$__  $$| $$  | $$| $$  | $$ /$$   | $$_____/| $$  | $$  | $$ /$$
 *  | $$     |  $$$$$$$| $$  | $$| $$  |  $$$$//$$|  $$$$$$$| $$  | $$  |  $$$$/
 *  |__/      \_______/|__/  |__/|__/   \___/ |__/ \_______/|__/  |__/   \___/
 *
 *  Panit.ent - A lightweight graphics editor
 *  https://github.com/aragajaga/panitent
 *
 *  File name: precomp.c
 *  Description: Precompiled header for common headers used in the application
 *  Author: Philosoph228 <philosoph228@gmail.com>
 *
 *  MIT License
 *
 *  Copyright (c) 2017-2024 Aragajaga
 *
 *  Permission is hereby granted, free of charge, to any person obtainig a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT ANY WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRIGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 *
 */

#ifndef PANITENT_PRECOMP_H
#define PANITENT_PRECOMP_H

#define NTDDI_VERSION NTDDI_WIN7
#define _WIN32_WINNT  _WIN32_WINNT_WIN7
#define WINVER        _WIN32_WINNT_WIN7

#ifndef UNICODE
#define UNICODE
#endif /* UNICODE */

#ifndef _UNICODE
#define _UNICODE
#endif /* _UNICODE */

#include <windows.h>
#include <windowsx.h>
#include <initguid.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <commctrl.h>
#include <strsafe.h>
#include <knownfolders.h>
#include <math.h>

#include <stdint.h>
#include <assert.h>

#endif /* PANITENT_PRECOMP_H */
