/**
 * @file
 *
 * Copyright (c) 2025. Grafika Strahlen LLC
 * All rights reserved.
 */
#pragma once

#define RT_STRICT
#include <VBox/log.h>
#include <ConPrinter.hpp>
#include <mutex>

extern ::std::mutex LogMutex;

template<typename Char>
inline u32 ConLog(const Char* str) noexcept
{
    ::std::lock_guard lock(LogMutex);

    const u32 ret = ConPrinter::Print(str);
    LogRel((str));
    return ret;
}

template<typename Char>
inline u32 ConLogLn(const Char* str) noexcept
{
    ::std::lock_guard lock(LogMutex);

    const u32 ret = ConPrinter::PrintLn(str);
    LogRel(("%s\n", str));
    return ret;
}

template<typename Char, typename CurrArg, typename... Args>
inline u32 ConLog(const Char* fmt, CurrArg currArg, const Args&... args) noexcept
{
    StringFormatContext<Char> ctx;
    const u32 ret = InternalFormat0(ctx, fmt, currArg, args...);
    const DynStringT<Char> str = ctx.Builder.ToString();

    ::std::lock_guard lock(LogMutex);

    ConPrinter::Print(str);
    LogRel((StringCast<char>(str).String()));
    return ret;
}

template<typename Char, typename CurrArg, typename... Args>
inline u32 ConLogLn(const Char* fmt, CurrArg currArg, const Args&... args) noexcept
{
    StringFormatContext<Char> ctx;
    const u32 ret = InternalFormat0(ctx, fmt, currArg, args...);
    const DynStringT<Char> str = ctx.Builder.Append('\n').ToString();

    ::std::lock_guard lock(LogMutex);

    ConPrinter::Print(str);
    LogRel((StringCast<char>(str).String()));
    return ret;
}

template<typename Char>
inline DECLCALLBACK(uSys) VBoxConLogRCCallback(void* const userParam, const char* const chars, const uSys count)
{
    StringFormatContext<Char> ctx;
    DynStringT<Char> str = ctx.Builder.Append('\n').ToString();
    *reinterpret_cast<DynStringT<Char>*>(userParam) = ::std::move(str);
    return count;
}

template<typename Char>
inline DynStringT<Char> ConLogRCDefine(int rc)
{
    char buffer[256];
    DynStringT<Char> str;
    (void) RTErrFormatDefine(rc, VBoxConLogRCCallback<Char>, &str, buffer, ::std::size(buffer));
    return str;
}

template<typename Char>
inline DynStringT<Char> ConLogRCMsgShort(int rc)
{
    char buffer[512];
    DynStringT<Char> str;
    (void) RTErrFormatMsgShort(rc, VBoxConLogRCCallback<Char>, &str, buffer, ::std::size(buffer));
    return str;
}

template<typename Char>
inline DynStringT<Char> ConLogRCMsgFull(int rc)
{
    char buffer[1024];
    DynStringT<Char> str;
    (void) RTErrFormatMsgFull(rc, VBoxConLogRCCallback<Char>, &str, buffer, ::std::size(buffer));
    return str;
}
