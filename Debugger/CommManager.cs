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

        private NamedPipeServerStream? _pipe;
        private BinaryReader? _reader;
        private BinaryWriter? _writer;

        public BinaryReader Reader => _reader!;
        public BinaryWriter Writer => _writer!;

        public bool StartPaused { get; set; }

        private bool _isReady;
        private volatile bool _connectionClosed;

        public CommManager(string pipeName)
        {
            _pipe = new NamedPipeServerStream(pipeName, PipeDirection.InOut, 1, PipeTransmissionMode.Byte);
            _isReady = false;
            _reader = new BinaryReader(_pipe);
            _writer = new BinaryWriter(_pipe);
            StartPaused = false;
            _connectionClosed = false;

            _pipe.BeginWaitForConnection(ConnectCallback, null);
        }

        ~CommManager()
        {
            Dispose(false);
        }

        protected void Dispose(bool disposing)
        {
            if(_pipe != null)
            {
                _pipe.Dispose();
                _pipe = null;
            }

            if(_reader != null)
            {
                _reader.Dispose();
                _reader = null;
            }

            if(_writer != null)
            {
                _writer.Dispose();
                _reader = null;
            }
        }

        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        private void ConnectCallback(IAsyncResult result)
        {
            _isReady = true;
            _connectionClosed = false;
            Writer.Write(DebugCodeStartPaused);
            Writer.Write(1);
            Writer.Write(StartPaused);
        }

        public bool CheckReady()
        {
            if(_connectionClosed && _isReady)
            {
                Reset();
                return false;
            }

            return _isReady;
        }

        public async Task<DebugDataHeader> ReadHeaderAsync()
        {
            return await Task.Run(() =>
            {
                if(!_isReady || _pipe == null || !_pipe.IsConnected || _connectionClosed)
                {
                    return new DebugDataHeader(DebugCodeError, 0);
                }

                try
                {
                    uint dataCode = Reader.ReadUInt32();
                    uint dataLength = Reader.ReadUInt32();

                    return new DebugDataHeader(dataCode, dataLength);
                }
                catch
                {
                    _connectionClosed = true;
                }

                return new DebugDataHeader(DebugCodeError, 0);
            });
        }

        public void Reset()
        {
            _pipe?.Disconnect();

            _isReady = false;

            _pipe?.BeginWaitForConnection(ConnectCallback, null);
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
