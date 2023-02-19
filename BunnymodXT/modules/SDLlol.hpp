#pragma once

#include "../sptlib-wrapper.hpp"
#include <SPTLib/IHookableNameFilter.hpp>

#define SDL_BUTTON(X)       (1 << ((X)-1))
#define SDL_BUTTON_LEFT     1
#define SDL_BUTTON_MIDDLE   2
#define SDL_BUTTON_RIGHT    3
#define SDL_BUTTON_X1       4
#define SDL_BUTTON_X2       5

typedef void* HWND;
class SDLlol : public IHookableNameFilter
{
	HOOK_DECL(int, __cdecl, SDL_WaitEventTimeout, void *event, int time)
	HOOK_DECL(void, __cdecl, SDL_WarpMouseInWindow, void *window, int x, int y)
	HOOK_DECL(void, __cdecl, SDL_GetWindowSize, void *window, int *x, int *y)
	//HOOK_DECL(void*, __cdecl, SDL_GL_CreateContext, void *window)

public:
	static SDLlol& GetInstance()
	{
		static SDLlol instance;
		return instance;
	}

	virtual void Hook(const std::wstring& moduleName, void* moduleHandle, void* moduleBase, size_t moduleLength, bool needToIntercept);
	virtual void Unhook();
	virtual void Clear();

	bool Found() const;
	void SetRelativeMouseMode(bool relative) const;
	uint32_t GetMouseState(int *x, int *y) const;
	HWND currentContext;
	bool started = false;

private:
	SDLlol() : IHookableNameFilter({ L"SDL2.dll", L"libSDL2.so", L"libSDL2-2.0.so.0" }) {};
	SDLlol(const SDLlol&);
	void operator=(const SDLlol&);

protected:
	typedef int(__cdecl *_SDL_SetRelativeMouseMode) (int enabled);
	_SDL_SetRelativeMouseMode ORIG_SDL_SetRelativeMouseMode;
	typedef uint32_t(__cdecl *_SDL_GetMouseState) (int *x, int *y);
	_SDL_GetMouseState ORIG_SDL_GetMouseState;
	typedef HWND(__cdecl *_SDL_GL_GetCurrentContext) ();
	_SDL_GL_GetCurrentContext ORIG_SDL_GL_GetCurrentContext;


	/*typedef HWND(__cdecl SDL_GL_CreateContext) (HWND *window);
	_SDL_GL_CreateContext ORIG_SDL_GL_CreateContext;*/
};
