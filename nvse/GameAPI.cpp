#include "nvse/GameAPI.h"

//static NVSEStringVarInterface* s_StringVarInterface = NULL;
bool extraTraces = false;

// arg1 = 1, ignored if canCreateNew is false, passed to 'init' function if a new object is created
typedef void * (* _GetSingleton)(bool canCreateNew);

const _ExtractArgs ExtractArgs = (_ExtractArgs)0x005ACCB0;

const _FormHeap_Allocate FormHeap_Allocate = (_FormHeap_Allocate)0x00401000;
const _FormHeap_Free FormHeap_Free = (_FormHeap_Free)0x00401030;

const _LookupFormByID LookupFormByID = (_LookupFormByID)0x004839C0;
const _CreateFormInstance CreateFormInstance = (_CreateFormInstance)0x00465110;

const _GetSingleton ConsoleManager_GetSingleton = (_GetSingleton)0x0071B160;
bool * bEchoConsole = (bool*)0x011F158C;

const _QueueUIMessage QueueUIMessage = (_QueueUIMessage)0x007052F0;

const _ShowMessageBox ShowMessageBox = (_ShowMessageBox)0x00703E80;
const _ShowMessageBox_Callback ShowMessageBox_Callback = (_ShowMessageBox_Callback)0x005B4A70;
const _ShowMessageBox_pScriptRefID ShowMessageBox_pScriptRefID = (_ShowMessageBox_pScriptRefID)0x011CAC64;
const _ShowMessageBox_button ShowMessageBox_button = (_ShowMessageBox_button)0x0118C684;

const _GetActorValueName GetActorValueName = (_GetActorValueName)0x00066EAC0;	// See Cmd_GetActorValue_Eval
const UInt32 * g_TlsIndexPtr = (UInt32 *)0x0126FD98;
const _MarkBaseExtraListScriptEvent MarkBaseExtraListScriptEvent = (_MarkBaseExtraListScriptEvent)0x005AC790;

SaveGameManager ** g_saveGameManager = (SaveGameManager**)0x011DE134;

#if RUNTIME

struct TLSData
{
	// thread local storage

	UInt32	pad000[(0x260 - 0x000) >> 2];	// 000
	NiNode			* lastNiNode;			// 260
	TESObjectREFR	* lastNiNodeREFR;		// 264
	UInt8			consoleMode;			// 268
	UInt8			pad269[3];				// 269
	// 25C is used as do not head track the player , 2B8 is used to init QueudFile::unk0018
};

static TLSData * GetTLSData()
{
	UInt32 TlsIndex = *g_TlsIndexPtr;
	TLSData * data = NULL;

	__asm {
		mov		ecx,	[TlsIndex]
		mov		edx,	fs:[2Ch]	// linear address of thread local storage array
		mov		eax,	[edx+ecx*4]
		mov		[data], eax
	}

	return data;
}

bool IsConsoleMode()
{
	TLSData * tlsData = GetTLSData();

	if(tlsData)
		return tlsData->consoleMode != 0;

	return false;
}

bool GetConsoleEcho()
{
	return *bEchoConsole != 0;
}

void SetConsoleEcho(bool doEcho)
{
	*bEchoConsole = doEcho ? 1 : 0;
}

const char * GetFullName(TESForm * baseForm)
{
	if(baseForm)
	{
		TESFullName* fullName = baseForm->GetFullName();
		if(fullName && fullName->name.m_data)
		{
			if (fullName->name.m_dataLen)
				return fullName->name.m_data;
		}
	}

	return "<no name>";
}

ConsoleManager * ConsoleManager::GetSingleton(void)
{
	return (ConsoleManager *)ConsoleManager_GetSingleton(true);
}

void Console_Print(const char * fmt, ...)
{
	ConsoleManager	* mgr = ConsoleManager::GetSingleton();
	if(mgr)
	{
		va_list	args;

		va_start(args, fmt);

		CALL_MEMBER_FN(mgr, Print)(fmt, args);

		va_end(args);
	}
}

TESSaveLoadGame * TESSaveLoadGame::Get()
{
	return (TESSaveLoadGame *)0x011DE45C;
}

SaveGameManager* SaveGameManager::GetSingleton()
{
	return *g_saveGameManager;
}

std::string GetSavegamePath()
{
	char path[0x104];
	CALL_MEMBER_FN(SaveGameManager::GetSingleton(), ConstructSavegamePath)(path);
	return path;
}

// ExtractArgsEx code
ScriptEventList* ResolveExternalVar(ScriptEventList* in_EventList, Script* in_Script, UInt8* &scriptData)
{
	UInt16 varIdx = *(UInt16*)++scriptData;
	scriptData += 2;

	Script::RefVariable* refVar = in_Script->GetVariable(varIdx);
	if (refVar)
	{
		refVar->Resolve(in_EventList);
		TESForm* refObj = refVar->form;
		if (refObj)
		{
			if IS_ID(refObj, TESQuest)
				return ((TESQuest*)refObj)->scriptEventList;
			else if (((refObj->typeID >= kFormType_TESObjectREFR) && (refObj->typeID <= kFormType_FlameProjectile)) || IS_ID(refObj, ContinuousBeamProjectile))
				return ((TESObjectREFR*)refObj)->GetEventList();
		}
	}

	return NULL;
}

TESGlobal* ResolveGlobalVar(ScriptEventList* in_EventList, Script* in_Script, UInt8* &scriptData)
{
	UInt16 varIdx = *(UInt16*)++scriptData;
	scriptData += 2;

	Script::RefVariable* globalRef = in_Script->GetVariable(varIdx);
	if (globalRef && IS_ID(globalRef->form, TESGlobal))
		return (TESGlobal*)globalRef->form;

	return NULL;
}

bool ExtractFloat(double *out, UInt8 *&scriptData, Script *scriptObj, ScriptEventList *eventList)
{
	//extracts one float arg

	if (*scriptData == 'r')		//reference to var in another script
	{
		Script::RefVariable* refVar = scriptObj->GetVariable(*(UInt16*)(scriptData + 1));
		if (!refVar) return false;

		refVar->Resolve(eventList);
		TESForm *refObj = refVar->form;
		if (!refObj) return false;

		if IS_ID(refObj, TESQuest)
			eventList = ((TESQuest*)refObj)->scriptEventList;
		else if (((refObj->typeID >= kFormType_TESObjectREFR) && (refObj->typeID <= kFormType_FlameProjectile)) || IS_ID(refObj, ContinuousBeamProjectile))
			eventList = ((TESObjectREFR*)refObj)->GetEventList();
		else eventList = NULL;

		if (!eventList)			//couldn't resolve script ref
			return false;
		scriptData += 3;
	}	

	switch (*scriptData)
	{
		case 'G':		//global var
		{
			Script::RefVariable *globalRef = scriptObj->GetVariable(*(UInt16*)(scriptData + 1));
			if (globalRef && IS_ID(globalRef->form, TESGlobal))
			{
				scriptData += 3;
				*out = ((TESGlobal*)globalRef->form)->data;
				return true;
			}
			break;
		}
		case 'z':		//literal double
		{
			*out = *(double*)++scriptData;
			scriptData += sizeof(double);
			return true;
		}
		case 'f':
		case 's':		//local var
		{
			ScriptVar *var = eventList->GetVariable(*(UInt16*)(scriptData + 1));
			if (var)
			{
				scriptData += 3;
				*out = var->data.num;
				return true;
			}
			break;
		}
	}
	return false;
}

TESForm* ExtractFormFromFloat(UInt8* &scriptData, Script* scriptObj, ScriptEventList* eventList)
{
	if (*scriptData == 'r')		//doesn't work as intended yet so refs must be local vars
	{
		eventList = ResolveExternalVar(eventList, scriptObj, scriptData);
		if (!eventList)
			return NULL;
	}

	UInt16 varIdx = *(UInt16*)++scriptData;
	scriptData += 2;

	ScriptVar *var = eventList->GetVariable(varIdx);
	if (var)
		return LookupFormByRefID(var->data.refID);
}

TESForm* ResolveForm(UInt8* &scriptData, Script* scriptObj, ScriptEventList* eventList)
{
	char argType = *scriptData;
	UInt16 varIdx = *(UInt16*)(scriptData + 1);
	//scriptData += 2;

	if (argType == 'r')
	{
		Script::RefVariable	*var = scriptObj->GetVariable(varIdx);
		if (var)
		{
			var->Resolve(eventList);
			scriptData += 3;
			return var->form;
		}
	}
	else if (argType == 'f')
		return ExtractFormFromFloat(scriptData, scriptObj, eventList);

	return NULL;
}

static const char* StringFromStringVar(UInt32 strID)
{
	/*const char *strVal = GetStringVar(strID);
	return strVal ? strVal : "";*/
	return "";
}

static const char* ResolveStringArgument(ScriptEventList* eventList, const char* stringArg)
{
	if (stringArg && stringArg[0] == '$')
	{
		VariableInfo* varInfo = eventList->m_script->GetVariableByName(stringArg + 1);
		if (varInfo)
		{
			ScriptVar *var = eventList->GetVariable(varInfo->idx);
			if (var)
				return StringFromStringVar((int)var->data.num);
		}
	}
	return stringArg;
}

/*bool v_ExtractArgsEx(SInt16 numArgs, ParamInfo *paramInfo, UInt8 *&scriptData, Script *scriptObj, ScriptEventList *eventList, va_list args)
{
	ParamInfo *info;
	UInt32 paramType;
	for (UInt32 i = 0; i < numArgs; i++)
	{
		info = &paramInfo[i];
		paramType = info->typeID;
		if (paramType > kParamType_Region)
			return false;

		switch (kClassifyParamExtract[paramType])
		{
			case 0:
			{
				char *out = va_arg(args, char*);
				UInt16 len = *(UInt16*)scriptData;
				scriptData += 2;
				memcpy(out, scriptData, len);
				scriptData += len;
				out[len] = 0;

				if (*out == '$')
				{
					VariableInfo *varInfo = eventList->m_script->GetVariableByName(out + 1);
					if (varInfo)
					{
						ScriptVar *var = eventList->GetVariable(varInfo->idx);
						if (var)
						{
							const char *strVal = GetStringVar((int)var->data);
							len = StrLen(strVal);
							if (len) memcpy(out, strVal, len + 1);
						}
					}
				}
				break;
			}
			case 1:
			{
				SInt32 *out = va_arg(args, SInt32*);
				if (*scriptData == 'n')
				{
					*out = *(SInt32*)++scriptData;
					scriptData += sizeof(SInt32);
				}
				else if (*scriptData == 'z')
				{
					*out = *(double*)++scriptData;
					scriptData += sizeof(double);
				}
				else
				{
					double data;
					if (!ExtractFloat(&data, scriptData, scriptObj, eventList))
						return false;
					*out = data;
				}
				break;
			}
			case 2:
			{
				UInt32 *out = va_arg(args, UInt32*);
				*out = *(UInt16*)scriptData;
				scriptData += 2;
				break;
			}
			case 3:
			{
				UInt8 *out = va_arg(args, UInt8*);
				*out = *(UInt8*)scriptData;
				scriptData += 1;
				break;
			}
			case 4:
			{
				float *out = va_arg(args, float*);
				if (*scriptData == 'z')
				{
					*out = *(double*)++scriptData;
					scriptData += sizeof(double);
				}
				else
				{
					double data;
					if (!ExtractFloat(&data, scriptData, scriptObj, eventList))
						return false;
					*out = data;
				}
				break;
			}
			case 5:
			{
				double *out = va_arg(args, double*);
				if (*scriptData == 'z')
				{
					*out = *(double*)++scriptData;
					scriptData += sizeof(double);
				}
				else if (!ExtractFloat(out, scriptData, scriptObj, eventList))
					return false;
				break;
			}
			case 6:
			{
				TESForm *form = NULL;
				if (*scriptData == 'f')
				{
					ScriptVar *var = eventList->GetVariable(*(UInt16*)(scriptData + 1));
					if (var)
						form = LookupFormByRefID(*(UInt32*)&var->data);
				}
				else if (*scriptData == 'r')
				{
					Script::RefVariable	*var = scriptObj->GetVariable(*(UInt16*)(scriptData + 1));
					if (var)
					{
						var->Resolve(eventList);
						form = var->form;
					}
				}

				if (!form) return false;
				scriptData += 3;
				TESForm **out = va_arg(args, TESForm**);

				switch (paramType)
				{
					case kParamType_ObjectID:
					{
						if (!kInventoryType[form->typeID])
							return false;
						*out = form;
						break;
					}
					case kParamType_ObjectRef:
					case kParamType_Container:
					{
						if ((form->typeID < kFormType_TESObjectREFR) || ((form->typeID > kFormType_FlameProjectile) && NOT_ID(form, ContinuousBeamProjectile)))
							return false;
						*out = form;
						break;
					}
					case kParamType_Actor:
					{
						if (NOT_ID(form, Character) && NOT_ID(form, Creature))
							return false;
						*out = form;
						break;
					}
					case kParamType_SpellItem:
					{
						if (NOT_ID(form, SpellItem) && NOT_ID(form, TESObjectBOOK))
							return false;
						*out = form;
						break;
					}
					case kParamType_Cell:
					{
						if NOT_ID(form, TESObjectCELL)
							return false;
						*out = form;
						break;
					}
					case kParamType_MagicItem:
					{
						MagicItem *magicItem = DYNAMIC_CAST(form, TESForm, MagicItem);
						if (!magicItem) return false;
						*(MagicItem**)out = magicItem;
						break;
					}
					case kParamType_Sound:
					{
						if NOT_ID(form, TESSound)
							return false;
						*out = form;
						break;
					}
					case kParamType_Topic:
					{
						if NOT_ID(form, TESTopic)
							return false;
						*out = form;
						break;
					}
					case kParamType_Quest:
					{
						if NOT_ID(form, TESQuest)
							return false;
						*out = form;
						break;
					}
					case kParamType_Race:
					{
						if NOT_ID(form, TESRace)
							return false;
						*out = form;
						break;
					}
					case kParamType_Class:
					{
						if NOT_ID(form, TESClass)
							return false;
						*out = form;
						break;
					}
					case kParamType_Faction:
					{
						if NOT_ID(form, TESFaction)
							return false;
						*out = form;
						break;
					}
					case kParamType_Global:
					{
						if NOT_ID(form, TESGlobal)
							return false;
						*out = form;
						break;
					}
					case kParamType_Furniture:
					{
						if (NOT_ID(form, TESFurniture) && NOT_ID(form, BGSListForm))
							return false;
						*out = form;
						break;
					}
					case kParamType_TESObject:
					{
						if (!form->IsBoundObject())
							return false;
						*out = form;
						break;
					}
					case kParamType_MapMarker:
					{
						if (NOT_ID(form, TESObjectREFR) || (((TESObjectREFR*)form)->baseForm->refID != 0x10))
							return false;
						*out = form;
						break;
					}
					case kParamType_ActorBase:
					{
						if (NOT_ID(form, TESNPC) && NOT_ID(form, TESCreature))
							return false;
						*out = form;
						break;
					}
					case kParamType_WorldSpace:
					{
						if NOT_ID(form, TESWorldSpace)
							return false;
						*out = form;
						break;
					}
					case kParamType_AIPackage:
					{
						if NOT_ID(form, TESPackage)
							return false;
						*out = form;
						break;
					}
					case kParamType_CombatStyle:
					{
						if NOT_ID(form, TESCombatStyle)
							return false;
						*out = form;
						break;
					}
					case kParamType_MagicEffect:
					{
						if NOT_ID(form, EffectSetting)
							return false;
						*out = form;
						break;
					}
					case kParamType_WeatherID:
					{
						if NOT_ID(form, TESWeather)
							return false;
						*out = form;
						break;
					}
					case kParamType_NPC:
					{
						if NOT_ID(form, TESNPC)
							return false;
						*out = form;
						break;
					}
					case kParamType_Owner:
					{
						if (NOT_ID(form, TESFaction) && NOT_ID(form, TESNPC))
							return false;
						*out = form;
						break;
					}
					case kParamType_EffectShader:
					{
						if NOT_ID(form, TESEffectShader)
							return false;
						*out = form;
						break;
					}
					case kParamType_FormList:
					{
						if NOT_ID(form, BGSListForm)
							return false;
						*out = form;
						break;
					}
					case kParamType_MenuIcon:
					{
						if NOT_ID(form, BGSMenuIcon)
							return false;
						*out = form;
						break;
					}
					case kParamType_Perk:
					{
						if NOT_ID(form, BGSPerk)
							return false;
						*out = form;
						break;
					}
					case kParamType_Note:
					{
						if NOT_ID(form, BGSNote)
							return false;
						*out = form;
						break;
					}
					case kParamType_ImageSpaceModifier:
					{
						if NOT_ID(form, TESImageSpaceModifier)
							return false;
						*out = form;
						break;
					}
					case kParamType_ImageSpace:
					{
						if NOT_ID(form, TESImageSpace)
							return false;
						*out = form;
						break;
					}
					case kParamType_EncounterZone:
					{
						if NOT_ID(form, BGSEncounterZone)
							return false;
						*out = form;
						break;
					}
					case kParamType_Message:
					{
						if NOT_ID(form, BGSMessage)
							return false;
						*out = form;
						break;
					}
					case kParamType_InvObjOrFormList:
					{
						if (NOT_ID(form, BGSListForm) && !kInventoryType[form->typeID])
							return false;
						*out = form;
						break;
					}
					case kParamType_NonFormList:
					{
						if (IS_ID(form, BGSListForm) || !form->IsBoundObject())
							return false;
						*out = form;
						break;
					}
					case kParamType_SoundFile:
					{
						if NOT_ID(form, BGSMusicType)
							return false;
						*out = form;
						break;
					}
					case kParamType_LeveledOrBaseChar:
					{
						if (NOT_ID(form, TESNPC) && NOT_ID(form, TESLevCharacter))
							return false;
						*out = form;
						break;
					}
					case kParamType_LeveledOrBaseCreature:
					{
						if (NOT_ID(form, TESCreature) && NOT_ID(form, TESLevCreature))
							return false;
						*out = form;
						break;
					}
					case kParamType_LeveledChar:
					{
						if NOT_ID(form, TESLevCharacter)
							return false;
						*out = form;
						break;
					}
					case kParamType_LeveledCreature:
					{
						if NOT_ID(form, TESLevCreature)
							return false;
						*out = form;
						break;
					}
					case kParamType_LeveledItem:
					{
						if NOT_ID(form, TESLevItem)
							return false;
						*out = form;
						break;
					}
					case kParamType_Reputation:
					{
						if NOT_ID(form, TESReputation)
							return false;
						*out = form;
						break;
					}
					case kParamType_Casino:
					{
						if NOT_ID(form, TESCasino)
							return false;
						*out = form;
						break;
					}
					case kParamType_CasinoChip:
					{
						if NOT_ID(form, TESCasinoChips)
							return false;
						*out = form;
						break;
					}
					case kParamType_Challenge:
					{
						if NOT_ID(form, TESChallenge)
							return false;
						*out = form;
						break;
					}
					case kParamType_CaravanMoney:
					{
						if NOT_ID(form, TESCaravanMoney)
							return false;
						*out = form;
						break;
					}
					case kParamType_CaravanCard:
					{
						if NOT_ID(form, TESCaravanCard)
							return false;
						*out = form;
						break;
					}
					case kParamType_CaravanDeck:
					{
						if NOT_ID(form, TESCaravanDeck)
							return false;
						*out = form;
						break;
					}
					case kParamType_Region:
					{
						if NOT_ID(form, TESRegion)
							return false;
						*out = form;
						break;
					}
					default:
					{
						*out = form;
						break;
					}
				}
				break;
			}
			default:
				return false;
		}
	}

	return true;
}*/

bool ExtractArgsRaw(ParamInfo * paramInfo, void * scriptDataIn, UInt32 * scriptDataOffset, Script * scriptObj, ScriptEventList * eventList, ...)
{
	va_list	args;
	va_start(args, eventList);

	UInt8	* scriptData = ((UInt8 *)scriptDataIn) + *scriptDataOffset;

	UInt32	numArgs = *((UInt16 *)scriptData);
	scriptData += sizeof(UInt16);

	for(UInt32 i = 0; i < numArgs; i++)
	{
		ParamInfo		* info = &paramInfo[i];
		ExtractedParam	* dst = va_arg(args, ExtractedParam *);

		dst->type = ExtractedParam::kType_Unknown;
		dst->isVar = false;

		switch(info->typeID)
		{
			case kParamType_String:
				dst->type = ExtractedParam::kType_String;
				dst->data.str.len = *((UInt16 *)scriptData);
				scriptData += sizeof(UInt16);
				dst->data.str.buf = (const char *)scriptData;
				scriptData += dst->data.str.len;
				break;

			case kParamType_ActorValue:
			case kParamType_AnimationGroup:
			case kParamType_Sex:
			case kParamType_CrimeType:
			case kParamType_MiscellaneousStat:
			case kParamType_Alignment:
			case kParamType_EquipType:
			case kParamType_CriticalStage:
				dst->type = ExtractedParam::kType_Imm16;
				dst->data.imm = *((UInt16 *)scriptData);
				scriptData += sizeof(UInt16);
				break;

			case kParamType_Axis:
			case kParamType_FormType:
				dst->type = ExtractedParam::kType_Imm8;
				dst->data.imm = *((UInt8 *)scriptData);
				scriptData += sizeof(UInt8);
				break;

			case kParamType_Integer:
			case kParamType_QuestStage:
			case kParamType_Float:
			{
				UInt8	type = *scriptData++;
				switch(type)
				{
					case 'n':	// 6E
						dst->type = ExtractedParam::kType_Imm32;
						dst->data.imm = *((UInt32 *)scriptData);
						scriptData += sizeof(UInt32);
						break;

					case 'z':	// 7A
						dst->type = ExtractedParam::kType_ImmDouble;
						dst->data.immDouble = (double *)scriptData;
						scriptData += sizeof(double);
						break;

					case 'f':	// 66
					case 'r':	// 72
					case 's':	// 73
					case 'G':	// 47
					{
						ScriptEventList	* srcEventList = eventList;
						UInt8			varType = *scriptData++;

						// remote reference?
						if(varType == 'r')
						{
							// swap the event list
							UInt16	varIdx = *((UInt16 *)scriptData);
							scriptData += 2;

							Script::RefVariable	* var = scriptObj->GetVariable(varIdx);
							if(var)
							{
								TESForm	* eventListSrc = var->form;
								switch(eventListSrc->typeID)
								{
									case kFormType_TESObjectREFR:
										srcEventList = ((TESObjectREFR *)eventListSrc)->GetEventList();
										break;

									case kFormType_TESQuest:
										srcEventList = ((TESQuest *)eventListSrc)->scriptEventList;
										break;

									default:
										//_ERROR("ExtractArgsRaw: unknown remote reference in number var (%02X)", eventListSrc->typeID);
										return false;
								}
							}
							else
							{
								//_ERROR("ExtractArgsRaw: couldn't find remote reference in number var");
								return false;
							}
						}
						
						else
						{
							// ###

							//default:
								//_ERROR("ExtractArgsRaw: unknown number var type (%02X)", type);
								return false;
						}
					}
					break;

					default:
						//_ERROR("ExtractArgsRaw: unknown number type (%02X)", type);
						return false;
				}
			}
			break;

			case kParamType_ObjectID:
			case kParamType_ObjectRef:
			case kParamType_Actor:
			case kParamType_SpellItem:
			case kParamType_Cell:
			case kParamType_MagicItem:
			case kParamType_Sound:
			case kParamType_Topic:
			case kParamType_Quest:
			case kParamType_Race:
			case kParamType_Class:
			case kParamType_Faction:
			case kParamType_Global:
			case kParamType_Furniture:
			case kParamType_TESObject:
			case kParamType_MapMarker:
			case kParamType_ActorBase:
			case kParamType_Container:
			case kParamType_WorldSpace:
			case kParamType_AIPackage:
			case kParamType_CombatStyle:
			case kParamType_MagicEffect:
			case kParamType_WeatherID:
			case kParamType_NPC:
			case kParamType_Owner:
			case kParamType_EffectShader:
			case kParamType_FormList:
			case kParamType_MenuIcon:
			case kParamType_Perk:
			case kParamType_Note:
			case kParamType_ImageSpaceModifier:
			case kParamType_ImageSpace:
			case kParamType_EncounterZone:
			case kParamType_Message:
			case kParamType_InvObjOrFormList:
			case kParamType_NonFormList:
			case kParamType_SoundFile:
			case kParamType_LeveledOrBaseChar:
			case kParamType_LeveledOrBaseCreature:
			case kParamType_LeveledChar:
			case kParamType_LeveledCreature:
			case kParamType_LeveledItem:
			case kParamType_AnyForm:
			case kParamType_Reputation:
			case kParamType_Casino:
			case kParamType_CasinoChip:
			case kParamType_Challenge:
			case kParamType_CaravanMoney:
			case kParamType_CaravanCard:
			case kParamType_CaravanDeck:
			case kParamType_Region:
			{
				UInt8	type = *scriptData++;
				switch(type)
				{
					case 'r':	// constant
					{
						UInt16	varIdx = *((UInt16 *)scriptData);
						scriptData += 2;

						Script::RefVariable	*var = scriptObj->GetVariable(varIdx);
						if (!var)
							return false;

						var->Resolve(eventList);

						dst->type = ExtractedParam::kType_Form;
						dst->data.form = var->form;
					}
					break;

					case 'f':	// variable
					{
						UInt16	varIdx = *((UInt16 *)scriptData);
						scriptData += 2;

						dst->type = ExtractedParam::kType_Form;
						dst->isVar = true;
						dst->data.var.var = eventList->GetVariable(varIdx);
						dst->data.var.parent = eventList;
					}
					break;

					default:
						//_ERROR("ExtractArgsRaw: unknown form type (%02X)", type);
						return false;
				}
			}
			break;

			case kParamType_VariableName:
				// unhandled, fall through

			default:
				//_ERROR("ExtractArgsRaw: unhandled type encountered, arg %d type %02X", i, info->typeID);
				//HALT("unhandled type");
				break;
		}
	}

	return true;
}

/*bool ExtractArgsEx(ParamInfo * paramInfo, void * scriptDataIn, UInt32 * scriptDataOffset, Script * scriptObj, ScriptEventList * eventList, ...)
{
	va_list	args;
	va_start(args, eventList);

	UInt8	* scriptData = ((UInt8 *)scriptDataIn) + *scriptDataOffset;
	UInt32	numArgs = *((UInt16 *)scriptData);
	scriptData += 2;

	//DEBUG_MESSAGE("scriptData:%08x numArgs:%d paramInfo:%08x scriptObj:%08x eventList:%08x", scriptData, numArgs, paramInfo, scriptObj, eventList);

	bool bExtracted = v_ExtractArgsEx(numArgs, paramInfo, scriptData, scriptObj, eventList, args);
	va_end(args);
	return bExtracted;
}*/

/*void ScriptEventList::Dump(void)
{
	UInt32 nEvents = m_eventList->Count();

	for(SInt32 n = 0; n < nEvents; ++n)
	{
		Event* pEvent = m_eventList->GetNthItem(n);
		if(pEvent)
		{
			Console_Print("%08X (%s) %08X", pEvent->object, GetObjectClassName(pEvent->object), pEvent->eventMask);
		}
	}
}*/

UInt32 ScriptEventList::ResetAllVariables()
{
	if (!m_vars) return 0;
	ListNode<ScriptVar> *varIter = m_vars->Head();
	ScriptVar *scriptVar;
	UInt32 numVars = 0;
	do
	{
		scriptVar = varIter->data;
		if (scriptVar)
		{
			scriptVar->data.num = 0;
			numVars++;
		}
	}
	while (varIter = varIter->next);
	return numVars;
}

ScriptVar *ScriptEventList::GetVariable(UInt32 id)
{
	if (m_vars)
	{
		ListNode<ScriptVar> *varIter = m_vars->Head();
		ScriptVar *scriptVar;
		do
		{
			scriptVar = varIter->data;
			if (scriptVar && (scriptVar->id == id))
				return scriptVar;
		}
		while (varIter = varIter->next);
	}
	return NULL;
}

ScriptEventList* EventListFromForm(TESForm* form)
{
	ScriptEventList* eventList = NULL;
	TESObjectREFR* refr = DYNAMIC_CAST(form, TESForm, TESObjectREFR);
	if (refr)
		eventList = refr->GetEventList();
	else
	{
		TESQuest* quest = DYNAMIC_CAST(form, TESForm, TESQuest);
		if (quest)
			eventList = quest->scriptEventList;
	}

	return eventList;
}

static void ConvertLiteralPercents(std::string* str)
{
	UInt32 idx = 0;
	while ((idx = str->find('%', idx)) != -1)
	{
		str->insert(idx, "%");
		idx += 2;
	}
}

static void SkipArgs(UInt8* &scriptData)
{
	switch (*scriptData)
	{
	case 'z':
		scriptData += sizeof(double) + 1;
		break;
	case 'r':
		scriptData += 6;
		break;
	default:
		scriptData += 3;
		break;
	}
}

static void OmitFormatStringArgs(std::string str, FormatStringArgs& args)
{
	//skip any args omitted by the %{ specifier
	UInt32 strIdx = 0;
	while ((strIdx = str.find('%', strIdx)) != -1 && args.HasMoreArgs())
	{
		switch(str[++strIdx])
		{
		case '%':
		case 'q':
		case 'Q':
		case 'r':
		case 'R':
			break;
		case 'c':
		case 'C':
			args.SkipArgs(2);
			break;
		default:
			args.SkipArgs(1);
		}
		strIdx++;
	}
}

//static bool ExtractFormattedString(UInt32 &numArgs, char* buffer, UInt8* &scriptData, Script* scriptObj, ScriptEventList* eventList)
bool ExtractFormattedString(FormatStringArgs& args, char* buffer)
{
	//extracts args based on format string, prints formatted string to buffer
	static const int maxArgs = 20;
	double f[maxArgs] = {0.0};
	UInt32 argIdx = 0;

	std::string fmtString = args.GetFormatString();
	UInt32 strIdx = 0;

	//extract args
	while ((strIdx = fmtString.find('%', strIdx)) != -1)
	{
		char argType = fmtString.at(strIdx+1);
		switch (argType)
		{
		case '%':										//literal %
			strIdx += 2;
			break;
					case 'z':
					case 'Z':										//string variable
						{
							fmtString.erase(strIdx, 2);
							double strID = 0;
							if (!args.Arg(args.kArgType_Float, &strID))
								return false;
			
							const char* toInsert = StringFromStringVar(strID);
			//#if NVSE_CORE
			//				StringVar* insStr = NULL;
			//				insStr = g_StringMap.Get(strID);
			//
			//				if (insStr)
			//					toInsert = insStr->GetCString();
			//#else			// called from a plugin command
			//				if (s_StringVarInterface)
			//					toInsert = s_StringVarInterface->GetString(strID);
			//#endif
							if (toInsert && toInsert[0])
								fmtString.insert(strIdx, toInsert);
							else
								fmtString.insert(strIdx, "NULL");
						}
						break;
		case 'r':										//newline
		case 'R':
			fmtString.erase(strIdx, 2);
			fmtString.insert(strIdx, "\n");
			break;
		case 'e':
		case 'E':										//workaround for CS not accepting empty strings
			fmtString.erase(strIdx, 2);
			break;
		case 'a':
		case 'A':										//character specified by ASCII code
			{
				fmtString.erase(strIdx, 2);
				double fCharCode = 0;
				if (args.Arg(args.kArgType_Float, &fCharCode))
					fmtString.insert(strIdx, 1, (char)fCharCode);
				else
					return false;
			}
			break;
		case 'n':										// name of obj/ref
		case 'N':
			{
				fmtString.erase(strIdx, 2);
				TESForm* form = NULL;
				if (!args.Arg(args.kArgType_Form, &form))
					return false;

				std::string strName(GetFullName(form));
				ConvertLiteralPercents(&strName);
				fmtString.insert(strIdx, strName);
				strIdx += strName.length();
			}
			break;
		case 'i':											//formID
		case 'I':
			{
				fmtString.erase(strIdx, 2);
				TESForm* form = NULL;
				if (!(args.Arg(args.kArgType_Form, &form)))
					return false;
				else if (!form)
					fmtString.insert(strIdx, "00000000");
				else
				{			
					char formID[9];
					sprintf_s(formID, 9, "%08X", form->refID);
					fmtString.insert(strIdx, formID);
				}
			}
			break;
		case 'c':											//named component of another object
		case 'C':											//2 args - object and index
			{
				TESForm* form = NULL;
				if (!args.Arg(args.kArgType_Form, &form))
					return false;

				fmtString.erase(strIdx, 2);
				if (!form)
					fmtString.insert(strIdx, "NULL");
				else
				{
					double objIdx = 0;

					if (!args.Arg(args.kArgType_Float, &objIdx))
						return false;
					else
					{
						std::string strName("");

						switch(form->typeID)
						{
#if 0
							case kFormType_SpellItem:
							case kFormType_EnchantmentItem:
							case kFormType_IngredientItem:
							case kFormType_AlchemyItem:
							{
								MagicItem* magItm = DYNAMIC_CAST(form, TESForm, MagicItem);
								if (!magItm)
									strName = "NULL";
								else
								{
									strName = magItm->list.GetNthEIName(objIdx);
									EffectItem* effItem = magItm->list.ItemAt(objIdx);
									if (effItem && effItem->HasActorValue())
									{
										UInt32 valIdx = strName.find(' ');
										if (valIdx != -1)
										{
											strName.erase(valIdx + 1, strName.length() - valIdx);
											strName.insert(valIdx + 1, std::string(GetActorValueString(effItem->actorValueOrOther)));
										}
									}
								}
							}
							break;
#endif

							case kFormType_TESAmmo:
							{
								TESAmmo	* ammo = DYNAMIC_CAST(form, TESForm, TESAmmo);

								if(!ammo)
									strName = "NULL";	// something is very wrong
								else switch((int)objIdx)
								{
									default:
									case 0:	// full name
										strName = GetFullName(ammo);
										break;

									case 1:	// short name
										strName = ammo->shortName.CStr();
										break;

									case 2:	// abbrev
										strName = ammo->abbreviation.CStr();
										break;
								}
							}
							break;

#if 1	// to be tested
							case kFormType_TESFaction:
							{
								TESFaction* fact = DYNAMIC_CAST(form, TESForm, TESFaction);
								if (!fact)
									strName = "NULL";
								else
								{
									strName = fact->GetNthRankName(objIdx);
								}
							}
							break;
#endif

							default:
								strName = "unknown";
								break;
						}

						ConvertLiteralPercents(&strName);

						fmtString.insert(strIdx, strName);
						strIdx += strName.length();
					}
				}
			}
			break;
		case 'k':
		case 'K':											//DX code
			{
				double keycode = 0;
				fmtString.erase(strIdx, 2);
				if (!args.Arg(args.kArgType_Float, &keycode))
					return false;

				const char* desc = GetDXDescription(keycode);
				fmtString.insert(strIdx, desc);

			}
			break;
		case 'v':
		case 'V':											//actor value
			{
				double actorVal = eActorVal_FalloutMax;
				fmtString.erase(strIdx, 2);
				if (!args.Arg(args.kArgType_Float, &actorVal))
					return false;

				std::string valStr(GetActorValueString(actorVal));
				if (valStr.length())
				{
					for (UInt32 idx = 1; idx < valStr.length(); idx++)
						if (isupper(valStr[idx]))
						{								//insert spaces to make names more presentable
							valStr.insert(idx, " ");
							idx += 2;
						}
				}
				fmtString.insert(strIdx, valStr);
			}
			break;
		case 'p':
		case 'P':											//pronouns
			{
				fmtString.erase(strIdx, 2);
				char pronounType = fmtString[strIdx];
				fmtString.erase(strIdx, 1);
				TESForm* form = NULL;
				if (!args.Arg(args.kArgType_Form, &form))
					return false;

				if (!form)
					fmtString.insert(strIdx, "NULL");
				else
				{			
					TESObjectREFR* refr = DYNAMIC_CAST(form, TESForm, TESObjectREFR);
					if (refr)
						form = refr->baseForm;

					short objType = 0;
					if (form->typeID == kFormType_TESNPC)
					{
						TESActorBaseData* actorBase = DYNAMIC_CAST(form, TESForm, TESActorBaseData);
						objType = (actorBase->IsFemale()) ? 2 : 1;
					}

					switch (pronounType)
					{
					case 'o':
					case 'O':
						if (objType == 1)
							fmtString.insert(strIdx, "him");
						else if (objType == 2)
							fmtString.insert(strIdx, "her");
						else
							fmtString.insert(strIdx, "it");
						break;
					case 's':
					case 'S':
						if (objType == 1)
							fmtString.insert(strIdx, "he");
						else if (objType == 2)
							fmtString.insert(strIdx, "she");
						else
							fmtString.insert(strIdx, "it");
						break;
					case 'p':
					case 'P':
						if (objType == 1)
							fmtString.insert(strIdx, "his");
						else if (objType == 2)
							fmtString.insert(strIdx, "her");
						else
							fmtString.insert(strIdx, "its");
						break;
					default:
						fmtString.insert(strIdx, "NULL");
					}
				}
			}
			break;
		case 'q':
		case 'Q':											//double quote
			fmtString.erase(strIdx, 2);
			fmtString.insert(strIdx, "\"");
			break;
		case '{':											//omit portion of string based on flag param
			{
				fmtString.erase(strIdx, 2);
				double flag = 0;
				if (!args.Arg(args.kArgType_Float, &flag))
					return false;

				UInt32 omitEnd = fmtString.find("%}", strIdx);
				if (omitEnd == -1)
					omitEnd = fmtString.length();

				if (!flag)
				{
					OmitFormatStringArgs(fmtString.substr(strIdx, omitEnd - strIdx), args);
					fmtString.erase(strIdx, omitEnd - strIdx + 2);
				}
				else
					fmtString.erase(omitEnd, 2);
			}
			break;
		case '}':											//in case someone left a stray closing bracket
			fmtString.erase(strIdx, 2);
			break;
		case 'x':											//hex
		case 'X':
			{
				double data = 0;
				if (!args.Arg(args.kArgType_Float, &data))
					return false;

				UInt64* hexArg = (UInt64*)(&f[argIdx++]);
				*hexArg = data;
				fmtString.erase(strIdx, 2);
				char width = 0;
				if (strIdx < fmtString.length())
				{
					if (isdigit(fmtString[strIdx]))	//single-digit width specifier optionally follows %x
					{
						width = fmtString[strIdx];
						fmtString.erase(strIdx, 1);
					}
				}
				fmtString.insert(strIdx, "%0llX");
				if (width)
					fmtString.insert(strIdx + 2, 1, width);
				strIdx++;
			}
			break;
		default:											//float
			{
				double data = 0;
				if (!args.Arg(args.kArgType_Float, &data))
					return false;

				f[argIdx++] = data;
				strIdx++;
			}
			break;
		}
	}

	if (sprintf_s(buffer, kMaxMessageLength - 2, fmtString.c_str(), f[0], f[1], f[2], f[3], f[4], f[5], f[6], f[7], f[8], f[9], f[10], f[11], f[12], f[13], f[14], f[15], f[16], f[17], f[18], f[19]) > 0)
	{
		buffer[kMaxMessageLength-1] = '\0';
		return true;
	}
	else if (fmtString.length() == 0)
	{
		buffer[0] = '\0';
		return true;
	}
	else
		return false;
}

void RegisterStringVarInterface(NVSEStringVarInterface* intfc)
{
	//s_StringVarInterface = intfc;
}

#if NVSE_CORE

//fmtStringPos is index of fmtString param in paramInfo, with first param = 0
bool ExtractFormatStringArgs(UInt32 fmtStringPos, char* buffer, ParamInfo * paramInfo, void * scriptDataIn, UInt32 * scriptDataOffset, Script * scriptObj, ScriptEventList * eventList, UInt32 maxParams, ...)
{
	va_list args;
	va_start(args, maxParams);

	UInt8	* scriptData = ((UInt8 *)scriptDataIn) + *scriptDataOffset;
	SInt16	numArgs = *((SInt16 *)scriptData);
	scriptData += 2;

	if (numArgs < 0) {
		UInt32 offsetPtr = 0;
		bool bResult = ExtractArgsOverride::ExtractFormattedString(paramInfo, scriptData, scriptObj, eventList, &offsetPtr, args,
			fmtStringPos, buffer, maxParams);
		if (!bResult) {
			DEBUG_PRINT("ExtractFormatStringArgs returns false");
		}
		return bResult;
	}

	numArgs -= fmtStringPos + 1;

	bool bExtracted = false;
	if (fmtStringPos > 0)
	{
		bExtracted = v_ExtractArgsEx(fmtStringPos, paramInfo, scriptData, scriptObj, eventList, args);
		if (!bExtracted)
			return false;
	}

	ScriptFormatStringArgs scriptArgs(numArgs, scriptData, scriptObj, eventList);
	bExtracted = ExtractFormattedString(scriptArgs, buffer);

	numArgs = scriptArgs.GetNumArgs();
	scriptData = scriptArgs.GetScriptData();
	//NOTE: if v_ExtractArgsEx was called above, passing args again in second call below = undefined behavior. Needs fixing.
	if (bExtracted && numArgs > 0)			//some optional normal params following format string params
	{
		if ((numArgs + fmtStringPos + 21) > maxParams)		//scripter included too many optional params - adjust to prevent crash
			numArgs = (maxParams - fmtStringPos - 21);

		bExtracted = v_ExtractArgsEx(numArgs, &(paramInfo[fmtStringPos + 21]), scriptData, scriptObj, eventList, args);
	}

	va_end(args);
	return bExtracted;
}

#endif

bool ExtractSetStatementVar(Script* script, ScriptEventList* eventList, void* scriptDataIn, double * outVarData, UInt8* outModIndex, bool shortPath)
{
	/*	DOES NOT WORK WITH FalloutNV, we are going to abuse the stack instead:
	//when script command called as righthand side of a set statement, the script data containing the variable
	//to assign to remains on the stack as arg to a previous function. We can get to it through scriptData in COMMAND_ARGS
	*/
	UInt8* dataStart = (UInt8*)scriptDataIn;	// should be 0x58 (or 0x72 if called with dot syntax)

	if (!((*dataStart == 0x58 || *dataStart == 0x72))) {
		return false;
	}

	// Calculate frame pointer for 4 calls above:
	void* callerFramePointer;
	_asm {
		mov callerFramePointer , ebp
	}
	for (int i = 0; i < 3; i++)
		callerFramePointer = (void*)(*(UInt32*)callerFramePointer);
	if (!shortPath) {
		callerFramePointer = (void*)(*(UInt32*)callerFramePointer);	// sv_Destruct calls us directly, others goes through AssignToStringVar
		callerFramePointer = (void*)(*(UInt32*)callerFramePointer);	// one more added for when multiple commands are grouped (like GetBipedModelPath)
	}

	UInt32 scriptDataPtrAddr = (UInt32)(callerFramePointer) + 0x08;
	UInt32* scriptDataAddr = (UInt32*)scriptDataPtrAddr;
	UInt8* scriptData = (UInt8*)(*scriptDataAddr);

	SInt32 scriptDataOffset = (UInt32)scriptData - (UInt32)(script->data);
	if (scriptDataOffset < 5)
		return false;

	bool bExtracted = false;
	scriptData -= 5;

	switch (*scriptData)			//work backwards from opcode to find lefthand var
	{
	case 'G':						//global
		{
			UInt16 refIdx = *(UInt16*)(scriptData + 1);
			Script::RefVariable* refVar = script->GetVariable(refIdx);
			if (!refVar)
				break;

			TESGlobal* globalVar = DYNAMIC_CAST(refVar->form, TESForm, TESGlobal);
			if (globalVar)
			{
				*outVarData = globalVar->data;
				if (outModIndex)
					*outModIndex = (globalVar->refID >> 24);
				bExtracted = true;
			}
		}
		break;
	case 'l':
	case 'f':
	case 's':
		{
			if (scriptDataOffset >= 8 && *(scriptData - 3) == 'r')	//external var
			{
				UInt16 refIdx = *(UInt16*)(scriptData - 2);
				Script::RefVariable* refVar = script->GetVariable(refIdx);
				if (!refVar)
					break;

				refVar->Resolve(eventList);
				TESForm* refForm = refVar->form;
				if (!refForm)
					break;

				if (refForm->typeID == kFormType_TESObjectREFR)
				{
					TESObjectREFR* refr = DYNAMIC_CAST(refForm, TESForm, TESObjectREFR);
					TESScriptableForm* scriptable = DYNAMIC_CAST(refr->baseForm, TESForm, TESScriptableForm);
					if (scriptable)
					{
						script = scriptable->script;
						eventList = refr->GetEventList();
					}
					else
						break;
				}
				else if (refForm->typeID == kFormType_TESQuest)
				{
					TESScriptableForm* scriptable = DYNAMIC_CAST(refForm, TESForm, TESScriptableForm);
					if (scriptable)
					{
						script = scriptable->script;
						TESQuest* quest = DYNAMIC_CAST(scriptable, TESScriptableForm, TESQuest);
						eventList = quest->scriptEventList;
					}
					else
						break;
				}
				else
					break;
			}

			UInt16 varIdx = *(UInt16*)(scriptData + 1);
			ScriptVar *var = eventList->GetVariable(varIdx);
			if (var)
			{
				*outVarData = var->data.num;
				if (outModIndex)
					*outModIndex = (script->refID >> 24);
				bExtracted = true;
			}
		}
		break;
	default:
		SCRIPT_ASSERT(false, script, "Function must be used within a Set statement");
	}

	return bExtracted;
}

// g_baseActorValueNames is only filled in after oblivion's global initializers run
const char* GetActorValueString(UInt32 actorValue)
{
	char* name = 0;
	if (actorValue <= eActorVal_FalloutMax)
		name = GetActorValueName(actorValue);
	if (!name)
		name = "unknown";

	return name;
}

UInt32 GetActorValueForScript(const char* avStr) 
{
	for (UInt32 i = 0; i <= eActorVal_FalloutMax; i++) {
		char* name = GetActorValueName(i);
		if (_stricmp(avStr, name) == 0)
			return i;
	}

	return eActorVal_NoActorValue;
}

UInt32 GetActorValueForString(const char* strActorVal, bool bForScript)
{
	if (bForScript)
		return GetActorValueForScript(strActorVal);

	for (UInt32 n = 0; n <= eActorVal_FalloutMax; n++) {
		char* name = GetActorValueName(n);
		if (_stricmp(strActorVal, name) == 0)
			return n;
	}
	return eActorVal_NoActorValue;
}

ScriptFormatStringArgs::ScriptFormatStringArgs(UInt32 _numArgs, UInt8* _scriptData, Script* _scriptObj, ScriptEventList* _eventList)
: numArgs(_numArgs), scriptData(_scriptData), scriptObj(_scriptObj), eventList(_eventList)
{
	//extract format string
	UInt16 len = *((UInt16*)scriptData);
	char* szFmt = new char[len+1];
	scriptData += 2;
	memcpy(szFmt, scriptData, len);
	szFmt[len] = '\0';

	scriptData += len;
	fmtString = std::string(std::string(ResolveStringArgument(eventList, szFmt)));
	delete szFmt;
}

std::string ScriptFormatStringArgs::GetFormatString()
{
	return fmtString;
}

bool ScriptFormatStringArgs::HasMoreArgs()
{
	return (numArgs > 0);
}

UInt32 ScriptFormatStringArgs::GetNumArgs()
{
	return numArgs;
}

UInt8* ScriptFormatStringArgs::GetScriptData()
{
	return scriptData;
}

bool ScriptFormatStringArgs::SkipArgs(UInt32 numToSkip)
{
	while (numToSkip--)
	{
		switch (*scriptData)
		{
		case 'z':
			scriptData += sizeof(double) + 1;
			break;
		case 'r':
			scriptData += 6;
			break;
		default:
			scriptData += 3;
			break;
		}

		numArgs--;
	}

	return true;
}

//Log error if expression evaluates to false
bool SCRIPT_ASSERT(bool expr, Script* script, const char * errorMsg, ...)
{
	//	static bool bAlerted = false;			//only alert user on first error
	//	static std::set<UInt32> naughtyScripts;	//one error per script to avoid thrashing
	//
	//	if (!expr && naughtyScripts.find(script->refID) == naughtyScripts.end())
	//	{
	//		const ModEntry ** activeMods = (*g_dataHandler)->GetActiveModList();
	//		UInt8 modIndex = script->GetModIndex();
	//		const ModEntry * modEntry = activeMods[modIndex];
	//
	//		const char * modName;
	//		if (modIndex != 0xFF && modEntry && modEntry->data && modEntry->data->name)
	//			modName = modEntry->data->name;
	//		else
	//			modName = "Unknown";
	//
	////		sprintf_s(errorHeader, sizeof(errorHeader) - 1, "** Error: Script %08X in file \"%s\" **", script->refID, modName);
	////		_MESSAGE("%s", errorHeader);
	//		_MESSAGE("** Script Error: Script %08x in file \"%s\" **", script->refID, modName);
	//
	//		va_list args;
	//		va_start(args, errorMsg);
	//
	//		char errorBuf[512];
	//		vsprintf_s(errorBuf, sizeof(errorBuf) - 1, errorMsg, args);
	//		va_end(args);
	//
	//		gLog.Indent();
	//		_MESSAGE("%s", errorBuf);
	//		gLog.Outdent();
	//
	//		if (!bAlerted)
	//		{
	//			MessageBoxAlert("NVSE has detected a script error. \n\nPlease check nvse.log for details.");
	//			bAlerted = true;
	//		}
	//
	//		naughtyScripts.insert(script->refID);
	//	}
	return expr;
}

bool ScriptFormatStringArgs::Arg(FormatStringArgs::argType asType, void * outResult)
{
	if (!SCRIPT_ASSERT((numArgs > 0), scriptObj, "Too few args for format specifier"))
		return false;

	numArgs--;

	switch (asType)
	{
	case kArgType_Float:
		{
			double data = 0;
			if (ExtractFloat(&data, scriptData, scriptObj, eventList))
			{
				*((double*)outResult) = data;
				return true;
			}
		}
		break;
	case kArgType_Form:
		{
			TESForm* form = ExtractFormFromFloat(scriptData, scriptObj, eventList);
			*((TESForm**)outResult) = form;
			return true;
		}
	}

	return false;
}

void ShowCompilerError(ScriptLineBuffer* lineBuf, const char * fmt, ...)
{

	char errorHeader[0x400];
	UInt32 offset = sprintf_s(errorHeader, 0x400, "Error on line %d\n\n", lineBuf->lineNumber);

	va_list	args;
	va_start(args, fmt);

	char	errorMsg[0x200];
	vsprintf_s(errorMsg, 0x200, fmt, args);

	strcat_s(errorHeader, 0x400, errorMsg);
	Console_Print(errorHeader);
	//_MESSAGE(errorHeader);

	va_end(args);
}

UInt32 GetActorValueMax(UInt32 actorValueCode) {
	switch (actorValueCode ) {
		case eActorVal_Aggression:			return   3; break;
		case eActorVal_Confidence:			return   4; break;
		case eActorVal_Energy:				return 100; break;
		case eActorVal_Responsibility:		return 100; break;
		case eActorVal_Mood:				return   8; break;

		case eActorVal_Strength:			return  10; break;
		case eActorVal_Perception:			return  10; break;
		case eActorVal_Endurance:			return  10; break;
		case eActorVal_Charisma:			return  10; break;
		case eActorVal_Intelligence:		return  10; break;
		case eActorVal_Agility:				return  10; break;
		case eActorVal_Luck:				return  10; break;

		case eActorVal_ActionPoints:		return   1; break;
		case eActorVal_CarryWeight:			return   1; break;
		case eActorVal_CritChance:			return 100; break;
		case eActorVal_HealRate:			return   1; break;
		case eActorVal_Health:				return   1; break;
		case eActorVal_MeleeDamage:			return   1; break;
		case eActorVal_DamageResistance:	return   1; break;
		case eActorVal_PoisonResistance:	return   1; break;
		case eActorVal_RadResistance:		return   1; break;
		case eActorVal_SpeedMultiplier:		return   1; break;
		case eActorVal_Fatigue:				return   1; break;
		case eActorVal_Karma:				return   1; break;
		case eActorVal_XP:					return   1; break;

		case eActorVal_Head:				return 100; break;
		case eActorVal_Torso:				return 100; break;
		case eActorVal_LeftArm:				return 100; break;
		case eActorVal_RightArm:			return 100; break;
		case eActorVal_LeftLeg:				return 100; break;
		case eActorVal_RightLeg:			return 100; break;
		case eActorVal_Brain:				return 100; break;

		case eActorVal_Barter:				return 100; break;
		case eActorVal_BigGuns:				return 100; break;
		case eActorVal_EnergyWeapons:		return 100; break;
		case eActorVal_Explosives:			return 100; break;
		case eActorVal_Lockpick:			return 100; break;
		case eActorVal_Medicine:			return 100; break;
		case eActorVal_MeleeWeapons:		return 100; break;
		case eActorVal_Repair:				return 100; break;
		case eActorVal_Science:				return 100; break;
		case eActorVal_Guns:				return 100; break;
		case eActorVal_Sneak:				return 100; break;
		case eActorVal_Speech:				return 100; break;
		case eActorVal_Survival:			return 100; break;
		case eActorVal_Unarmed:				return 100; break;

		case eActorVal_InventoryWeight:		return   1; break;
		case eActorVal_Paralysis:			return   1; break;
		case eActorVal_Invisibility:		return   1; break;
		case eActorVal_Chameleon:			return   1; break;
		case eActorVal_NightEye:			return   1; break;
		case eActorVal_Turbo:				return   1; break;
		case eActorVal_FireResistance:		return   1; break;
		case eActorVal_WaterBreathing:		return   1; break;
		case eActorVal_RadLevel:			return   1; break;
		case eActorVal_BloodyMess:			return   1; break;
		case eActorVal_UnarmedDamage:		return   1; break;
		case eActorVal_Assistance:			return   2; break;

		case eActorVal_ElectricResistance:	return   1; break;

		case eActorVal_EnergyResistance:	return   1; break;
		case eActorVal_EMPResistance:		return   1; break;
		case eActorVal_Var1Medical:			return   1; break;
		case eActorVal_Var2:				return   1; break;
		case eActorVal_Var3:				return   1; break;
		case eActorVal_Var4:				return   1; break;
		case eActorVal_Var5:				return   1; break;
		case eActorVal_Var6:				return   1; break;
		case eActorVal_Var7:				return   1; break;
		case eActorVal_Var8:				return   1; break;
		case eActorVal_Var9:				return   1; break;
		case eActorVal_Var10:				return   1; break;

		case eActorVal_IgnoreCrippledLimbs:	return   1; break;
		case eActorVal_Dehydration:			return   1; break;
		case eActorVal_Hunger:				return   1; break;
		case eActorVal_Sleepdeprevation:	return   1; break;
		case eActorVal_Damagethreshold:		return   1; break;
		default: return 1;
	}
}

#endif
