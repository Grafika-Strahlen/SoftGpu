/**
 * @file
 *
 * Copyright (c) 2025. Grafika Strahlen LLC
 * All rights reserved.
 */
#include <ConPrinter.hpp>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_init.h>

namespace tau::vd {

bool InitSdl() noexcept
{
    SDL_SetAppMetadata("SoftGPU", "1.0.0", "dev.grafikastrahlen.softgpu");

    if(!SDL_Init(SDL_INIT_VIDEO))
    {
        ConPrinter::PrintLn("Error on SDL init: {}", SDL_GetError());
        return false;
    }

    return true;
}

void CleanupSdl() noexcept
{
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
    SDL_Quit();
}

static ::std::atomic<bool> s_ShouldClose = false;

void PollEvents() noexcept
{
    SDL_Event event;
    while(SDL_PollEvent(&event))
    {
        switch(event.type)
        {
            case SDL_EVENT_QUIT:
            case SDL_EVENT_TERMINATING:
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                s_ShouldClose = true;
                break;
            case SDL_EVENT_KEY_UP:
                if(event.key.key == SDLK_ESCAPE)
                {
                    s_ShouldClose = true;
                }
                break;
            default:
                break;
        }
    }
}

bool ShouldClose() noexcept
{
    return s_ShouldClose;
}

}
