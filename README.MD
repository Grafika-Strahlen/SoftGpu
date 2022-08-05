# Software GPU Emulation

This emulates the hardware of a GPU entirely in software. To make it easier to convert to an HDL later some design considerations were taken.

* Each class represents an entity.
* Entities can contain other entities.
  * These child entities are stored as a value. Thus the top level entity would have within its every single child entity, entities are not dynamically allocated.
  * Entities can receive in their constructor a pointer to the parent entity, and an ID.
    * This is to model hardware port mapping.
    * If an entity needs to communicate with a sibling, it must go through the parent.
      * This is done using functions, as opposed to navigating the object tree.
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

BAR 0&1 is a 64  bit BAR to access the main graphics memory.

BAR 2&4 is a 64 bit BAR to access the device registers.

### Registers

| Address | Name      | Access | Description                                                  |
| ------- | --------- | ------ | ------------------------------------------------------------ |
| 0x0000  | Magic     | R      | A 32 bit magic indicator of the GPU. This should always return 0x4879666C. |
| 0x0004  | Revision  | R      | A 32 bit value indicating the revision of the chip.          |
| 0x0008  | Emulation | R      | A 32 bit value indicating if this is emulated. 0 = Microprocessor, 1 = FPGA, 2 = Emulation |
| 0x000C  | Reset     | R      | Performing a 32 bit read on this register will cause the GPU to reset to the default state. |
| 0x0010  | Control   | RW     | Bit 0: Interrupt Enable.<br />Bits 1 - 31: Reserved.         |



## Current Layout Plan

```
.-----------.
| Processor |
*--+--------*
   |
   |  .----------------.
   +--| Memory Manager |
   |  *--+-------------*
   |     |
   |     |  .--------------------------------.
   |     +--| L1 Cache (Not Implemented Yet) |
   |     |  *--------------------------------*
   |     |
   |     |  .---------------.
   |     \--| L0 Cache 0..3 |
   |        *---------------*
   |
   |  .-------------------------------.
   \--| Streaming Multiprocessor 0..3 |
      *--+----------------------------*
         |
         |  .---------------.
         +--| Register File |
         |  *---------------*
         |
         |  .-----------------------------------------.
         +--| Instruction Cache (Not implemented yet) |
         |  *-----------------------------------------*
         |
         |  .-------------------------------------.
         +--| Texture Cache (Not implemented yet) |
         |  *-------------------------------------*
         |
         |  .---------------.
         +--| Dispatch Unit |
         |  *---------------*
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





