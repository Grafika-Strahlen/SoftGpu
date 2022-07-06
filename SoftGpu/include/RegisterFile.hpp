#pragma once

#include <Objects.hpp>
#include <NumTypes.hpp>

class RegisterFile final
{
    DEFAULT_CONSTRUCT_PU(RegisterFile);
    DEFAULT_DESTRUCT(RegisterFile);
    DELETE_CM(RegisterFile);
public:
    [[nodiscard]] u32 GetRegister(const u32 baseRegister, const u32 targetRegister) const noexcept { return m_Registers[baseRegister + targetRegister]; }
    void SetRegister(const u32 baseRegister, const u32 targetRegister, const u32 value) noexcept { m_Registers[baseRegister + targetRegister] = value; }
private:
    u32 m_Registers[4096];
};
