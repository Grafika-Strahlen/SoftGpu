namespace Debugger
{
    partial class DebuggerUI
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.panel1 = new System.Windows.Forms.Panel();
            this.Registers = new System.Windows.Forms.TabControl();
            this.tabPage1 = new System.Windows.Forms.TabPage();
            this.flowLayoutPanel1 = new System.Windows.Forms.FlowLayoutPanel();
            this.panel2 = new System.Windows.Forms.Panel();
            this.clockCycleDisplay = new System.Windows.Forms.TextBox();
            this.clockCycleLabel = new System.Windows.Forms.Label();
            this.panel3 = new System.Windows.Forms.Panel();
            this.resumeButton = new System.Windows.Forms.Button();
            this.pauseButton = new System.Windows.Forms.Button();
            this.stepCycleButton = new System.Windows.Forms.Button();
            this.startPausedCheckbox = new System.Windows.Forms.CheckBox();
            this.tabPage2 = new System.Windows.Forms.TabPage();
            this.registerListView = new System.Windows.Forms.ListView();
            this.registerViewIndexColumn = new System.Windows.Forms.ColumnHeader();
            this.registerViewValueColumn = new System.Windows.Forms.ColumnHeader();
            this.registerViewContestionColumn = new System.Windows.Forms.ColumnHeader();
            this.registerControlPanel = new System.Windows.Forms.Panel();
            this.splitContainer1 = new System.Windows.Forms.SplitContainer();
            this.displayFpLabel = new System.Windows.Forms.Label();
            this.baseRegisterLabel = new System.Windows.Forms.Label();
            this.replicationIndexLabel = new System.Windows.Forms.Label();
            this.dispatchUnitLabel = new System.Windows.Forms.Label();
            this.smLabel = new System.Windows.Forms.Label();
            this.displayFpCheckBox = new System.Windows.Forms.CheckBox();
            this.baseRegisterDisplay = new System.Windows.Forms.TextBox();
            this.replicationIndexSelector = new System.Windows.Forms.ComboBox();
            this.dispatchUnitSelector = new System.Windows.Forms.ComboBox();
            this.smSelector = new System.Windows.Forms.ComboBox();
            this.panel1.SuspendLayout();
            this.Registers.SuspendLayout();
            this.tabPage1.SuspendLayout();
            this.flowLayoutPanel1.SuspendLayout();
            this.panel2.SuspendLayout();
            this.panel3.SuspendLayout();
            this.tabPage2.SuspendLayout();
            this.registerControlPanel.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.splitContainer1)).BeginInit();
            this.splitContainer1.Panel1.SuspendLayout();
            this.splitContainer1.Panel2.SuspendLayout();
            this.splitContainer1.SuspendLayout();
            this.SuspendLayout();
            // 
            // panel1
            // 
            this.panel1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.panel1.AutoScroll = true;
            this.panel1.Controls.Add(this.Registers);
            this.panel1.Location = new System.Drawing.Point(12, 12);
            this.panel1.Name = "panel1";
            this.panel1.Size = new System.Drawing.Size(853, 459);
            this.panel1.TabIndex = 0;
            // 
            // Registers
            // 
            this.Registers.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.Registers.Controls.Add(this.tabPage1);
            this.Registers.Controls.Add(this.tabPage2);
            this.Registers.Location = new System.Drawing.Point(3, 3);
            this.Registers.Name = "Registers";
            this.Registers.SelectedIndex = 0;
            this.Registers.Size = new System.Drawing.Size(847, 453);
            this.Registers.TabIndex = 0;
            // 
            // tabPage1
            // 
            this.tabPage1.Controls.Add(this.flowLayoutPanel1);
            this.tabPage1.Location = new System.Drawing.Point(4, 24);
            this.tabPage1.Name = "tabPage1";
            this.tabPage1.Padding = new System.Windows.Forms.Padding(3);
            this.tabPage1.Size = new System.Drawing.Size(839, 425);
            this.tabPage1.TabIndex = 0;
            this.tabPage1.Text = "Basic Info";
            this.tabPage1.UseVisualStyleBackColor = true;
            // 
            // flowLayoutPanel1
            // 
            this.flowLayoutPanel1.Controls.Add(this.panel2);
            this.flowLayoutPanel1.Controls.Add(this.panel3);
            this.flowLayoutPanel1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.flowLayoutPanel1.Location = new System.Drawing.Point(3, 3);
            this.flowLayoutPanel1.Name = "flowLayoutPanel1";
            this.flowLayoutPanel1.Size = new System.Drawing.Size(833, 419);
            this.flowLayoutPanel1.TabIndex = 1;
            // 
            // panel2
            // 
            this.panel2.AutoScroll = true;
            this.panel2.Controls.Add(this.clockCycleDisplay);
            this.panel2.Controls.Add(this.clockCycleLabel);
            this.panel2.Location = new System.Drawing.Point(3, 3);
            this.panel2.Name = "panel2";
            this.panel2.Size = new System.Drawing.Size(200, 47);
            this.panel2.TabIndex = 0;
            // 
            // clockCycleDisplay
            // 
            this.clockCycleDisplay.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.clockCycleDisplay.Location = new System.Drawing.Point(3, 18);
            this.clockCycleDisplay.Name = "clockCycleDisplay";
            this.clockCycleDisplay.Size = new System.Drawing.Size(194, 23);
            this.clockCycleDisplay.TabIndex = 1;
            // 
            // clockCycleLabel
            // 
            this.clockCycleLabel.AutoSize = true;
            this.clockCycleLabel.Location = new System.Drawing.Point(3, 0);
            this.clockCycleLabel.Name = "clockCycleLabel";
            this.clockCycleLabel.Size = new System.Drawing.Size(69, 15);
            this.clockCycleLabel.TabIndex = 0;
            this.clockCycleLabel.Text = "Clock Cycle";
            // 
            // panel3
            // 
            this.panel3.Controls.Add(this.resumeButton);
            this.panel3.Controls.Add(this.pauseButton);
            this.panel3.Controls.Add(this.stepCycleButton);
            this.panel3.Controls.Add(this.startPausedCheckbox);
            this.panel3.Location = new System.Drawing.Point(209, 3);
            this.panel3.Name = "panel3";
            this.panel3.Size = new System.Drawing.Size(129, 115);
            this.panel3.TabIndex = 1;
            // 
            // resumeButton
            // 
            this.resumeButton.Enabled = false;
            this.resumeButton.Location = new System.Drawing.Point(3, 53);
            this.resumeButton.Name = "resumeButton";
            this.resumeButton.Size = new System.Drawing.Size(116, 23);
            this.resumeButton.TabIndex = 4;
            this.resumeButton.Text = "Resume";
            this.resumeButton.UseVisualStyleBackColor = true;
            this.resumeButton.Click += new System.EventHandler(this.ResumeClick);
            // 
            // pauseButton
            // 
            this.pauseButton.Enabled = false;
            this.pauseButton.Location = new System.Drawing.Point(3, 24);
            this.pauseButton.Name = "pauseButton";
            this.pauseButton.Size = new System.Drawing.Size(116, 23);
            this.pauseButton.TabIndex = 3;
            this.pauseButton.Text = "Pause";
            this.pauseButton.UseVisualStyleBackColor = true;
            this.pauseButton.Click += new System.EventHandler(this.PauseClick);
            // 
            // stepCycleButton
            // 
            this.stepCycleButton.Enabled = false;
            this.stepCycleButton.Location = new System.Drawing.Point(3, 83);
            this.stepCycleButton.Name = "stepCycleButton";
            this.stepCycleButton.Size = new System.Drawing.Size(116, 23);
            this.stepCycleButton.TabIndex = 2;
            this.stepCycleButton.Text = "Step";
            this.stepCycleButton.UseVisualStyleBackColor = true;
            this.stepCycleButton.Click += new System.EventHandler(this.StepCycleClick);
            // 
            // startPausedCheckbox
            // 
            this.startPausedCheckbox.AutoSize = true;
            this.startPausedCheckbox.Location = new System.Drawing.Point(3, 3);
            this.startPausedCheckbox.Name = "startPausedCheckbox";
            this.startPausedCheckbox.Size = new System.Drawing.Size(116, 19);
            this.startPausedCheckbox.TabIndex = 1;
            this.startPausedCheckbox.Text = "Break On Launch";
            this.startPausedCheckbox.UseVisualStyleBackColor = true;
            this.startPausedCheckbox.CheckedChanged += new System.EventHandler(this.BreakOnLaunchChange);
            // 
            // tabPage2
            // 
            this.tabPage2.Controls.Add(this.registerListView);
            this.tabPage2.Controls.Add(this.registerControlPanel);
            this.tabPage2.Location = new System.Drawing.Point(4, 24);
            this.tabPage2.Name = "tabPage2";
            this.tabPage2.Padding = new System.Windows.Forms.Padding(3);
            this.tabPage2.Size = new System.Drawing.Size(839, 425);
            this.tabPage2.TabIndex = 1;
            this.tabPage2.Text = "tabPage2";
            this.tabPage2.UseVisualStyleBackColor = true;
            // 
            // registerListView
            // 
            this.registerListView.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.registerListView.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.registerViewIndexColumn,
            this.registerViewValueColumn,
            this.registerViewContestionColumn});
            this.registerListView.GridLines = true;
            this.registerListView.Location = new System.Drawing.Point(299, 6);
            this.registerListView.Name = "registerListView";
            this.registerListView.Size = new System.Drawing.Size(534, 416);
            this.registerListView.TabIndex = 1;
            this.registerListView.UseCompatibleStateImageBehavior = false;
            this.registerListView.View = System.Windows.Forms.View.Details;
            // 
            // registerViewIndexColumn
            // 
            this.registerViewIndexColumn.Text = "Register Index";
            this.registerViewIndexColumn.Width = 100;
            // 
            // registerViewValueColumn
            // 
            this.registerViewValueColumn.Text = "Value";
            this.registerViewValueColumn.Width = 200;
            // 
            // registerViewContestionColumn
            // 
            this.registerViewContestionColumn.Text = "Contestion";
            this.registerViewContestionColumn.Width = 80;
            // 
            // registerControlPanel
            // 
            this.registerControlPanel.Controls.Add(this.splitContainer1);
            this.registerControlPanel.Location = new System.Drawing.Point(6, 6);
            this.registerControlPanel.Name = "registerControlPanel";
            this.registerControlPanel.Size = new System.Drawing.Size(287, 157);
            this.registerControlPanel.TabIndex = 0;
            // 
            // splitContainer1
            // 
            this.splitContainer1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.splitContainer1.Location = new System.Drawing.Point(0, 0);
            this.splitContainer1.Name = "splitContainer1";
            // 
            // splitContainer1.Panel1
            // 
            this.splitContainer1.Panel1.Controls.Add(this.displayFpLabel);
            this.splitContainer1.Panel1.Controls.Add(this.baseRegisterLabel);
            this.splitContainer1.Panel1.Controls.Add(this.replicationIndexLabel);
            this.splitContainer1.Panel1.Controls.Add(this.dispatchUnitLabel);
            this.splitContainer1.Panel1.Controls.Add(this.smLabel);
            // 
            // splitContainer1.Panel2
            // 
            this.splitContainer1.Panel2.Controls.Add(this.displayFpCheckBox);
            this.splitContainer1.Panel2.Controls.Add(this.baseRegisterDisplay);
            this.splitContainer1.Panel2.Controls.Add(this.replicationIndexSelector);
            this.splitContainer1.Panel2.Controls.Add(this.dispatchUnitSelector);
            this.splitContainer1.Panel2.Controls.Add(this.smSelector);
            this.splitContainer1.Size = new System.Drawing.Size(287, 157);
            this.splitContainer1.SplitterDistance = 154;
            this.splitContainer1.TabIndex = 0;
            // 
            // displayFpLabel
            // 
            this.displayFpLabel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.displayFpLabel.AutoSize = true;
            this.displayFpLabel.Location = new System.Drawing.Point(29, 120);
            this.displayFpLabel.Name = "displayFpLabel";
            this.displayFpLabel.Size = new System.Drawing.Size(122, 15);
            this.displayFpLabel.TabIndex = 4;
            this.displayFpLabel.Text = "Display Floating Point";
            // 
            // baseRegisterLabel
            // 
            this.baseRegisterLabel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.baseRegisterLabel.AutoSize = true;
            this.baseRegisterLabel.Location = new System.Drawing.Point(75, 93);
            this.baseRegisterLabel.Name = "baseRegisterLabel";
            this.baseRegisterLabel.Size = new System.Drawing.Size(76, 15);
            this.baseRegisterLabel.TabIndex = 3;
            this.baseRegisterLabel.Text = "Base Register";
            // 
            // replicationIndexLabel
            // 
            this.replicationIndexLabel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.replicationIndexLabel.AutoSize = true;
            this.replicationIndexLabel.Location = new System.Drawing.Point(53, 64);
            this.replicationIndexLabel.Name = "replicationIndexLabel";
            this.replicationIndexLabel.Size = new System.Drawing.Size(98, 15);
            this.replicationIndexLabel.TabIndex = 2;
            this.replicationIndexLabel.Text = "Replication Index";
            // 
            // dispatchUnitLabel
            // 
            this.dispatchUnitLabel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.dispatchUnitLabel.AutoSize = true;
            this.dispatchUnitLabel.Location = new System.Drawing.Point(73, 35);
            this.dispatchUnitLabel.Name = "dispatchUnitLabel";
            this.dispatchUnitLabel.Size = new System.Drawing.Size(78, 15);
            this.dispatchUnitLabel.TabIndex = 1;
            this.dispatchUnitLabel.Text = "Dispatch Unit";
            // 
            // smLabel
            // 
            this.smLabel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.smLabel.AutoSize = true;
            this.smLabel.Location = new System.Drawing.Point(8, 6);
            this.smLabel.Name = "smLabel";
            this.smLabel.Size = new System.Drawing.Size(143, 15);
            this.smLabel.TabIndex = 0;
            this.smLabel.Text = "Streaming Multiprocessor";
            // 
            // displayFpCheckBox
            // 
            this.displayFpCheckBox.AutoSize = true;
            this.displayFpCheckBox.Location = new System.Drawing.Point(3, 119);
            this.displayFpCheckBox.Name = "displayFpCheckBox";
            this.displayFpCheckBox.Size = new System.Drawing.Size(15, 14);
            this.displayFpCheckBox.TabIndex = 4;
            this.displayFpCheckBox.UseVisualStyleBackColor = true;
            this.displayFpCheckBox.CheckedChanged += new System.EventHandler(this.RegisterViewSelect);
            // 
            // baseRegisterDisplay
            // 
            this.baseRegisterDisplay.Enabled = false;
            this.baseRegisterDisplay.Location = new System.Drawing.Point(3, 90);
            this.baseRegisterDisplay.MaxLength = 16;
            this.baseRegisterDisplay.Name = "baseRegisterDisplay";
            this.baseRegisterDisplay.ReadOnly = true;
            this.baseRegisterDisplay.Size = new System.Drawing.Size(121, 23);
            this.baseRegisterDisplay.TabIndex = 3;
            // 
            // replicationIndexSelector
            // 
            this.replicationIndexSelector.FormattingEnabled = true;
            this.replicationIndexSelector.Items.AddRange(new object[] {
            "Replication 0",
            "Replication 1",
            "Replication 2",
            "Replication 3"});
            this.replicationIndexSelector.Location = new System.Drawing.Point(3, 61);
            this.replicationIndexSelector.Name = "replicationIndexSelector";
            this.replicationIndexSelector.Size = new System.Drawing.Size(121, 23);
            this.replicationIndexSelector.TabIndex = 2;
            this.replicationIndexSelector.SelectionChangeCommitted += new System.EventHandler(this.RegisterViewSelect);
            // 
            // dispatchUnitSelector
            // 
            this.dispatchUnitSelector.FormattingEnabled = true;
            this.dispatchUnitSelector.Items.AddRange(new object[] {
            "Dispatch 0",
            "Dispatch 1"});
            this.dispatchUnitSelector.Location = new System.Drawing.Point(3, 32);
            this.dispatchUnitSelector.Name = "dispatchUnitSelector";
            this.dispatchUnitSelector.Size = new System.Drawing.Size(121, 23);
            this.dispatchUnitSelector.TabIndex = 1;
            this.dispatchUnitSelector.SelectionChangeCommitted += new System.EventHandler(this.RegisterViewSelect);
            // 
            // smSelector
            // 
            this.smSelector.FormattingEnabled = true;
            this.smSelector.Items.AddRange(new object[] {
            "SM 0",
            "SM 1",
            "SM 2",
            "SM 3"});
            this.smSelector.Location = new System.Drawing.Point(3, 3);
            this.smSelector.Name = "smSelector";
            this.smSelector.Size = new System.Drawing.Size(121, 23);
            this.smSelector.TabIndex = 0;
            this.smSelector.SelectionChangeCommitted += new System.EventHandler(this.RegisterViewSelect);
            // 
            // DebuggerUI
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(7F, 15F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(877, 483);
            this.Controls.Add(this.panel1);
            this.KeyPreview = true;
            this.Name = "DebuggerUI";
            this.Text = "DebuggerUI";
            this.KeyUp += new System.Windows.Forms.KeyEventHandler(this.OnKeyUp);
            this.panel1.ResumeLayout(false);
            this.Registers.ResumeLayout(false);
            this.tabPage1.ResumeLayout(false);
            this.flowLayoutPanel1.ResumeLayout(false);
            this.panel2.ResumeLayout(false);
            this.panel2.PerformLayout();
            this.panel3.ResumeLayout(false);
            this.panel3.PerformLayout();
            this.tabPage2.ResumeLayout(false);
            this.registerControlPanel.ResumeLayout(false);
            this.splitContainer1.Panel1.ResumeLayout(false);
            this.splitContainer1.Panel1.PerformLayout();
            this.splitContainer1.Panel2.ResumeLayout(false);
            this.splitContainer1.Panel2.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.splitContainer1)).EndInit();
            this.splitContainer1.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private Panel panel1;
        private TabControl Registers;
        private TabPage tabPage1;
        private TabPage tabPage2;
        private Panel panel2;
        private TextBox clockCycleDisplay;
        private Label clockCycleLabel;
        private FlowLayoutPanel flowLayoutPanel1;
        private Panel panel3;
        private CheckBox startPausedCheckbox;
        private Button stepCycleButton;
        private Button pauseButton;
        private Button resumeButton;
        private Panel registerControlPanel;
        private SplitContainer splitContainer1;
        private ComboBox replicationIndexSelector;
        private ComboBox dispatchUnitSelector;
        private ComboBox smSelector;
        private Label smLabel;
        private Label dispatchUnitLabel;
        private Label replicationIndexLabel;
        private Label baseRegisterLabel;
        private TextBox baseRegisterDisplay;
        private ListView registerListView;
        private ColumnHeader registerViewIndexColumn;
        private ColumnHeader registerViewValueColumn;
        private Label displayFpLabel;
        private CheckBox displayFpCheckBox;
        private ColumnHeader registerViewContestionColumn;
    }
}