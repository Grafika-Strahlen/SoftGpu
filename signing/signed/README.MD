# Loading the DLL

To load the DLL first change the owner to `NT Service\TrustedInstaller`. Then run `vboxmanage setextradata GpuTester VBoxInternal/PDM/Devices/SoftGpu/Path D:\GPU\SoftGpu\signing\signed\VirtualBoxDevice.dll`, replacing the path to this current DLL, also replace `GpuTester` with the name of your VirtualBox image.

You may also want to set the environment variables `VBOX_LOG` and `VBOX_LOG_RELEASE` to `+pdm*.e.l.f +dev*.e.l.f +misc.e.l.f`.

You'll also need to run `vboxmanage setextradata GpuTester VBoxInternal/Devices/softgpu/0/Priority 1` and `vboxmanage setextradata GpuTester VBoxInternal/Devices/softgpu/0/Trusted 1`.