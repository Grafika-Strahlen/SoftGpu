#pragma once

#define RT_STRICT
#include <VBox/log.h>
#include <ConPrinter.hpp>

template<typename Char>
inline u32 ConLog(const Char* str) noexcept
{
    const u32 ret = ConPrinter::Print(str);
    LogRel((str));
    return ret;
}

template<typename Char>
inline u32 ConLogLn(const Char* str) noexcept
{
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
    ConPrinter::Print(str);
    LogRel((StringCast<char>(str).String()));
    return ret;
}
