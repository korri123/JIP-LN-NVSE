#pragma once

typedef UInt32 (*_TraitNameToID)(const char *traitName);
extern const _TraitNameToID TraitNameToID;

const char * TraitIDToName(int id);	// slow

//	Tile			
//		TileRect		3C
//			TileMenu	40
//		TileImage		48
//		TileText		4C
//		Tile3D			50

enum
{
	kTileValue_x =					0xFA1,
	kTileValue_y,
	kTileValue_visible,
	kTileValue_class,
	kTileValue_clipwindow =			0xFA6,
	kTileValue_stackingtype,
	kTileValue_locus,
	kTileValue_alpha,
	kTileValue_id,
	kTileValue_disablefade,
	kTileValue_listindex,
	kTileValue_depth,
	kTileValue_clips,
	kTileValue_target,
	kTileValue_height,
	kTileValue_width,
	kTileValue_red,
	kTileValue_green,
	kTileValue_blue,
	kTileValue_tile,
	kTileValue_childcount,
	kTileValue_child_count =		kTileValue_childcount,
	kTileValue_justify,
	kTileValue_zoom,
	kTileValue_font,
	kTileValue_wrapwidth,
	kTileValue_wraplimit,
	kTileValue_wraplines,
	kTileValue_pagenum,
	kTileValue_ishtml,
	kTileValue_cropoffsety,
	kTileValue_cropy =				kTileValue_cropoffsety,
	kTileValue_cropoffsetx,
	kTileValue_cropx =				kTileValue_cropoffsetx,
	kTileValue_menufade,
	kTileValue_explorefade,
	kTileValue_mouseover,
	kTileValue_string,
	kTileValue_shiftclicked,
	kTileValue_clicked =			0xFC7,
	kTileValue_clicksound =			0xFCB,
	kTileValue_filename,
	kTileValue_filewidth,
	kTileValue_fileheight,
	kTileValue_repeatvertical,
	kTileValue_repeathorizontal,
	kTileValue_animation =			0xFD2,
	kTileValue_linecount =			0xDD4,
	kTileValue_pagecount,
	kTileValue_xdefault,
	kTileValue_xup,
	kTileValue_xdown,
	kTileValue_xleft,
	kTileValue_xright,
	kTileValue_xbuttona =			0xFDD,
	kTileValue_xbuttonb,
	kTileValue_xbuttonx,
	kTileValue_xbuttony,
	kTileValue_xbuttonlt,
	kTileValue_xbuttonrt,
	kTileValue_xbuttonlb,
	kTileValue_xbuttonrb,
	kTileValue_xbuttonstart =		0xFE7,
	kTileValue_mouseoversound,
	kTileValue_draggable,
	kTileValue_dragstartx,
	kTileValue_dragstarty,
	kTileValue_dragoffsetx,
	kTileValue_dragoffsety,
	kTileValue_dragdeltax,
	kTileValue_dragdeltay,
	kTileValue_dragx,
	kTileValue_dragy,
	kTileValue_wheelable,
	kTileValue_wheelmoved,
	kTileValue_systemcolor,
	kTileValue_brightness,
	kTileValue_linegap =			0xFF7,
	kTileValue_resolutionconverter,
	kTileValue_texatlas,
	kTileValue_rotateangle,
	kTileValue_rotateaxisx,
	kTileValue_rotateaxisy,

	kTileValue_user0 =				0x1004,
	kTileValue_user1,
	kTileValue_user2,
	kTileValue_user3,
	kTileValue_user4,
	kTileValue_user5,
	kTileValue_user6,
	kTileValue_user7,
	kTileValue_user8,
	kTileValue_user9,
	kTileValue_user10,
	kTileValue_user11,
	kTileValue_user12,
	kTileValue_user13,
	kTileValue_user14,
	kTileValue_user15,
	kTileValue_user16,

	kTileValue_max
};

// 38+
class Tile
{
public:
	enum eTileID
	{
		kTileID_rect = 0x0385,
		kTileID_image,
		kTileID_text,
		kTileID_3D,
		kTileID_nif = kTileID_3D,
		kTileID_menu,

		// Not a Tile descendend
		kTileID_hotrect,
		kTileID_window,
		// This one descend from TileImage
		kTileID_radial,

		kTileID_max
	};

	MEMBER_FN_PREFIX(Tile);
	DEFINE_MEMBER_FN(SetStringValue, void, 0x00A01350, UInt32 valueID, const char* str, bool bPropagate);
	DEFINE_MEMBER_FN(SetFloatValue, void, 0x00A012D0, UInt32 valueID, float num, bool bPropagate);

	virtual Tile		*Destroy(bool doFree);
	virtual void		Init(Tile *parent, const char *name, Tile *replacedChild);
	virtual NiNode		*CalcNode(void);
	virtual UInt32		GetType(void);		// returns one of kTileValue_XXX
	virtual const char	*GetTypeStr(void);	// 4-byte id
	virtual bool		Unk_05(UInt32 arg0, UInt32 arg1);
	virtual UInt32		UpdateField(UInt32 valueID, float floatValue, const char *strValue);
	virtual void		Unk_07(void);
	virtual UInt32		Unk_08(void);
	virtual void		Unk_09(UInt32 arg0, UInt32 arg1, UInt32 arg2);

	struct Value;

	enum ActionType
	{
		kAction_copy = 0x7D0,
		kAction_add,
		kAction_sub,
		kAction_mul,
		kAction_div,
		kAction_min,
		kAction_max,
		kAction_mod,
		kAction_floor,
		kAction_ceil,
		kAction_abs,
		kAction_round,
		kAction_gt,
		kAction_gte,
		kAction_eq,
		kAction_neq,
		kAction_lt,
		kAction_lte,
		kAction_and,
		kAction_or,
		kAction_not,
		kAction_onlyif,
		kAction_onlyifnot,
		kAction_ref,
		kAction_begin,
		kAction_end,
	};

	// 0C
	class Action
	{
	public:
		virtual float	GetFloat();
		virtual Value	*GetValue();

		UInt32		type;		// 04
		Action		*next;		// 08
	};

	// 10
	class RefValueAction : public Action
	{
	public:
		Value		*tileVal;	// 0C
	};

	// 10
	class FloatAction : public Action
	{
	public:
		float		value;		// 0C
	};

	// 14
	struct Value
	{
		UInt32		id;			// 00
		Tile		*parent;	// 04
		float		num;		// 08
		char		*str;		// 0C
		Action		*action;	// 10

		__forceinline void SetFloat(float fltVal, bool bPropagate = true)
		{
			ThisCall(0xA0A270, this, fltVal, bPropagate);
		}
		__forceinline void SetString(const char *strVal, bool bPropagate = true)
		{
			ThisCall(0xA0A300, this, strVal, bPropagate);
		}
	};

	DList<Tile>					children;		// 04
	BSSimpleArray<Value*>		values;			// 10
	String						name;			// 20
	Tile						*parent;		// 28
	NiNode						*node;			// 2C
	UInt32						flags;			// 30
	UInt8						unk34;			// 34
	UInt8						unk35;			// 35
	UInt8						pad35[2];		// 36

	static UInt32 TraitNameToID(const char *traitName);
	static UInt32 TraitNameToIDAdd(const char *traitName);
	Value *GetValue(UInt32 typeID);
	Value *GetValueName(const char *valueName);
	__forceinline float GetValueFloat(UInt32 id)
	{
		return ThisCall<float>(0xA011B0, this, id);
	}
	DListNode<Tile> *GetNthChild(UInt32 index);
	Tile *GetChild(const char *childName);
	Tile *GetComponent(const char *componentTile, const char **trait);
	Tile *GetComponentTile(const char *componentTile);
	Value *GetComponentValue(const char *componentPath);
	__forceinline Tile *ReadXML(const char *xmlPath)
	{
		return ThisCall<Tile*>(0xA01B00, this, xmlPath);
	}
	char *GetComponentFullName(char *resStr);
	__forceinline void SetFloat(UInt32 id, float fltVal, bool bPropagate = true)
	{
		ThisCall(0xA012D0, this, id, fltVal, bPropagate);
	}
	__forceinline void SetString(UInt32 id, const char *strVal, bool bPropagate = true)
	{
		ThisCall(0xA01350, this, id, strVal, bPropagate);
	}
	__forceinline void GradualSetFloat(UInt32 id, float startVal, float endVal, float seconds, UInt32 changeMode = 0)
	{
		ThisCall(0xA07C60, this, id, startVal, endVal, seconds, changeMode);
	}
	Menu *GetParentMenu();
	void DestroyAllChildren();
	void PokeValue(UInt32 valueID);
	void FakeClick();

	void Dump();
};

// 1C
struct GradualSetFloat
{
	float		startValue;		// 00
	float		endValue;		// 04
	UInt32		startTimeMS;	// 08
	float		durationMS;		// 0C
	UInt32		valueID;		// 10
	Tile		*tile;			// 14
	UInt32		changeMode;		// 18	0-4, see 0xA081B5
	//	0		From start to end in duration
	//	1		From start to end to start, in duration, perpetually
	//	2		From start to end to start, in duration, 4 times
	//	3		From start to end to start, 7 times, in duration
	//	4		From start to end in duration/6, end for duration*2/3, from end to start in duration/6
	//	5**		From start to end, in duration, perpetually (suitable for image rotation)
};

// 3C
class TileRect : public Tile
{
public:
	UInt32	unk38;	// 38
};

// 40
class TileMenu : public TileRect
{
public:
	Menu	*menu;	// 3C
};

// 48
class TileImage : public Tile
{
public:
	float				flt038;			// 38
	UInt32				unk03C;			// 3C
	TileShaderProperty	*shaderProp;	// 40
	UInt8				byt044;			// 44
	UInt8				fill[3];		// 45-47
};

class TileText : public Tile
{
public:
};