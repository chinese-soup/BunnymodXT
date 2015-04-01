#include "../stdafx.hpp"

#include "../sptlib-wrapper.hpp"
#include <SPTLib/MemUtils.hpp>
#include <SPTLib/Hooks.hpp>
#include "ServerDLL.hpp"
#include "ClientDLL.hpp"
#include "HwDLL.hpp"
#include "../patterns.hpp"
#include "../cvars.hpp"
#include "../hud_custom.hpp"

// Linux hooks.
#ifndef _WIN32
extern "C" void __cdecl _Z8CmdStartPK7edict_sPK9usercmd_sj(const edict_t* player, const usercmd_t* cmd, unsigned int random_seed)
{
	return ServerDLL::HOOKED_CmdStart(player, cmd, random_seed);
}

extern "C" void __cdecl _ZN10CNihilanth10DyingThinkEv(void* thisptr)
{
	return ServerDLL::HOOKED_CNihilanth__DyingThink_Linux(thisptr);
}

extern "C" void __cdecl _ZN11COFGeneWorm10DyingThinkEv(void* thisptr)
{
	return ServerDLL::HOOKED_COFGeneWorm__DyingThink_Linux(thisptr);
}
#endif

void ServerDLL::Hook(const std::wstring& moduleName, void* moduleHandle, void* moduleBase, size_t moduleLength, bool needToIntercept)
{
	Clear(); // Just in case.

	m_Handle = moduleHandle;
	m_Base = moduleBase;
	m_Length = moduleLength;
	m_Name = moduleName;
	m_Intercepted = needToIntercept;

	FindStuff();
	RegisterCVarsAndCommands();

	if (needToIntercept)
		MemUtils::Intercept(moduleName, {
			{ reinterpret_cast<void**>(&ORIG_PM_Jump), reinterpret_cast<void*>(HOOKED_PM_Jump) },
			{ reinterpret_cast<void**>(&ORIG_PM_PreventMegaBunnyJumping), reinterpret_cast<void*>(HOOKED_PM_PreventMegaBunnyJumping) },
			{ reinterpret_cast<void**>(&ORIG_PM_PlayerMove), reinterpret_cast<void*>(HOOKED_PM_PlayerMove) },
			{ reinterpret_cast<void**>(&ORIG_CmdStart), reinterpret_cast<void*>(HOOKED_CmdStart) },
			{ reinterpret_cast<void**>(&ORIG_CNihilanth__DyingThink), reinterpret_cast<void*>(HOOKED_CNihilanth__DyingThink) },
			{ reinterpret_cast<void**>(&ORIG_COFGeneWorm__DyingThink), reinterpret_cast<void*>(HOOKED_COFGeneWorm__DyingThink) },
			{ reinterpret_cast<void**>(&ORIG_CMultiManager__ManagerUse), reinterpret_cast<void*>(HOOKED_CMultiManager__ManagerUse) }
		});
}

void ServerDLL::Unhook()
{
	if (m_Intercepted)
		MemUtils::RemoveInterception(m_Name, {
			{ reinterpret_cast<void**>(&ORIG_PM_Jump), reinterpret_cast<void*>(HOOKED_PM_Jump) },
			{ reinterpret_cast<void**>(&ORIG_PM_PreventMegaBunnyJumping), reinterpret_cast<void*>(HOOKED_PM_PreventMegaBunnyJumping) },
			{ reinterpret_cast<void**>(&ORIG_PM_PlayerMove), reinterpret_cast<void*>(HOOKED_PM_PlayerMove) },
			{ reinterpret_cast<void**>(&ORIG_CmdStart), reinterpret_cast<void*>(HOOKED_CmdStart) },
			{ reinterpret_cast<void**>(&ORIG_CNihilanth__DyingThink), reinterpret_cast<void*>(HOOKED_CNihilanth__DyingThink) },
			{ reinterpret_cast<void**>(&ORIG_COFGeneWorm__DyingThink), reinterpret_cast<void*>(HOOKED_COFGeneWorm__DyingThink) },
			{ reinterpret_cast<void**>(&ORIG_CMultiManager__ManagerUse), reinterpret_cast<void*>(HOOKED_CMultiManager__ManagerUse) }
		});

	Clear();
}

void ServerDLL::Clear()
{
	IHookableDirFilter::Clear();
	ORIG_PM_Jump = nullptr;
	ORIG_PM_PreventMegaBunnyJumping = nullptr;
	ORIG_PM_PlayerMove = nullptr;
	ORIG_PM_ClipVelocity = nullptr;
	ORIG_CmdStart = nullptr;
	ORIG_CNihilanth__DyingThink = nullptr;
	ORIG_CNihilanth__DyingThink_Linux = nullptr;
	ORIG_COFGeneWorm__DyingThink = nullptr;
	ORIG_COFGeneWorm__DyingThink_Linux = nullptr;
	ORIG_CMultiManager__ManagerUse = nullptr;
	ORIG_GetEntityAPI = nullptr;
	ppmove = nullptr;
	offPlayerIndex = 0;
	offOldbuttons = 0;
	offOnground = 0;
	offVelocity = 0;
	offOrigin = 0;
	offAngles = 0;
	offCmd = 0;
	offBhopcap = 0;
	memset(originalBhopcapInsn, 0, sizeof(originalBhopcapInsn));
	pEngfuncs = nullptr;
	ppGlobals = nullptr;
	cantJumpNextTime.clear();
	m_Intercepted = false;
}

bool ServerDLL::CanHook(const std::wstring& moduleFullName)
{
	if (!IHookableDirFilter::CanHook(moduleFullName))
		return false;

	// Filter out addons like metamod which may be located into a "dlls" folder under addons.
	std::wstring pathToLiblist = moduleFullName.substr(0, moduleFullName.rfind(GetFolderName(moduleFullName))).append(L"liblist.gam");

	// If liblist.gam exists in the parent directory, then we're (hopefully) good.
	struct wrapper {
		wrapper(FILE* f) : file(f) {};
		~wrapper() {
			if (file)
				fclose(file);
		}
		operator FILE*() const
		{
			return file;
		}

		FILE* file;
	} liblist(fopen(Convert(pathToLiblist).c_str(), "r"));

	if (liblist)
		return true;

	return false;
}

void ServerDLL::FindStuff()
{
	auto fPM_PreventMegaBunnyJumping = MemUtils::Find(reinterpret_cast<void**>(&ORIG_PM_PreventMegaBunnyJumping), m_Handle, "PM_PreventMegaBunnyJumping", m_Base, m_Length, Patterns::ptnsPMPreventMegaBunnyJumping,
		[](MemUtils::ptnvec_size ptnNumber) { }, []() { }
	);

	auto fPM_PlayerMove = MemUtils::Find(reinterpret_cast<void**>(&ORIG_PM_PlayerMove), m_Handle, "PM_PlayerMove", m_Base, m_Length, Patterns::ptnsPMPlayerMove,
		[&](MemUtils::ptnvec_size ptnNumber) {
			offPlayerIndex = 0;
			offVelocity = 92;
			offOrigin = 56;
			offAngles = 68;
			offCmd = 283736;
		}, []() { }
	);

	auto fPM_Jump = MemUtils::Find(reinterpret_cast<void**>(&ORIG_PM_Jump), m_Handle, "PM_Jump", m_Base, m_Length, Patterns::ptnsPMJump,
		[&](MemUtils::ptnvec_size ptnNumber) {
			offPlayerIndex = 0;
			offOldbuttons = 200;
			offOnground = 224;
			if (ptnNumber == MemUtils::INVALID_SEQUENCE_INDEX) // Linux.
			{
				ppmove = *reinterpret_cast<void***>(reinterpret_cast<uintptr_t>(ORIG_PM_Jump) + 1);
				void *bhopcapAddr;
				auto n = MemUtils::FindUniqueSequence(m_Base, m_Length, Patterns::ptnsBhopcap, &bhopcapAddr);
				if (n != MemUtils::INVALID_SEQUENCE_INDEX)
				{
					offBhopcap = reinterpret_cast<ptrdiff_t>(bhopcapAddr) - reinterpret_cast<ptrdiff_t>(ORIG_PM_Jump) + 27;
					memcpy(originalBhopcapInsn, reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(bhopcapAddr) + 27), sizeof(originalBhopcapInsn));
				}
			}
			else
			{
				switch (ptnNumber)
				{
				case 0:
				case 1:
					ppmove = *reinterpret_cast<void***>(reinterpret_cast<uintptr_t>(ORIG_PM_Jump) + 2);
					break;

				case 2:
				case 3: // AG-Client, shouldn't happen here but who knows.
					ppmove = *reinterpret_cast<void***>(reinterpret_cast<uintptr_t>(ORIG_PM_Jump) + 3);
					break;
				}
			}
		}, []() { }
	);

	ORIG_PM_ClipVelocity = reinterpret_cast<_PM_ClipVelocity>(MemUtils::GetSymbolAddress(m_Handle, "PM_ClipVelocity")); // For Linux. TODO: add Windows patterns.

	bool noBhopcap = false;
	auto n = fPM_PreventMegaBunnyJumping.get();
	if (ORIG_PM_PreventMegaBunnyJumping) {
		if (n == MemUtils::INVALID_SEQUENCE_INDEX)
			EngineDevMsg("[server dll] Found PM_PreventMegaBunnyJumping at %p.\n", ORIG_PM_PreventMegaBunnyJumping);
		else
			EngineDevMsg("[server dll] Found PM_PreventMegaBunnyJumping at %p (using the %s pattern).\n", ORIG_PM_PreventMegaBunnyJumping, Patterns::ptnsPMPreventMegaBunnyJumping[n].build.c_str());
	} else {
		EngineDevWarning("[server dll] Could not find PM_PreventMegaBunnyJumping.\n");
		EngineWarning("Bhopcap disabling is not available.\n");
		noBhopcap = true;
	}

	n = fPM_PlayerMove.get();
	if (ORIG_PM_PlayerMove) {
		if (n == MemUtils::INVALID_SEQUENCE_INDEX)
			EngineDevMsg("[server dll] Found PM_PlayerMove at %p.\n", ORIG_PM_PlayerMove);
		else
			EngineDevMsg("[server dll] Found PM_PlayerMove at %p (using the %s pattern).\n", ORIG_PM_PlayerMove, Patterns::ptnsPMPlayerMove[n].build.c_str());
	} else
		EngineDevWarning("[server dll] Could not find PM_PlayerMove.\n");

	n = fPM_Jump.get();
	if (ORIG_PM_Jump) {
		if (n == MemUtils::INVALID_SEQUENCE_INDEX)
			EngineDevMsg("[server dll] Found PM_Jump at %p.\n", ORIG_PM_Jump);
		else
			EngineDevMsg("[server dll] Found PM_Jump at %p (using the %s pattern).\n", ORIG_PM_Jump, Patterns::ptnsPMJump[n].build.c_str());
		if (offBhopcap)
			EngineDevMsg("[server dll] Found the bhopcap pattern at %p.\n", reinterpret_cast<void*>(offBhopcap + reinterpret_cast<uintptr_t>(ORIG_PM_Jump) - 27));
	} else {
		EngineDevWarning("[server dll] Could not find PM_Jump.\n");
		EngineWarning("Autojump is not available.\n");
		if (!noBhopcap)
			EngineWarning("Bhopcap disabling is not available.\n");
	}

	ORIG_CmdStart = reinterpret_cast<_CmdStart>(MemUtils::GetSymbolAddress(m_Handle, "_Z8CmdStartPK7edict_sPK9usercmd_sj"));
	if (ORIG_CmdStart)
		EngineDevMsg("[server dll] Found CmdStart at %p.\n", ORIG_CmdStart);
	else {
		ORIG_GetEntityAPI = reinterpret_cast<_GetEntityAPI>(MemUtils::GetSymbolAddress(m_Handle, "GetEntityAPI"));
		if (ORIG_GetEntityAPI) {
			DLL_FUNCTIONS funcs;
			if (ORIG_GetEntityAPI(&funcs, 140)) {
				ORIG_CmdStart = funcs.pfnCmdStart; // Gets our hooked CmdStart address on Linux.
				EngineDevMsg("[server dll] Found CmdStart at %p.\n", ORIG_CmdStart);
			}
			else
				EngineDevWarning("[server dll] Could not get the server DLL function table.\n");
		} else
			EngineDevWarning("[server dll] Could not get the address of GetEntityAPI.\n");
	}

	ORIG_CNihilanth__DyingThink = reinterpret_cast<_CNihilanth__DyingThink>(MemUtils::GetSymbolAddress(m_Handle, "?DyingThink@CNihilanth@@QAEXXZ"));
	if (ORIG_CNihilanth__DyingThink)
		EngineDevMsg("[server dll] Found CNihilanth::DyingThink at %p.\n", ORIG_CNihilanth__DyingThink);
	else {
		ORIG_CNihilanth__DyingThink_Linux = reinterpret_cast<_CNihilanth__DyingThink_Linux>(MemUtils::GetSymbolAddress(m_Handle, "_ZN10CNihilanth10DyingThinkEv"));
		if (ORIG_CNihilanth__DyingThink_Linux)
			EngineDevMsg("[server dll] Found CNihilanth::DyingThink [Linux] at %p.\n", ORIG_CNihilanth__DyingThink_Linux);
		else {
			EngineDevWarning("[server dll] Could not find CNihilanth::DyingThink.\n");
			EngineWarning("Nihilanth automatic timer stopping is not available.\n");
		}
	}

	ORIG_COFGeneWorm__DyingThink = reinterpret_cast<_COFGeneWorm__DyingThink>(MemUtils::GetSymbolAddress(m_Handle, "?DyingThink@COFGeneWorm@@QAEXXZ"));
	if (ORIG_COFGeneWorm__DyingThink)
		EngineDevMsg("[server dll] Found COFGeneWorm::DyingThink at %p.\n", ORIG_COFGeneWorm__DyingThink);
	else {
		ORIG_COFGeneWorm__DyingThink_Linux = reinterpret_cast<_COFGeneWorm__DyingThink_Linux>(MemUtils::GetSymbolAddress(m_Handle, "_ZN11COFGeneWorm10DyingThinkEv"));
		if (ORIG_COFGeneWorm__DyingThink_Linux)
			EngineDevMsg("[server dll] Found COFGeneWorm::DyingThink [Linux] at %p.\n", ORIG_COFGeneWorm__DyingThink_Linux);
		else {
			EngineDevWarning("[server dll] Could not find COFGeneWorm::DyingThink.\n");
			EngineWarning("Gene Worm automatic timer stopping is not available.\n");
		}
	}

	ORIG_CMultiManager__ManagerUse = reinterpret_cast<_CMultiManager__ManagerUse>(MemUtils::GetSymbolAddress(m_Handle, "?ManagerUse@CMultiManager@@QAEXPAVCBaseEntity@@0W4USE_TYPE@@M@Z"));
	if (ORIG_CMultiManager__ManagerUse)
		EngineDevMsg("[server dll] Found CMultiManager::ManagerUse at %p.\n", ORIG_CMultiManager__ManagerUse);
	else {
		// ORIG_CMultiManager__ManagerUse_Linux = reinterpret_cast<_CMultiManager__ManagerUse_Linux>(MemUtils::GetSymbolAddress(m_Handle, "_ZN11COFGeneWorm10DyingThinkEv"));
		// if (ORIG_CMultiManager__ManagerUse_Linux)
		// 	EngineDevMsg("[server dll] Found COFGeneWorm::DyingThink [Linux] at %p.\n", ORIG_CMultiManager__ManagerUse_Linux);
		// else {
			EngineDevWarning("[server dll] Could not find CMultiManager::ManagerUse.\n");
			EngineWarning("Blue Shift automatic timer stopping is not available.\n");
		// }
	}
	
	// This has to be the last thing to check and hook.
	pEngfuncs = reinterpret_cast<enginefuncs_t*>(MemUtils::GetSymbolAddress(m_Handle, "g_engfuncs"));
	ppGlobals = reinterpret_cast<globalvars_t**>(MemUtils::GetSymbolAddress(m_Handle, "gpGlobals"));
	if (pEngfuncs && ppGlobals) {
		EngineDevMsg("[server dll] pEngfuncs is %p.\n", pEngfuncs);
		EngineDevMsg("[server dll] ppGlobals is %p.\n", ppGlobals);
	} else {
		auto pGiveFnptrsToDll = MemUtils::GetSymbolAddress(m_Handle, "GiveFnptrsToDll");
		if (pGiveFnptrsToDll)
		{
			// Find "mov edi, offset dword; rep movsd" inside GiveFnptrsToDll. The pointer to g_engfuncs is that dword.
			const byte pattern[] = { 0xBF, '?', '?', '?', '?', 0xF3, 0xA5 };
			auto addr = MemUtils::FindPattern(pGiveFnptrsToDll, 40, pattern, "x????xx");

			// Linux version: mov offset dword[eax], esi; mov [ecx+eax+4], ebx
			if (!addr)
			{
				const byte pattern_[] = { 0x89, 0xB0, '?', '?', '?', '?', 0x89, 0x5C, 0x01, 0x04 };
				addr = MemUtils::FindPattern(pGiveFnptrsToDll, 40, pattern_, "xx????xxxx");
				if (addr)
					addr = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(addr)+1); // So we're compatible with the previous pattern.
			}

			if (addr)
			{
				pEngfuncs = *reinterpret_cast<enginefuncs_t**>(reinterpret_cast<uintptr_t>(addr)+1);
				ppGlobals = *reinterpret_cast<globalvars_t***>(reinterpret_cast<uintptr_t>(addr)+9);
				EngineDevMsg("[server dll] pEngfuncs is %p.\n", pEngfuncs);
				EngineDevMsg("[server dll] ppGlobals is %p.\n", ppGlobals);
			}
			else
			{
				EngineDevWarning("[server dll] Couldn't find the pattern in GiveFnptrsToDll.\n");
				EngineWarning("Serverside logging is not available.\n");
				EngineWarning("Blue Shift automatic timer stopping is not available.\n");
			}
		}
		else
		{
			EngineDevWarning("[server dll] Couldn't get the address of GiveFnptrsToDll.\n");
			EngineWarning("Serverside logging is not avaliable.\n");
			EngineWarning("Blue Shift automatic timer stopping is not available.\n");
		}
	}
}

void ServerDLL::RegisterCVarsAndCommands()
{
	EngineDevMsg("[server dll] Registering CVars.\n");

	#define REG(cvar) HwDLL::GetInstance().RegisterCVar(CVars::cvar)
	if (ORIG_PM_Jump)
		REG(bxt_autojump);
	if (!ORIG_PM_PreventMegaBunnyJumping)
		CVars::bxt_bhopcap.Set("0");
	REG(bxt_bhopcap);
	if (ORIG_CNihilanth__DyingThink || ORIG_CNihilanth__DyingThink_Linux || ORIG_COFGeneWorm__DyingThink || ORIG_COFGeneWorm__DyingThink_Linux)
		REG(bxt_timer_autostop);
	#undef REG
}

HOOK_DEF_0(ServerDLL, void, __cdecl, PM_Jump)
{
	auto pmove = reinterpret_cast<uintptr_t>(*ppmove);
	int playerIndex = *reinterpret_cast<int*>(pmove + offPlayerIndex);

	int *onground = reinterpret_cast<int*>(pmove + offOnground);
	int orig_onground = *onground;

	int *oldbuttons = reinterpret_cast<int*>(pmove + offOldbuttons);
	int orig_oldbuttons = *oldbuttons;

	if (CVars::bxt_autojump.GetBool())
	{
		if ((orig_onground != -1) && !cantJumpNextTime[playerIndex])
			*oldbuttons &= ~IN_JUMP;
	}

	cantJumpNextTime[playerIndex] = false;

	if (offBhopcap)
	{
		auto pPMJump = reinterpret_cast<ptrdiff_t>(ORIG_PM_Jump);
		if (CVars::bxt_bhopcap.GetBool())
		{
			if (*reinterpret_cast<byte*>(pPMJump + offBhopcap) == 0x90
				&& *reinterpret_cast<byte*>(pPMJump + offBhopcap + 1) == 0x90)
				MemUtils::ReplaceBytes(reinterpret_cast<void*>(pPMJump + offBhopcap), 6, originalBhopcapInsn);
		}
		else if (*reinterpret_cast<byte*>(pPMJump + offBhopcap) == 0x0F
				&& *reinterpret_cast<byte*>(pPMJump + offBhopcap + 1) == 0x82)
				MemUtils::ReplaceBytes(reinterpret_cast<void*>(pPMJump + offBhopcap), 6, reinterpret_cast<const byte*>("\x90\x90\x90\x90\x90\x90"));
	}

	ORIG_PM_Jump();

	if ((orig_onground != -1) && (*onground == -1))
		cantJumpNextTime[playerIndex] = true;

	if (CVars::bxt_autojump.GetBool())
		*oldbuttons = orig_oldbuttons;
}

HOOK_DEF_0(ServerDLL, void, __cdecl, PM_PreventMegaBunnyJumping)
{
	if (CVars::bxt_bhopcap.GetBool())
		return ORIG_PM_PreventMegaBunnyJumping();
}

HOOK_DEF_1(ServerDLL, void, __cdecl, PM_PlayerMove, qboolean, server)
{
	if (!ppmove)
		return ORIG_PM_PlayerMove(server);

	auto pmove = reinterpret_cast<uintptr_t>(*ppmove);

	int playerIndex = *reinterpret_cast<int*>(pmove + offPlayerIndex);

	float *velocity, *origin, *angles;
	velocity = reinterpret_cast<float*>(pmove + offVelocity);
	origin =   reinterpret_cast<float*>(pmove + offOrigin);
	angles =   reinterpret_cast<float*>(pmove + offAngles);
	usercmd_t *cmd = reinterpret_cast<usercmd_t*>(pmove + offCmd);

	#define ALERT(at, format, ...) pEngfuncs->pfnAlertMessage(at, const_cast<char*>(format), ##__VA_ARGS__)
	if (CVars::_bxt_taslog.GetBool())
	{
		ALERT(at_console, "-- BXT TAS Log Start --\n");
		ALERT(at_console, "Player index: %d; msec: %hhu (%Lf)\n", playerIndex, cmd->msec, static_cast<long double>(cmd->msec) * 0.001);
		ALERT(at_console, "Velocity: %.8f; %.8f; %.8f; origin: %.8f; %.8f; %.8f\n", velocity[0], velocity[1], velocity[2], origin[0], origin[1], origin[2]);
	}

	ORIG_PM_PlayerMove(server);

	if (CVars::_bxt_taslog.GetBool())
	{
		ALERT(at_console, "Angles: %.8f; %.8f; %.8f\n", angles[0], angles[1], angles[2]);
		ALERT(at_console, "New velocity: %.8f; %.8f; %.8f; new origin: %.8f; %.8f; %.8f\n", velocity[0], velocity[1], velocity[2], origin[0], origin[1], origin[2]);
		ALERT(at_console, "-- BXT TAS Log End --\n");
	}
	#undef ALERT

	CustomHud::UpdatePlayerInfo(velocity, origin);
}

HOOK_DEF_4(ServerDLL, int, __cdecl, PM_ClipVelocity, float*, in, float*, normal, float*, out, float, overbounce)
{
	auto ret = ORIG_PM_ClipVelocity(in, normal, out, overbounce);

	if (CVars::_bxt_taslog.GetBool()) {
		if (normal[2] != 1.0f && normal[2] != -1.0f)
			pEngfuncs->pfnAlertMessage(at_console, const_cast<char*>("PM_ClipVelocity: %f (%f %f %f [%f] -> %f %f %f [%f])\n"),
				std::acos(static_cast<double>(normal[2])) * 180 / M_PI, in[0], in[1], in[2], std::hypot(in[0], in[1]), out[0], out[1], out[2], std::hypot(out[0], out[1]));
	}

	return ret;
}

HOOK_DEF_3(ServerDLL, void, __cdecl, CmdStart, const edict_t*, player, const usercmd_t*, cmd, unsigned int, random_seed)
{
	HwDLL::GetInstance().SetLastRandomSeed(random_seed);
	auto seed = random_seed;
	bool changedSeed = false;
	if (HwDLL::GetInstance().IsCountingSharedRNGSeed()) {
		auto lastSeed = HwDLL::GetInstance().GetSharedRNGSeedCounter();
		seed = lastSeed - (--HwDLL::GetInstance().QueuedSharedRNGSeeds);
		changedSeed = true;
	}

	#define ALERT(at, format, ...) pEngfuncs->pfnAlertMessage(at, const_cast<char*>(format), ##__VA_ARGS__)
	if (CVars::_bxt_taslog.GetBool())
	{
		ALERT(at_console, "-- CmdStart Start --\n");
		ALERT(at_console, "Buttons: %hu\n", cmd->buttons);
		ALERT(at_console, "Random_seed: %u", random_seed);
		if (changedSeed)
			ALERT(at_console, " (overriding with %u)", seed);
		ALERT(at_console, "\n");
		ALERT(at_console, "Paused: %s\n", (HwDLL::GetInstance().IsPaused() ? "true" : "false"));
		ALERT(at_console, "-- CmdStart End --\n");
	}
	#undef ALERT

	return ORIG_CmdStart(player, cmd, seed);
}

HOOK_DEF_2(ServerDLL, void, __fastcall, CNihilanth__DyingThink, void*, thisptr, int, edx)
{
	if (CVars::bxt_timer_autostop.GetBool())
		CustomHud::SetCountingTime(false);

	return ORIG_CNihilanth__DyingThink(thisptr, edx);
}

HOOK_DEF_1(ServerDLL, void, __cdecl, CNihilanth__DyingThink_Linux, void*, thisptr)
{
	if (CVars::bxt_timer_autostop.GetBool())
		CustomHud::SetCountingTime(false);

	return ORIG_CNihilanth__DyingThink_Linux(thisptr);
}

HOOK_DEF_2(ServerDLL, void, __fastcall, COFGeneWorm__DyingThink, void*, thisptr, int, edx)
{
	if (CVars::bxt_timer_autostop.GetBool())
		CustomHud::SetCountingTime(false);

	return ORIG_COFGeneWorm__DyingThink(thisptr, edx);
}

HOOK_DEF_1(ServerDLL, void, __cdecl, COFGeneWorm__DyingThink_Linux, void*, thisptr)
{
	if (CVars::bxt_timer_autostop.GetBool())
		CustomHud::SetCountingTime(false);

	return ORIG_COFGeneWorm__DyingThink_Linux(thisptr);
}

HOOK_DEF_6(ServerDLL, void, __fastcall, CMultiManager__ManagerUse, void*, thisptr, int, edx, void*, pActivator, void*, pCaller, int, useType, float, value)
{
	if (CVars::bxt_timer_autostop.GetBool() && ppGlobals && pCaller) {
		entvars_t *pev = *reinterpret_cast<entvars_t**>(reinterpret_cast<uintptr_t>(thisptr) + 4);
		if (pev && pev->targetname) {
			const char *targetname = (*ppGlobals)->pStringBase + pev->targetname;
			if (!std::strcmp(targetname, "roll_the_credits") || !std::strcmp(targetname, "youwinmulti")) {
				entvars_t *callerPev = *reinterpret_cast<entvars_t**>(reinterpret_cast<uintptr_t>(pCaller) + 4);
				if (callerPev && callerPev->targetname) {
					const char *callerTargetname = (*ppGlobals)->pStringBase + callerPev->targetname;
					if (!std::strcmp(callerTargetname, "mgr_take_over") || !std::strcmp(callerTargetname, "endbot"))
						CustomHud::SetCountingTime(false);
				}
			}
		}
	}

	return ORIG_CMultiManager__ManagerUse(thisptr, edx, pActivator, pCaller, useType, value);
}
