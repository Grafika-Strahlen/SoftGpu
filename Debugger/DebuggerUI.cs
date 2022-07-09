using System.Globalization;
using Keys = System.Windows.Forms.Keys;
using Timer = System.Windows.Forms.Timer;

namespace Debugger
{
    public partial class DebuggerUI : Form
    {
        private readonly CommManager _commManager;
        private readonly Timer _timer = new Timer();
        private bool _isStepping;
        private bool _stepReady;

        private readonly uint[,] _registers;
        private readonly byte[,] _registerContestion;
        private readonly uint[,,] _baseRegisters;
        private uint _registerBaseView;

        public DebuggerUI()
        {
            InitializeComponent();

            _commManager = new CommManager("gpu-pipe");
            _isStepping = false;
            _stepReady = false;
            _registers = new uint[4, 4096];
            _registerContestion = new byte[4, 4096];
            _baseRegisters = new uint[4, 2, 4];
            _registerBaseView = 0;

            _timer.Tick += Tick;
            _timer.Interval = 5;
            _timer.Enabled = true;

            _timer.Start();
        }

        private async void Tick(object? obj, EventArgs eventArgs)
        {
            _commManager.StartPaused = startPausedCheckbox.Checked;

            if(!_commManager.CheckReady())
            {
                return;
            }

            if(_stepReady)
            {
                return;
            }
            
            try
            {
                DebugDataHeader dataHeader = await _commManager.ReadHeaderAsync();

                if(dataHeader.DataCode == CommManager.DebugCodeError)
                {
                    return;
                }

                if(dataHeader.DataCode == CommManager.DebugCodeReportTiming)
                {
                    uint clockCycle = await Task.Run(() => _commManager.Reader.ReadUInt32());
                    clockCycleDisplay.Text = clockCycle.ToString();
                }
                else if(dataHeader.DataCode == CommManager.DebugCodeReportStepReady)
                {
                    _stepReady = true;
                    resumeButton.Enabled = true;
                    stepCycleButton.Enabled = true;
                }
                else if(dataHeader.DataCode == CommManager.DebugCodeCheckForPause)
                {
                    if(!_isStepping)
                    {
                        pauseButton.Enabled = true;
                    }
                    await Task.Run(() =>
                    {
                        if(_isStepping)
                        {
                            _commManager.Writer.Write(CommManager.DebugCodePause);
                            _commManager.Writer.Write(0);
                        }
                        else
                        {
                            _commManager.Writer.Write(CommManager.DebugCodeNop);
                            _commManager.Writer.Write(0);
                        }
                    });
                }
                else if(dataHeader.DataCode == CommManager.DebugCodeReportRegisterFile)
                {
                    uint sm = _commManager.Reader.ReadUInt32();

                    for(uint i = 0; i < 4096; ++i)
                    {
                        _registers[sm, i] = _commManager.Reader.ReadUInt32();
                    }

                    ReRenderRegisters();
                }
                else if(dataHeader.DataCode == CommManager.DebugCodeReportBaseRegister)
                {
                    uint sm = _commManager.Reader.ReadUInt32();
                    uint dispatchUnit = _commManager.Reader.ReadUInt32();

                    for (uint i = 0; i < 4; ++i)
                    {
                        _baseRegisters[sm, dispatchUnit, i] = _commManager.Reader.ReadUInt32();
                    }

                    ReRenderRegisters();
                }
                else if(dataHeader.DataCode == CommManager.DebugCodeReportRegisterContestion)
                {
                    uint sm = _commManager.Reader.ReadUInt32();

                    for (uint i = 0; i < 4096; ++i)
                    {
                        _registerContestion[sm, i] = _commManager.Reader.ReadByte();
                    }

                    ReRenderRegisters();
                }
                else
                {
                    if(dataHeader.DataCode != 0)
                    {
                        // If we don't recognize the data code, skip the data.
                        await Task.Run(() => _commManager.Reader.ReadBytes((int) dataHeader.DataLength));
                    }
                }
            }
            catch
            {
                _commManager.Reset();
            }
        }

        private void BreakOnLaunchChange(object sender, EventArgs e)
        {
            _commManager.StartPaused = startPausedCheckbox.Checked;
            if(!_commManager.CheckReady())
            {
                _isStepping = startPausedCheckbox.Checked;
            }
        }

        private void PauseClick(object sender, EventArgs e)
        {
            if(pauseButton.Enabled)
            {
                pauseButton.Enabled = false;
                resumeButton.Enabled = true;
                stepCycleButton.Enabled = true;
                _isStepping = true;
            }
        }

        private async void ResumeClick(object sender, EventArgs e)
        {
            if(resumeButton.Enabled && _stepReady)
            {
                _stepReady = false;
                _isStepping = false;

                pauseButton.Enabled = true;
                resumeButton.Enabled = false;
                stepCycleButton.Enabled = false;
                
                await Task.Run(() =>
                {
                    _commManager.Writer.Write(CommManager.DebugCodeResume);
                    _commManager.Writer.Write(0);
                });
            }
        }

        private async void StepCycleClick(object sender, EventArgs e)
        {
            await StepCycle();
        }

        private async Task StepCycle()
        {
            if(stepCycleButton.Enabled && _stepReady)
            {
                _stepReady = false;

                stepCycleButton.Enabled = false;


                await Task.Run(() =>
                {
                    if(_commManager.CheckReady())
                    {
                        _commManager.Writer.Write(CommManager.DebugCodeStep);
                        _commManager.Writer.Write(0);
                    }
                });
            }
        }

        private void RegisterViewSelect(object sender, EventArgs e)
        {
            ReRenderRegisters();
        }

        private void ReRenderRegisters()
        {
            if(smSelector.SelectedIndex == -1 || dispatchUnitSelector.SelectedIndex == -1 || replicationIndexSelector.SelectedIndex == -1)
            {
                baseRegisterDisplay.Text = "";
                return;
            }

            _registerBaseView = _baseRegisters[smSelector.SelectedIndex, dispatchUnitSelector.SelectedIndex, replicationIndexSelector.SelectedIndex];
            baseRegisterDisplay.Text = _registerBaseView.ToString();

            registerListView.Items.Clear();

            for(uint i = 0; i < 256; ++i)
            {
                uint registerIndex = _registerBaseView + i;

                string[] data = new string[3];

                data[0] = registerIndex.ToString();

                uint registerData = _registers[smSelector.SelectedIndex, registerIndex];

                if(displayFpCheckBox.Checked)
                {
                    data[1] = BitConverter.UInt32BitsToSingle(registerData).ToString(CultureInfo.CurrentUICulture);
                }
                else
                {
                    data[1] = "0x" + registerData.ToString("X8");
                }

                data[2] = _registerContestion[smSelector.SelectedIndex, registerIndex].ToString();

                registerListView.Items.Add(new ListViewItem(data));
            }
        }

        private async void OnKeyUp(object sender, KeyEventArgs e)
        {
            if(e.KeyCode == Keys.F10)
            {
                await StepCycle();
            }
        }
    }
}
