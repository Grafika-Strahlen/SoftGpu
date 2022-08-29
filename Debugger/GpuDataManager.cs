using System.Timers;
using Timer = System.Timers.Timer;

namespace Debugger
{
    public class GpuDataManager
    {
        private readonly CommManager _commManager;
        private readonly Thread _thread;

        public readonly uint[,] Registers;
        public readonly byte[,] RegisterContestion;
        public readonly uint[,,] BaseRegisters;

        public volatile bool RegistersDirty;

        public volatile uint ClockCycle;

        public GpuDataManager(CommManager commManager)
        {
            _commManager = commManager;
            _thread = new Thread(ThreadLoop);

            Registers = new uint[4, 4096];
            RegisterContestion = new byte[4, 4096];
            BaseRegisters = new uint[4, 2, 4];

            RegistersDirty = false;
            ClockCycle = 0;

            _thread.Start();
        }

        private void ThreadLoop()
        {
            while(true)
            {
                if(!_commManager.CheckReady())
                {
                    continue;
                }

                try
                {
                    DebugDataHeader dataHeader = _commManager.ReadInfoHeader();

                    if(dataHeader.DataCode == CommManager.DebugCodeError)
                    {
                        return;
                    }

                    if(dataHeader.DataCode == CommManager.DebugCodeReportTiming)
                    {
                        ClockCycle = _commManager.InfoReader.ReadUInt32();
                    }
                    else if(dataHeader.DataCode == CommManager.DebugCodeReportRegisterFile)
                    {
                        uint sm = _commManager.InfoReader.ReadUInt32();

                        for(uint i = 0; i < 4096; ++i)
                        {
                            Registers[sm, i] = _commManager.InfoReader.ReadUInt32();
                        }

                        RegistersDirty = true;
                    }
                    else if(dataHeader.DataCode == CommManager.DebugCodeReportBaseRegister)
                    {
                        uint sm = _commManager.InfoReader.ReadUInt32();
                        uint dispatchUnit = _commManager.InfoReader.ReadUInt32();

                        for(uint i = 0; i < 4; ++i)
                        {
                            BaseRegisters[sm, dispatchUnit, i] = _commManager.InfoReader.ReadUInt32();
                        }

                        RegistersDirty = true;
                    }
                    else if(dataHeader.DataCode == CommManager.DebugCodeReportRegisterContestion)
                    {
                        uint sm = _commManager.InfoReader.ReadUInt32();

                        for(uint i = 0; i < 4096; ++i)
                        {
                            RegisterContestion[sm, i] = _commManager.InfoReader.ReadByte();
                        }

                        RegistersDirty = true;
                    }
                    else
                    {
                        if(dataHeader.DataCode != 0)
                        {
                            // If we don't recognize the data code, skip the data.
                            _commManager.InfoReader.ReadBytes((int)dataHeader.DataLength);
                        }
                    }
                }
                catch
                {
                    _commManager.Reset();
                }

            }
        }
    }
}
