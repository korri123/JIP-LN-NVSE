#include "nvse/GameBSExtraData.h"
#include "nvse/GameExtraData.h"

bool BaseExtraList::HasType(UInt32 type) const
{
	return (m_presenceBitfield[type >> 3] & (1 << (type & 7))) != 0;
}

__declspec(naked) BSExtraData *BaseExtraList::GetByType(UInt32 xType) const
{
	__asm
	{
		cmp		dword ptr [ecx+4], 0
		jz		retnNULL
		mov		edx, [esp+4]
		shr		edx, 3
		movzx	eax, byte ptr [ecx+edx+8]
		mov		edx, [esp+4]
		and		edx, 7
		bt		eax, edx
		jnc		retnNULL
		push	ecx
		mov		ecx, 0x11C3920
		call	LightCS::Enter
		pop		ecx
		mov		eax, [ecx+4]
		mov		edx, [esp+4]
		ALIGN 16
	iterHead:
		cmp		[eax+4], dl
		jz		lockLeave
		mov		eax, [eax+8]
		test	eax, eax
		jnz		iterHead
	lockLeave:
		mov		edx, 0x11C3920
		dec		dword ptr [edx+4]
		jnz		done
		mov		dword ptr [edx], 0
	done:
		retn	4
	retnNULL:
		xor		eax, eax
		retn	4
	}
}

__declspec(naked) ExtraDataList* __stdcall ExtraDataList::Create(BSExtraData *xBSData)
{
	__asm
	{
		push	0x20
		GAME_HEAP_ALLOC
		pxor	xmm0, xmm0
		movups	[eax], xmm0
		movups	[eax+0x10], xmm0
		mov		dword ptr [eax], kVtbl_ExtraDataList
		mov		edx, [esp+4]
		test	edx, edx
		jz		done
		push	eax
		push	edx
		mov		ecx, eax
		CALL_EAX(0x40FF60)
		pop		eax
	done:
		retn	4
	}
}

char BaseExtraList::GetExtraFactionRank(TESFaction *faction) const
{
	ExtraFactionChanges *xFactionChanges = GetExtraType(this, FactionChanges);
	if (xFactionChanges && xFactionChanges->data)
	{
		ListNode<FactionListData> *traverse = xFactionChanges->data->Head();
		FactionListData *pData;
		do
		{
			pData = traverse->data;
			if (pData && (pData->faction == faction))
				return pData->rank;
		}
		while (traverse = traverse->next);
	}
	return -1;
}

SInt32 BaseExtraList::GetCount() const
{
	ExtraCount *xCount = GetExtraType(this, Count);
	return xCount ? xCount->count : 1;
}

void __fastcall ExtraValueStr(BSExtraData *xData, char *buffer)
{
	switch (xData->type)
	{
		case kExtraData_Ownership:
		{
			ExtraOwnership *xOwnership = (ExtraOwnership*)xData;
			sprintf_s(buffer, 0x20, "%08X", xOwnership->owner ? xOwnership->owner->refID : 0);
			break;
		}
		case kExtraData_Count:
		{
			ExtraCount *xCount = (ExtraCount*)xData;
			sprintf_s(buffer, 0x20, "%d", xCount->count);
			break;
		}
		default:
			sprintf_s(buffer, 0x20, "%08X", ((UInt32*)xData)[3]);
			break;
	}
}

void BaseExtraList::DebugDump() const
{
	PrintDebug("\nBaseExtraList Dump:");
	Console_Print("BaseExtraList Dump:");
	s_debug.Indent();
	if (m_data)
	{
		char dataStr[0x20];
		for (BSExtraData *traverse = m_data; traverse; traverse = traverse->next)
		{
			ExtraValueStr(traverse, dataStr);
			PrintDebug("%08X\t%02X\t%s\t%s", traverse, traverse->type, GetExtraDataName(traverse->type), dataStr);
			Console_Print("%08X  %02X  %s  %s", traverse, traverse->type, GetExtraDataName(traverse->type), dataStr);
		}
		Console_Print(" ");
	}
	else
	{
		PrintDebug("No data in list");
		Console_Print("No data in list");
	}
	s_debug.Outdent();
}
