using System.IO.Pipes;

namespace Debugger
{
    public class CommManager : IDisposable
    {
        public const uint DebugCodeError = 0xFFFFFFFF;
        public const uint DebugCodeNop = 0;
        public const uint DebugCodeStartPaused = 1;
        public const uint DebugCodeReportTiming = 2;
        public const uint DebugCodeReportStepReady = 3;
        public const uint DebugCodePause = 4;
        public const uint DebugCodeResume = 5;
        public const uint DebugCodeStep = 6;
        public const uint DebugCodeCheckForPause = 7;
        public const uint DebugCodeReportRegisterFile = 8;
        public const uint DebugCodeReportBaseRegister = 9;
        public const uint DebugCodeReportRegisterContestion = 10;

        private NamedPipeServerStream? _steppingPipe;
        private NamedPipeServerStream? _infoPipe;
        private BinaryReader? _steppingReader;
        private BinaryWriter? _steppingWriter;
        private BinaryReader? _infoReader;
        private BinaryWriter? _infoWriter;

        public BinaryReader SteppingReader => _steppingReader!;
        public BinaryWriter SteppingWriter => _steppingWriter!;

        public BinaryReader InfoReader => _infoReader!;
        public BinaryWriter InfoWriter => _infoWriter!;

        public bool StartPaused { get; set; }

        private bool _isSteppingReady;
        private bool _isInfoReady;
        private volatile bool _steppingConnectionClosed;
        private volatile bool _infoConnectionClosed;

        public CommManager(string steppingPipeName, string infoPipeName)
        {
            _steppingPipe = new NamedPipeServerStream(steppingPipeName, PipeDirection.InOut, 1, PipeTransmissionMode.Byte);
            _infoPipe = new NamedPipeServerStream(infoPipeName, PipeDirection.InOut, 1, PipeTransmissionMode.Byte);

            _steppingReader = new BinaryReader(_steppingPipe);
            _steppingWriter = new BinaryWriter(_steppingPipe);
            _infoReader = new BinaryReader(_infoPipe);
            _infoWriter = new BinaryWriter(_infoPipe);

            StartPaused = false;

            _isSteppingReady = false;
            _isInfoReady = false;

            _steppingConnectionClosed = false;
            _infoConnectionClosed = false;

            _steppingPipe.BeginWaitForConnection(SteppingConnectCallback, null);
            _infoPipe.BeginWaitForConnection(InfoConnectCallback, null);
        }

        ~CommManager()
        {
            Dispose(false);
        }

        protected void Dispose(bool disposing)
        {
            if(_steppingPipe != null)
            {
                _steppingPipe.Dispose();
                _steppingPipe = null;
            }

            if(_infoPipe != null)
            {
                _infoPipe.Dispose();
                _infoPipe = null;
            }

            if(_steppingReader != null)
            {
                _steppingReader.Dispose();
                _steppingReader = null;
            }

            if(_steppingWriter != null)
            {
                _steppingWriter.Dispose();
                _steppingWriter = null;
            }

            if(_infoReader != null)
            {
                _infoReader.Dispose();
                _infoReader = null;
            }

            if(_infoWriter != null)
            {
                _infoWriter.Dispose();
                _infoWriter = null;
            }
        }

        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        private void SteppingConnectCallback(IAsyncResult result)
        {
            _isSteppingReady = true;
            _steppingConnectionClosed = false;
            SteppingWriter.Write(DebugCodeStartPaused);
            SteppingWriter.Write(1);
            SteppingWriter.Write(StartPaused);
        }

        private void InfoConnectCallback(IAsyncResult result)
        {
            _isInfoReady = true;
            _infoConnectionClosed = false;
        }

        public bool CheckReady()
        {
            if(_steppingConnectionClosed && _isSteppingReady)
            {
                Reset();
                return false;
            }

            if(_infoConnectionClosed && _isInfoReady)
            {
                Reset();
                return false;
            }

            return _isSteppingReady && _isInfoReady;
        }

        public async Task<DebugDataHeader> ReadSteppingHeaderAsync()
        {
            return await Task.Run(() =>
            {
                if(!_isSteppingReady || _steppingPipe == null || !_steppingPipe.IsConnected || _steppingConnectionClosed)
                {
                    return new DebugDataHeader(DebugCodeError, 0);
                }

                try
                {
                    uint dataCode = SteppingReader.ReadUInt32();
                    uint dataLength = SteppingReader.ReadUInt32();

                    return new DebugDataHeader(dataCode, dataLength);
                }
                catch
                {
                    _steppingConnectionClosed = true;
                }

                return new DebugDataHeader(DebugCodeError, 0);
            });
        }
        public DebugDataHeader ReadInfoHeader()
        {
            if(!_isInfoReady || _infoPipe == null || !_infoPipe.IsConnected || _infoConnectionClosed)
            {
                return new DebugDataHeader(DebugCodeError, 0);
            }

            try
            {
                uint dataCode = InfoReader.ReadUInt32();
                uint dataLength = InfoReader.ReadUInt32();

                return new DebugDataHeader(dataCode, dataLength);
            }
            catch
            {
                _steppingConnectionClosed = true;
            }

            return new DebugDataHeader(DebugCodeError, 0);
        }

        public async Task<DebugDataHeader> ReadInfoHeaderAsync()
        {
            return await Task.Run(() =>
            {
                if(!_isInfoReady || _infoPipe == null || !_infoPipe.IsConnected || _infoConnectionClosed)
                {
                    return new DebugDataHeader(DebugCodeError, 0);
                }

                try
                {
                    uint dataCode = InfoReader.ReadUInt32();
                    uint dataLength = InfoReader.ReadUInt32();

                    return new DebugDataHeader(dataCode, dataLength);
                }
                catch
                {
                    _steppingConnectionClosed = true;
                }

                return new DebugDataHeader(DebugCodeError, 0);
            });
        }

        public void Reset()
        {
            _steppingPipe?.Disconnect();
            _infoPipe?.Disconnect();

            _isSteppingReady = false;
            _isInfoReady = false;

            _steppingPipe?.BeginWaitForConnection(SteppingConnectCallback, null);
            _infoPipe?.BeginWaitForConnection(InfoConnectCallback, null);
        }
    }

    public struct DebugDataHeader
    {
        public uint DataCode;
        public uint DataLength;

        public DebugDataHeader(uint dataCode, uint dataLength)
        {
            DataCode = dataCode;
            DataLength = dataLength;
        }
    }
}
