# Software GPU Emulation

This emulates the hardware of a GPU entirely in software. To make it easier to convert to an HDL later some design considerations were taken.

* Each class represents an entity.
* Entities can contain other entities.
  * These child entities are stored as a value. Thus the top level entity would have within its every single child entity, entities are not dynamically allocated.
  * Entities can receive in their constructor a pointer to the parent entity, and an ID.
    * This is to model hardware port mapping.
    * If an entity needs to communicate with a sibling, it must go through the parent.
      * This is done using bus functions, as opposed to navigating the object tree. The bus function simply returns a reference to the bus object that is stored within the entity. This pretty much lines up directly with how hardware entities communicate. Pointers are allowed in the bus object to return data back to the caller (much like binding a signal to a port in VHDL), but there is typically some other mechanism to identify when the operation has actually completed.
    * The ID is sometimes used when calling the parent so that the parent can identify the call location. 
      * This is particularly useful during broadcast operations.
      * This would map differently to hardware, but is a simple enough construct that the hardware can be easily designed to take whatever form is necessary.
* Entities receive a clock signal.
  * This signal can be passed to child entities.
  * This signal is either used to implement an FSM or to add artificial delays to emulate long running hardware (like binary64 divide or SFU operations).
* Everything is done synchronously.
  * While hardware is an inherently asynchronous operation, each phase of a clock cycle is generally done in lock-step.
    * To account for the asynchronous nature of hardware, some entities can have multi clock cycle delays before returning data.
* Data is passed in packed structures.
  * Structs can have padding bits for alignment with x86, but these can easily be removed in the hardware.
  * Only the minimum number of bits to represent a unit of data are used.

## PCI Spec

BAR 0 is a 32 bit BAR to access the device registers. This register is not prefetchable.

BAR 1&2 is a 64  bit BAR to access the main graphics memory. This register is prefetchable.

Supports PCIe Capability. The capability structure is at 0x48.

Supports Power Management Capability. The structure is at 0x5C.

Supports Message Signalled Interrupts Capability. The structure is at 0x64. Only supports a single interrupt.

Supports Advanced Error Reporting Extended Capability. The structure is at 0x100.

### Registers

| Address | Name                          | Access | Description                                                  |
| ------- | ----------------------------- | ------ | ------------------------------------------------------------ |
| 0x0000  | Magic                         | R      | A 32 bit magic indicator of the GPU. This should always return 0x4879666C. |
| 0x0004  | Revision                      | R      | A 32 bit value indicating the revision of the chip.          |
| 0x0008  | Emulation                     | R      | A 32 bit value indicating if this is emulated. 0 = Microprocessor, 1 = FPGA, 2 = Simulation |
| 0x000C  | Reset                         | R      | Performing a 32 bit read on this register will cause the GPU to reset to the default state. |
| 0x0010  | Control                       | RW     | Bit 0: Interrupt Enable.<br />Bits 1 - 31: Reserved.         |
| 0x0014  | VRAM Size Low                 | R      | The low 32 bits of the VRAM Size.                            |
| 0x0018  | VRAM Size High                | R      | The high 32 bits of the VRAM Size.                           |
| 0x001C  | Interrupt Type                | RW     | We only support 1 MSI interrupt. Thus the driver has to read from the register to learn what the interrupt actually is. Writing to this register will reset the value to 0. |
| 0x1014  | VGA Width                     | R      | Used to store the width of the virtual monitor. Don't use, the Display Info registers now fill this purpose. |
| 0x1018  | VGA Height                    | R      | Used to store the height of the virtual monitor. Don't use, the Display Info registers now fill this purpose. |
|         |                               |        |                                                              |
| 0x2000  | Display Information (DI) Base | N/A    | This is the base register for Display Information. There are currently 8 displays, and 7 registers per display. |
| +0x00   | DI Width                      | RW     | Gets and sets the width of the specified monitor. It is recommended to disable the monitor before making changes. |
| +0x04   | DI Height                     | RW     | Gets and sets the height of the specified monitor. It is recommended to disable the monitor before making changes. |
| +0x08   | DI Bits Per Pixel             | RW     | Gets and sets the BPP of the specified monitor. In practice this doesn't do anything, and needs to be replaced with a color format specifier. It is recommended to disable the monitor before making changes. |
| +0x0C   | DI Enable                     | RW     | Enables or disables the specified monitor. Disabling a monitor will currently set it to display black, but will not clear the framebuffer. |
| +0x10   | DI Refresh Rate Numerator     | RW     | Gets and sets the refresh-rate numerator of the specified monitor. The refresh-rate is calculated by dividing this by the refresh-rate denominator. It is recommended to disable the monitor before making changes. |
| +0x14   | DI Refresh Rate Denominator   | RW     | Gets and sets the refresh-rate denominator of the specified monitor. The refresh-rate is calculated by dividing the refresh-rate numerator by this. It is recommended to disable the monitor before making changes. |
| +0x18   | DI VSync Enable               | RW     | Gets and sets the VSync interrupt state. This doesn't actually do what one would typically consider VSync, but rather relates sending interrupts at the end of a frame, this has to do with CRTCs. |
|         |                               |        |                                                              |
| 0x3000  | EDID Base                     | R      | This is the base register for EDID information. There are currently 8 displays, and the EDID information is 128 bytes. |
|         |                               |        |                                                              |
| 0x8000  | Debug Print                   | W      | Writing to this register will output data to the console. You can technically read from it, it just returns 0. |



## Current Layout Plan

```
.-----------.
| Processor |
*--+--------*
   |
   |  .------------------.
   +--| Cache Controller |
   |  *--+---------------*
   |     |
   |     |  .--------------------------------.
   |     +--| L1 Cache (Not Implemented Yet) |
   |     |  *--------------------------------*
   |     |
   |     |  .---------------.
   |     \--| L0 Cache 0..3 |
   |        *---------------*
   |
   |  .----------------.
   +--| PCI Controller |
   |  *----------------*
   |
   |  .----------------.
   +--| ROM Controller |
   |  *----------------*
   |
   |  .-----------------------.
   +--| PCI Control Registers |
   |  *-----------------------*
   |
   |  .-----------------.
   +--| Display Manager |
   |  *-----------------*
   |
   |  .-------------------------------.
   \--| Streaming Multiprocessor 0..3 |
      *--+----------------------------*
         |
         |  .---------------------------------.
         +--| Memory Management Unit (Paging) |
         |  *---------------------------------*
         |
         |  .---------------.
         +--| Register File |
         |  *---------------*
         |
         |  .-----------------------------------------.
         +--| Instruction Cache (Not Implemented Yet) |
         |  *-----------------------------------------*
         |
         |  .-------------------------------------.
         +--| Texture Cache (Not Implemented Yet) |
         |  *-------------------------------------*
         |
         |  .--------------------.
         +--| Dispatch Unit 0..1 |
         |  *--------------------*
         |
         |  .-----------------.
         +--| Load/Store 0..3 |
         |  *-----------------*
         |
         |  .--------------.
         +--| FP Core 0..7 |
         |  *--+-----------*
         |     |
         |     |  .-----.
         |     \--| FPU |
         |        *-----*
         |
         |  .------------------.
         +--| Int/FP Core 0..7 |
         |  *--+---------------*
         |	   |
         |	   |  .-----.
         |     +--| FPU |
         |     |  *-----*
         |     |
         |     |  .---------------------------.
         |     \--| ALU (Not Implemented Yet) |
         |        *---------------------------*
         |
         |  .--------------------------------.
         +--| SFU 0..3 (Not Implemented Yet) |
         |  *--------------------------------*
         |
         |  .--------------------------------------------.
         +--| Texture Sampler 0..1 (Not Implemented Yet) |
         |  *--------------------------------------------*
         |  
         |  .----------------------------------.
         +--| Rasterizer (Not Implemented Yet) |
         |  *----------------------------------*
         |  
         |  .------------------------------------.
         +--| Vertex Fetch (Not Implemented Yet) |
         |  *------------------------------------*
         |  
         |  .-----------------------------------.
         \--| Tessellator (Not Implemented Yet) |
            *-----------------------------------*
```

## Virtual Box Device

The GPU can be simulated in a VM using VirtualBox. VirtualBox apparently emulates hardware for VM using a rich device emulation API. That API is capable of, among other things handling config reads and writes through custom functions, handling memory reads and writes (such as for MMIO) through custom functions, and sending interrupts to the guest. This is perfect for having a software device appear as real hardware to an operating system.

The code in [VBDevice.cpp](https://github.com/hyfloac/SoftGpu/blob/main/VirtualBoxDevice/src/VBDevice.cpp) implements nearly all of the necessary elements for simulating the GPU. The GPU is hosted on a separate thread, so that it can act like a separate device with its own internal clock. When the guest performs MMIO operations they're intercepted by VirtualBox and sent to our handler, which will then lock on a mutex, write the request data, then wait for an event to signal that the request has been processed. 

When writing to BAR 1, this step is skipped, as BAR 1 operations just end up with a simple memcpy to some memory. This improves performance for framebuffer writes, but ultimately will not work when virtual memory management is enabled to support [GpuMmu](https://learn.microsoft.com/en-us/windows-hardware/drivers/display/gpummu-model).

There is an additional thread which runs the Vulkan window. This thread will periodically copy the memory inside the GPU's framebuffer, into a staging buffer, which is then copied to a real framebuffer and displayed. It would be possible to do this with VirtualBox's driver API, but this was quicker to get working. It also has the added benefit of letting you close the window without killing the entire VM.

