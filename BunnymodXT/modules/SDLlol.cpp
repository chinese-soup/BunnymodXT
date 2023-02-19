#include "../stdafx.hpp"

#include "../sptlib-wrapper.hpp"
#include <SPTLib/MemUtils.hpp>
#include "SDLlol.hpp"
#include "HwDLL.hpp"


#ifndef _WIN32
extern "C" int __cdecl SDL_WaitEventTimeout(void *event, int time)
{
	return SDLlol::HOOKED_SDL_WaitEventTimeout(event, time);
}
/*extern "C" void __cdecl SDL_CreateWindow(const char *title, int x, int y, int w, int h, u_int32 flags)
{
	return SDLlol::HOOKED_SDL_CreateWindow(title, x, y, w, h, flags);
}*/
/*extern "C" void __cdecl SDL_GetWindowSize(void *window, int*w, int*h)
{
	return SDLlol::HOOKED_SDL_GetWindowSize(window, w, h);
}*/
/*extern "C" void* __cdecl SDL_GL_CreateContext(void *window)
{
	return SDLlol::HOOKED_SDL_GL_CreateContext(window);
}*/

#endif


void SDLlol::Hook(const std::wstring& moduleName, void* moduleHandle, void* moduleBase, size_t moduleLength, bool needToIntercept)
{
	Clear(); // Just in case.

	m_Handle = moduleHandle;
	m_Base = moduleBase;
	m_Length = moduleLength;
	m_Name = moduleName;
	m_Intercepted = needToIntercept;

	ORIG_SDL_SetRelativeMouseMode = reinterpret_cast<_SDL_SetRelativeMouseMode>(MemUtils::GetSymbolAddress(m_Handle, "SDL_SetRelativeMouseMode"));
	if (ORIG_SDL_SetRelativeMouseMode) {
		EngineDevMsg("[sdl] Found SDL_SetRelativeMouseMode at %p.\n", ORIG_SDL_SetRelativeMouseMode);
	} else {
		EngineDevWarning("[sdl] Could not find SDL_SetRelativeMouseMode.\n");
	}

	ORIG_SDL_GetMouseState = reinterpret_cast<_SDL_GetMouseState>(MemUtils::GetSymbolAddress(m_Handle, "SDL_GetMouseState"));
	if (ORIG_SDL_GetMouseState) {
		EngineDevMsg("[sdl] Found SDL_GetMouseState at %p.\n", ORIG_SDL_GetMouseState);
	} else {
		EngineDevWarning("[sdl] Could not find SDL_GetMouseState.\n");
	}

	ORIG_SDL_WaitEventTimeout = reinterpret_cast<_SDL_WaitEventTimeout>(MemUtils::GetSymbolAddress(m_Handle, "SDL_WaitEventTimeout"));
	if (ORIG_SDL_WaitEventTimeout) {
		EngineDevMsg("[sdl] Found SDL_WaitEventTimeout at %p.\n", ORIG_SDL_WaitEventTimeout);
	} else {
		EngineDevWarning("[sdl] Could not find SDL_WaitEventTimeout.\n");
	}

	ORIG_SDL_GetWindowSize = reinterpret_cast<_SDL_GetWindowSize>(MemUtils::GetSymbolAddress(m_Handle, "SDL_GetWindowSize"));
	if (ORIG_SDL_GetWindowSize) {
		EngineDevMsg("[sdl] Found ORIG_SDL_GetWindowSize at %p.\n", ORIG_SDL_GetWindowSize);
	} else {
		EngineDevWarning("[sdl] Could not find ORIG_SDL_GetWindowSize.\n");
	}

	ORIG_SDL_GetWindowSize = reinterpret_cast<_SDL_GetWindowSize>(MemUtils::GetSymbolAddress(m_Handle, "SDL_GetWindowSize"));
	if (ORIG_SDL_GetWindowSize) {
		EngineDevMsg("[sdl] Found ORIG_SDL_GetWindowSize at %p.\n", ORIG_SDL_GetWindowSize);
	} else {
		EngineDevWarning("[sdl] Could not find ORIG_SDL_GetWindowSize.\n");
	}

	ORIG_SDL_GL_GetCurrentContext = reinterpret_cast<_SDL_GL_GetCurrentContext>(MemUtils::GetSymbolAddress(m_Handle, "SDL_GL_GetCurrentContext"));
	if (ORIG_SDL_GL_GetCurrentContext) {
		EngineDevMsg("[sdl] Found ORIG_SDL_GL_GetCurrentContext at %p.\n", ORIG_SDL_GL_GetCurrentContext);
	} else {
		EngineDevWarning("[sdl] Could not find ORIG_SDL_GL_GetCurrentContext.\n");
	}
//
//	ORIG_SDL_GL_CreateContext = reinterpret_cast<_SDL_GL_CreateContext>(MemUtils::GetSymbolAddress(m_Handle, "ORIG_SDL_GL_CreateContext"));
//	if (ORIG_SDL_GL_CreateContext) {
//		EngineDevMsg("[sdl] Found ORIG_SDL_GL_CreateContext at %p.\n", ORIG_SDL_GL_CreateContext);
//	} else {
//		EngineDevWarning("[sdl] Could not find ORIG_SDL_GL_CreateContext.\n");
//	}
//

	/*ORIG_SDL_GetWindowPosition = reinterpret_cast<_SDL_GetWindowPosition>(MemUtils::GetSymbolAddress(m_Handle, "SDL_GetWindowPosition"));
	if (ORIG_SDL_GetWindowPosition) {
		EngineDevMsg("[sdl] Found SDL_CreateWindow at %p.\n", ORIG_SDL_GetWindowPosition);
	} else {
		EngineDevWarning("[sdl] Could not find SDL_GetWindowPosition.\n");
	}*/
}

void SDLlol::Unhook()
{
	Clear();
}

void SDLlol::Clear()
{
	IHookableNameFilter::Clear();
	ORIG_SDL_SetRelativeMouseMode = nullptr;
	ORIG_SDL_GetMouseState = nullptr;
	ORIG_SDL_WaitEventTimeout = nullptr;
	ORIG_SDL_GL_GetCurrentContext = nullptr;
}


bool SDLlol::Found() const
{
	return ORIG_SDL_SetRelativeMouseMode != nullptr && ORIG_SDL_GetMouseState != nullptr;
}

void SDLlol::SetRelativeMouseMode(bool relative) const
{
	if (ORIG_SDL_SetRelativeMouseMode != nullptr)
		ORIG_SDL_SetRelativeMouseMode(relative ? 1 : 0);
}

uint32_t SDLlol::GetMouseState(int *x, int *y) const
{
	if (ORIG_SDL_GetMouseState != nullptr)
		return ORIG_SDL_GetMouseState(x, y);

	*x = 0;
	*y = 0;
	return 0;
}

HOOK_DEF_2(SDLlol, int, __cdecl, SDL_WaitEventTimeout, void*, event, int, time)
{
	currentContext = ORIG_SDL_GL_GetCurrentContext();
	return ORIG_SDL_WaitEventTimeout(event, 0);
}

HOOK_DEF_3(SDLlol, void, __cdecl, SDL_GetWindowSize, void*, window, int*, w, int*, h)
{
	//EngineDevMsg("LOL %d %d\n", w, h);
	//*w = 1280;
	//*h = 720;

	ORIG_SDL_GetWindowSize(window, w, h);

}


/*HOOK_DEF_1(SDLlol, void*, __cdecl, SDL_GL_CreateContext, void*, window)
{
	EngineDevMsg("CONTEXT CREATING!");
	//void *glcontext = ORIG_SDL_GL_CreateContext(window);
	//return glcontext;
}*/