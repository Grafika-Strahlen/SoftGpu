# VirtualBox Device Loader

This is a loader that disables hardening to load in an unsigned device.

The loader will export the `VBoxDevicesRegister` and `VBoxDriversRegister` functions, which in turn load a real device and proxy the function calls.

## Loading Process

1. Load *ntdll.dll*.
2. Get the function pointers for `LdrLoadDll`, `NtCreateSection`, and `NtProtectVirtualMemory`.
3. Use `NtProtectVirtualMemory` to enable read-write-execution protection for the prolog of `LdrLoadDll` and `NtCreateSection`.
4. Copy the first 16 bytes of `LdrLoadDll` and `NtCreateSection`. These should contain the redirect to VirtualBox's hardening code.
5. Replace the first 16 bytes of `LdrLoadDll` and `NtCreateSection` with the original bytes I found in those DLLs on my computer.
6. Use `NtProtectVirtualMemory` to restore the original protections for the prologs of `LdrLoadDll` and `NtCreateSection`.
7. Flush the instruction caches for `LdrLoadDll` and `NtCreateSection`.
8. Query an environment variable to get the path to the real device or driver.
9. Load the real device or driver DLL.
10. Perform operation 3.
11. Restore the first 16 bytes of `LdrLoadDll` and `NtCreateSection` to VirtualBox's hardening thunks. 
12. Perform operations 5..7.
13. Load and call the real register function from the loaded DLL.

## Environment Variables

* *TAU_VBOX_DEVICE* - The path to an unsigned (or signed if you really want) device DLL.
* *TAU_VBOX_DRIVER* - The path to an unsigned (or signed if you really want) driver DLL.

These can be the same DLL.

## Rapid Development

The whole point of this loader is to save time that is spent signing the drivers to get VirtualBox to load them. Conveniently, for development of this loader, you can use this same loader. You will have to come up with new environment variable names though.

## Usage

To use this you will need to sign the resultant DLL with an EV Code Signing Certificate and get it [Attestation signed by Microsoft](https://learn.microsoft.com/en-us/windows-hardware/drivers/dashboard/code-signing-attestation).

You will need to disable Driver Signature Enforcement in Windows to use load in the unsigned DLLs. You can do this with an admin Command Prompt and the following command:

```bash
bcdedit /set {bootmgr} displaybootmenu yes
```

Then restart your computer (don't use shutdown, fast boot is enabled by default and a restart actually performs a true full shutdown), when the screen appears to select Windows press F8, then 7. This will only persist for the current boot session (sort of, due to fast boot, if you shutdown and boot your computer it will actually persist).

Follow the information in /signing/signed/README.MD for information on how to load this into a VirtualBox VM.