#include "nvse/GameObjects.h"
#include "nvse/GameExtraData.h"
#include "nvse/GameTasks.h"

extern PlayerCharacter *g_thePlayer;

ScriptEventList *TESObjectREFR::GetEventList() const
{
	ExtraScript *xScript = GetExtraType(&extraDataList, Script);
	return xScript ? xScript->eventList : NULL;
}

PlayerCharacter *PlayerCharacter::GetSingleton()
{
	return g_thePlayer;
}

__declspec(naked) TESContainer *TESObjectREFR::GetContainer()
{
	__asm
	{
		mov		eax, [ecx]
		mov		eax, [eax+0x100]
		call	eax
		test	al, al
		mov		eax, [ecx+0x20]
		jz		notActor
		add		eax, 0x64
		retn
	notActor:
		cmp		dword ptr [eax], kVtbl_TESObjectCONT
		jnz		notCONT
		add		eax, 0x30
		retn
	notCONT:
		xor		eax, eax
		retn
	}
}

bool TESObjectREFR::IsMapMarker()
{
	return baseForm->refID == 0x10;
}

extern ModelLoader *g_modelLoader;

void TESObjectREFR::Update3D()
{
	if (this == g_thePlayer)
	{
		ThisCall(0x8D3FA0, this);
	}
	else if (GetNiNode())
	{
		Set3D(NULL, true);
		extraDataList.flags |= 1;
		g_modelLoader->QueueReference(this);
	}
}

TESObjectREFR *TESObjectREFR::Create(bool bTemp)
{
	TESObjectREFR *refr = (TESObjectREFR*)GameHeapAlloc(sizeof(TESObjectREFR));
	ThisCall(0x55A2F0, refr);
	if (bTemp) ThisCall(0x484490, refr);
	return refr;
}