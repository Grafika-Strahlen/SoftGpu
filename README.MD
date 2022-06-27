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
         |  .-------------------------------------.
         +--| Dispatch Unit (Not implemented yet) |
         |  *-------------------------------------*
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
         |  .----------------------------------------.
         +--| Int/FP Core 0..7 (Not Implemented Yet) |
         |  *--+-------------------------------------*
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





