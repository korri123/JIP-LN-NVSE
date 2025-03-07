#pragma once

#define CALL_EAX(addr) __asm mov eax, addr __asm call eax
#define JMP_EAX(addr)  __asm mov eax, addr __asm jmp eax
#define JMP_EDX(addr)  __asm mov edx, addr __asm jmp edx

// These are used for 10h aligning segments in ASM code (massive performance gain, particularly with loops).
#define EMIT(bt) __asm _emit bt
#define NOP_0x1 EMIT(0x90)
//	"\x90"
#define NOP_0x2 EMIT(0x66) NOP_0x1
//	"\x66\x90"
#define NOP_0x3 EMIT(0x0F) EMIT(0x1F) EMIT(0x00)
//	"\x0F\x1F\x00"
#define NOP_0x4 EMIT(0x0F) EMIT(0x1F) EMIT(0x40) EMIT(0x00)
//	"\x0F\x1F\x40\x00"
#define NOP_0x5 EMIT(0x0F) EMIT(0x1F) EMIT(0x44) EMIT(0x00) EMIT(0x00)
//	"\x0F\x1F\x44\x00\x00"
#define NOP_0x6 EMIT(0x66) NOP_0x5
//	"\x66\x0F\x1F\x44\x00\x00"
#define NOP_0x7 EMIT(0x0F) EMIT(0x1F) EMIT(0x80) EMIT(0x00) EMIT(0x00) EMIT(0x00) EMIT(0x00)
//	"\x0F\x1F\x80\x00\x00\x00\x00"
#define NOP_0x8 EMIT(0x0F) EMIT(0x1F) EMIT(0x84) EMIT(0x00) EMIT(0x00) EMIT(0x00) EMIT(0x00) EMIT(0x00)
//	"\x0F\x1F\x84\x00\x00\x00\x00\x00"
#define NOP_0x9 EMIT(0x66) NOP_0x8
//	"\x66\x0F\x1F\x84\x00\x00\x00\x00\x00"
#define NOP_0xA EMIT(0x66) NOP_0x9
//	"\x66\x66\x0F\x1F\x84\x00\x00\x00\x00\x00"
#define NOP_0xB EMIT(0x66) NOP_0xA
//	"\x66\x66\x66\x0F\x1F\x84\x00\x00\x00\x00\x00"
#define NOP_0xC NOP_0x8 NOP_0x4
#define NOP_0xD NOP_0x8 NOP_0x5
#define NOP_0xE NOP_0x7 NOP_0x7
#define NOP_0xF NOP_0x8 NOP_0x7

#define GAME_HEAP_ALLOC __asm mov ecx, 0x11F6238 CALL_EAX(0xAA3E40)
#define GAME_HEAP_FREE  __asm mov ecx, 0x11F6238 CALL_EAX(0xAA4060)

#define MEMBER_FN_PREFIX(className)	\
	typedef className _MEMBER_FN_BASE_TYPE

#define DEFINE_MEMBER_FN_LONG(className, functionName, retnType, address, ...)		\
	typedef retnType (className::* _##functionName##_type)(__VA_ARGS__);			\
																					\
	inline _##functionName##_type * _##functionName##_GetPtr(void)					\
	{																				\
		static const UInt32 _address = address;										\
		return (_##functionName##_type *)&_address;									\
	}

#define DEFINE_MEMBER_FN(functionName, retnType, address, ...)	\
	DEFINE_MEMBER_FN_LONG(_MEMBER_FN_BASE_TYPE, functionName, retnType, address, __VA_ARGS__)

#define CALL_MEMBER_FN(obj, fn)	\
	((*(obj)).*(*((obj)->_##fn##_GetPtr())))

#define SIZEOF_ARRAY(arrayName, elementType) (sizeof(arrayName) / sizeof(elementType))

template <typename T_Ret = void, typename ...Args>
__forceinline T_Ret ThisCall(UInt32 _addr, void *_this, Args ...args)
{
	return ((T_Ret (__thiscall *)(void*, Args...))_addr)(_this, std::forward<Args>(args)...);
}

template <typename T_Ret = void, typename ...Args>
__forceinline T_Ret StdCall(UInt32 _addr, Args ...args)
{
	return ((T_Ret (__stdcall *)(Args...))_addr)(std::forward<Args>(args)...);
}

template <typename T_Ret = void, typename ...Args>
__forceinline T_Ret CdeclCall(UInt32 _addr, Args ...args)
{
	return ((T_Ret (__cdecl *)(Args...))_addr)(std::forward<Args>(args)...);
}

#define GameHeapAlloc(size) ThisCall<void*, UInt32>(0xAA3E40, (void*)0x11F6238, size)
#define GameHeapFree(ptr) ThisCall<void, void*>(0xAA4060, (void*)0x11F6238, ptr)

#define GetRandomInt(n) ThisCall<SInt32, SInt32>(0xAA5230, (void*)0x11C4180, n)
#define GetRandomIntInRange(iMin, iMax) (GetRandomInt(iMax - iMin) + iMin)

#define LOG_HOOKS 0

static const double
kDblPId180 = 0.017453292519943295,
kDblPId2 = 1.57079632679489662,
kDbl180dPI = 57.29577951308232088;

static const float
kFlt1d1000 = 0.001F,
kFlt1d200 = 0.005F,
kFlt1d100 = 0.01F,
kFltPId180 = 0.01745329238F,
kFlt1d10 = 0.1F,
kFltHalf = 0.5F,
kFltOne = 1.0F,
kFlt10 = 10.0F,
kFlt180dPI = 57.2957795F,
kFlt100 = 100.0F,
kFlt200 = 200.0F,
kFlt1000 = 1000.0F,
kFltMax = FLT_MAX;

typedef void* (__cdecl *memcpy_t)(void*, const void*, size_t);
extern memcpy_t MemCopy, MemMove;

//	Workaround for bypassing the compiler calling the d'tor on function-scope objects.
template <typename T> class TempObject
{
	friend T;

	struct Buffer
	{
		UInt8	bytes[sizeof(T)];
	}
	objData;

public:
	TempObject() {Reset();}
	TempObject(const T &src) {memcpy((void*)&objData, (const void*)&src, sizeof(T));}

	void Reset() {new ((T*)&objData) T();}

	T& operator()() {return *(T*)&objData;}

	TempObject& operator=(const T &rhs)
	{
		memcpy((void*)&objData, (const void*)&rhs, sizeof(T));
		return *this;
	}
	TempObject& operator=(const TempObject &rhs)
	{
		memcpy((void*)&objData, (const void*)&rhs.objData, sizeof(T));
		return *this;
	}
};

//	Swap lhs and rhs, bypassing operator=
template <typename T> __forceinline void RawSwap(const T &lhs, const T &rhs)
{
	UInt8 buffer[sizeof(T)];
	memcpy((void*)buffer, (const void*)&lhs, sizeof(T));
	memcpy((void*)&lhs, (const void*)&rhs, sizeof(T));
	memcpy((void*)&rhs, (const void*)buffer, sizeof(T));
}

class CriticalSection
{
	CRITICAL_SECTION	critSection;

public:
	CriticalSection() {InitializeCriticalSection(&critSection);}
	~CriticalSection() {DeleteCriticalSection(&critSection);}

	void Enter() {EnterCriticalSection(&critSection);}
	void Leave() {LeaveCriticalSection(&critSection);}
	bool TryEnter() {return TryEnterCriticalSection(&critSection) != 0;}
};

class PrimitiveCS
{
	UInt32	owningThread;

public:
	PrimitiveCS() : owningThread(0) {}

	PrimitiveCS *Enter();
	__forceinline void Leave() {owningThread = 0;}
};

class LightCS
{
	UInt32	owningThread;
	UInt32	enterCount;

public:
	LightCS() : owningThread(0), enterCount(0) {}

	void Enter();
	__forceinline void Leave()
	{
		if (!--enterCount)
			owningThread = 0;
	}
};

template <typename T_CS> class ScopedLock
{
	T_CS		*cs;

public:
	ScopedLock(T_CS *_cs) : cs(_cs) {cs->Enter();}
	~ScopedLock() {cs->Leave();}
};

union FunctionArg
{
	void		*pVal;
	float		fVal;
	UInt32		uVal;
	SInt32		iVal;

	FunctionArg& operator=(void *other)
	{
		pVal = other;
		return *this;
	}
	FunctionArg& operator=(float other)
	{
		fVal = other;
		return *this;
	}
	FunctionArg& operator=(UInt32 other)
	{
		uVal = other;
		return *this;
	}
	FunctionArg& operator=(SInt32 other)
	{
		iVal = other;
		return *this;
	}
};

TESForm* __stdcall LookupFormByRefID(UInt32 refID);

union Coordinate
{
	UInt32		xy;
	struct
	{
		SInt16	y;
		SInt16	x;
	};

	Coordinate() {}
	Coordinate(SInt16 _x, SInt16 _y) : x(_x), y(_y) {}
	Coordinate(UInt32 _xy) : xy(_xy) {}
	Coordinate(SInt32 *_x) : x(_x[0]), y(_x[1]) {}

	inline Coordinate& operator=(const Coordinate &rhs)
	{
		xy = rhs.xy;
		return *this;
	}
	inline Coordinate& operator=(UInt32 rhs)
	{
		xy = rhs;
		return *this;
	}

	inline bool operator==(const Coordinate &rhs) {return xy == rhs.xy;}
	inline bool operator!=(const Coordinate &rhs) {return xy != rhs.xy;}

	inline Coordinate operator+(const Coordinate &rhs)
	{
		return Coordinate(x + rhs.x, y + rhs.y);
	}

	inline operator UInt32() const {return xy;}
};

template <typename T1, typename T2> inline T1 GetMin(T1 value1, T2 value2)
{
	return (value1 < value2) ? value1 : value2;
}

template <typename T1, typename T2> inline T1 GetMax(T1 value1, T2 value2)
{
	return (value1 > value2) ? value1 : value2;
}

template <typename T> inline T sqr(T value)
{
	return value * value;
}

extern const UInt8 kLwrCaseConverter[], kUprCaseConverter[];

UInt32 __vectorcall cvtd2ui(double value);

double __vectorcall cvtui2d(UInt32 value);
void __fastcall cvtui2d(UInt32 value, double *result);

int __vectorcall ifloor(float value);

int __vectorcall iceil(float value);

__forceinline int iround(float value)
{
	return _mm_cvt_ss2si(_mm_load_ss(&value));
}

char *GetStrArgBuffer();

void __fastcall NiReleaseObject(NiRefObject *toRelease);

NiRefObject** __stdcall NiReleaseAddRef(void *toRelease, NiRefObject *toAdd);

UInt32 __fastcall RGBHexToDec(UInt32 rgb);

UInt32 __fastcall RGBDecToHex(UInt32 rgb);

UInt32 __fastcall StrLen(const char *str);

bool __fastcall MemCmp(const void *ptr1, const void *ptr2, UInt32 bsize);

void __fastcall MemZero(void *dest, UInt32 bsize);

char* __fastcall StrCopy(char *dest, const char *src);

char* __fastcall StrNCopy(char *dest, const char *src, UInt32 length);

char* __fastcall StrLenCopy(char *dest, const char *src, UInt32 length);

char* __fastcall StrCat(char *dest, const char *src);

char __fastcall StrCompare(const char *lstr, const char *rstr);

char __fastcall StrBeginsCS(const char *lstr, const char *rstr);

char __fastcall StrBeginsCI(const char *lstr, const char *rstr);

void __fastcall FixPath(char *str);

void __fastcall StrToLower(char *str);

void __fastcall StrToUpper(char *str);

void __fastcall ReplaceChr(char *str, char from, char to);

char* __fastcall FindChr(const char *str, char chr);

char* __fastcall FindChrR(const char *str, UInt32 length, char chr);

char* __fastcall SubStrCI(const char *srcStr, const char *subStr);

char* __fastcall SlashPos(const char *str);

char* __fastcall SlashPosR(const char *str);

char* __fastcall GetNextToken(char *str, char delim);

char* __fastcall GetNextToken(char *str, const char *delims);

char* __fastcall CopyString(const char *key);

char* __fastcall CopyCString(const char *src);

char* __fastcall IntToStr(char *str, int num);

char* __vectorcall FltToStr(char *str, double value);

int __fastcall StrToInt(const char *str);

UInt32 __fastcall StrToUInt(const char *str);

double __vectorcall StrToDbl(const char *str);

char* __fastcall UIntToHex(char *str, UInt32 num);

UInt32 __fastcall HexToUInt(const char *str);

class TempCString
{
	char		*str;

	void Clear()
	{
		if (!str) return;
		free(str);
		str = NULL;
	}

public:
	TempCString() : str(NULL) {}
	TempCString(UInt32 initSize)
	{
		str = (char*)malloc(initSize + 1);
		*str = 0;
	}
	TempCString(const char *src)
	{
		str = src ? CopyString(src) : NULL;
	}
	TempCString(const TempCString &src)
	{
		str = src.str ? CopyString(src.str) : NULL;
	}

	~TempCString() {Clear();}

	const char *CString() const {return str ? str : "";}
	char *Data() {return str;}

	bool operator==(const TempCString &rhs) {return !StrCompare(str, rhs.str);}
	bool operator<(const TempCString &rhs) {return StrCompare(str, rhs.str) < 0;}
	bool operator>(const TempCString &rhs) {return StrCompare(str, rhs.str) > 0;}

	char& operator[](UInt32 index) {return str[index];}

	TempCString& operator=(const TempCString &rhs)
	{
		if (this != &rhs)
		{
			Clear();
			if (rhs.str)
				str = CopyString(rhs.str);
		}
		return *this;
	}
	TempCString& operator=(const char *srcStr)
	{
		if (str != srcStr)
		{
			Clear();
			if (srcStr)
				str = CopyString(srcStr);
		}
		return *this;
	}
};

class DString
{
	char		*str;
	UInt16		alloc;
	UInt16		length;

	DString(char *_str, UInt16 _length, UInt16 _alloc) : str(_str), alloc(_alloc), length(_length) {}

public:
	DString() : str(NULL) {*(UInt32*)&alloc = 0;}
	DString(const char *from);
	DString(const DString &from);
	DString(UInt16 _alloc);

	~DString()
	{
		if (str)
		{
			Pool_Free(str, alloc);
			str = NULL;
			*(UInt32*)&alloc = 0;
		}
	}

	const char *CString() const {return str ? str : "";}
	char *Data() {return str;}

	UInt32 Size() const {return length;}
	bool Empty() const {return !length;}
	UInt32 AllocSize() const {return alloc;}

	void Reserve(UInt16 size);

	void Clear()
	{
		if (length)
		{
			length = 0;
			*str = 0;
		}
	}

	char At(UInt16 index) const {return (index < length) ? str[index] : 0;}

	char& operator[](UInt16 index) const {return str[index];}

	DString& operator=(const char *other);
	DString& operator=(const DString &other);

	DString& operator+=(char chr);
	DString& operator+=(const char *other);
	DString& operator+=(const DString &other);

	bool operator==(const char *other);
	bool operator==(const DString &other);

	DString& Insert(UInt16 index, char chr);
	DString& Insert(UInt16 index, const char *other);
	DString& Insert(UInt16 index, const DString &other);

	DString& Erase(UInt16 index, UInt16 count);

	DString& Replace(UInt16 bgnIdx, const char *other);
	DString& Replace(UInt16 bgnIdx, const DString &other);

	DString SubString(UInt16 bgnIdx, UInt16 endIdx);

	DString ToLower();
	DString ToUpper();

	friend DString operator+(const DString &lStr, char rChr);
	friend DString operator+(const DString &lStr, const char *rStr);
	friend DString operator+(const char *lStr, const DString &rStr);
};

#define AUX_BUFFER_INIT_SIZE 0x8000

class AuxBuffer
{
	UInt8		*ptr;
	UInt32		size;

public:
	AuxBuffer() : ptr(NULL), size(AUX_BUFFER_INIT_SIZE) {}

	static UInt8 *Get(UInt32 bufIdx, UInt32 reqSize);
};

bool __fastcall FileExists(const char *filePath);

class FileStream
{
	FILE		*theFile;

public:
	FileStream() : theFile(NULL) {}
	FileStream(const char *filePath);
	FileStream(const char *filePath, UInt32 inOffset);
	~FileStream() {if (theFile) fclose(theFile);}

	bool Open(const char *filePath);
	bool OpenAt(const char *filePath, UInt32 inOffset);
	bool OpenWrite(char *filePath, bool append);
	void SetOffset(UInt32 inOffset);

	void Close()
	{
		fclose(theFile);
		theFile = NULL;
	}

	explicit operator bool() const {return theFile != NULL;}

	UInt32 GetLength();
	char ReadChar();
	void ReadBuf(void *outData, UInt32 inLength);
	void WriteChar(char chr);
	void WriteStr(const char *inStr);
	void WriteBuf(const void *inData, UInt32 inLength);
	int WriteFmtStr(const char *fmt, ...);

	static void MakeAllDirs(char *fullPath);
};

extern const char kIndentLevelStr[];

class DebugLog
{
	FILE			*theFile;
	UInt32			indent;

public:
	DebugLog() : theFile(NULL), indent(40) {}
	~DebugLog() {if (theFile) fclose(theFile);}

	bool Create(const char *filePath);
	void Message(const char *msgStr);
	void FmtMessage(const char *fmt, va_list args);
	void Indent() {if (indent) indent--;}
	void Outdent() {if (indent < 40) indent++;}
};

extern DebugLog s_log, s_debug;

void PrintLog(const char *fmt, ...);
void PrintDebug(const char *fmt, ...);

class LineIterator
{
	char	*dataPtr;

public:
	LineIterator(const char *filePath, char *buffer);

	explicit operator bool() const {return *dataPtr != 3;}
	void operator++();

	char *Get() {return dataPtr;}
};

class DirectoryIterator
{
	HANDLE				handle;
	WIN32_FIND_DATA		fndData;

public:
	DirectoryIterator(const char *path) : handle(FindFirstFile(path, &fndData)) {}
	~DirectoryIterator() {Close();}

	bool IsFile() const {return !(fndData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);}
	bool IsFolder() const
	{
		if (IsFile())
			return false;
		if (fndData.cFileName[0] != '.')
			return true;
		if (fndData.cFileName[1] != '.')
			return fndData.cFileName[1] != 0;
		return fndData.cFileName[2] != 0;
	}
	const char *Get() const {return fndData.cFileName;}
	void Close()
	{
		if (handle != INVALID_HANDLE_VALUE)
		{
			FindClose(handle);
			handle = INVALID_HANDLE_VALUE;
		}
	}

	explicit operator bool() const {return handle != INVALID_HANDLE_VALUE;}
	void operator++() {if (!FindNextFile(handle, &fndData)) Close();}
};

UInt32 __fastcall FileToBuffer(const char *filePath, char *buffer);

void __stdcall StoreOriginalData(UInt32 addr, UInt8 size);

void __stdcall SafeWrite8(UInt32 addr, UInt32 data);
void __stdcall SafeWrite16(UInt32 addr, UInt32 data);
void __stdcall SafeWrite32(UInt32 addr, UInt32 data);
void __stdcall SafeWriteBuf(UInt32 addr, void * data, UInt32 len);

#define SAFE_WRITE_BUF(addr, data) SafeWriteBuf(addr, data, sizeof(data) - 1)

// 5 bytes
void __stdcall WriteRelJump(UInt32 jumpSrc, UInt32 jumpTgt);
void __stdcall WriteRelCall(UInt32 jumpSrc, UInt32 jumpTgt);

// 10 bytes
void __stdcall WritePushRetRelJump(UInt32 baseAddr, UInt32 retAddr, UInt32 jumpTgt);

void __fastcall GetTimeStamp(char *buffer);

UInt64 GetTimeMs64();

const char* __fastcall GetDXDescription(UInt32 keyID);

UInt32 __fastcall ByteSwap(UInt32 dword);

void DumpMemImg(void *data, UInt32 size, UInt8 extra = 0);