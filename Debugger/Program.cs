namespace Debugger
{
    internal static class Program
    {
        /// <summary>
        ///  The main entry point for the application.
        /// </summary>
        [STAThread]
        static void Main()
        {
            CommManager commManager = new CommManager("gpu-pipe-step", "gpu-pipe-info");
            GpuDataManager dataManager = new GpuDataManager(commManager);

            // To customize application configuration such as set high DPI settings or default font,
            // see https://aka.ms/applicationconfiguration.
            ApplicationConfiguration.Initialize();
            Application.Run(new DebuggerUI(commManager, dataManager));
        }
    }
}