/**
 * @file
 *
 * Copyright (c) 2025. Grafika Strahlen LLC
 * All rights reserved.
 */
#include "Core.hpp"
#include "StreamingMultiprocessor.hpp"

void FpCore::InvokeRegisterFileHigh(const RegisterFile::CommandPacket packet) noexcept
{
    m_SM->InvokeRegisterFileHigh(m_UnitIndex & 0x2, packet);
}

void FpCore::InvokeRegisterFileLow(const RegisterFile::CommandPacket packet) noexcept
{
    m_SM->InvokeRegisterFileLow(m_UnitIndex & 0x2, packet);
}

void FpCore::ReportReady() const noexcept
{
    m_SM->ReportFpCoreReady(m_UnitIndex);
}

void IntFpCore::InvokeRegisterFileHigh(const RegisterFile::CommandPacket packet) noexcept
{
    m_SM->InvokeRegisterFileHigh(m_UnitIndex & 0x2, packet);
}

void IntFpCore::InvokeRegisterFileLow(const RegisterFile::CommandPacket packet) noexcept
{
    m_SM->InvokeRegisterFileLow(m_UnitIndex & 0x2, packet);
}

void IntFpCore::ReportReady() const noexcept
{
    m_SM->ReportIntFpCoreReady(m_UnitIndex);
}
