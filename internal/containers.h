#pragma once

#define MAP_DEFAULT_ALLOC			8
#define MAP_DEFAULT_BUCKET_COUNT	8
#define MAP_MAX_BUCKET_COUNT		0x40000
#define VECTOR_DEFAULT_ALLOC		8

void* __fastcall Pool_Alloc_Buckets(UInt32 numBuckets);
UInt32 __fastcall AlignBucketCount(UInt32 count);
UInt32 __fastcall StrHashCS(const char *inKey);
UInt32 __fastcall StrHashCI(const char *inKey);

template <typename T_Data> class Stack
{
	using Data_Arg = std::conditional_t<std::is_scalar_v<T_Data>, T_Data, T_Data&>;

	struct Node
	{
		Node		*next;
		T_Data		data;

		void Clear() {data.~T_Data();}
	};

	Node	*head;

public:
	Stack() : head(nullptr) {}
	~Stack() {Clear();}

	bool Empty() const {return !head;}

	T_Data& Top()
	{
		return head->data;
	}

	T_Data *Push(Data_Arg item)
	{
		Node *newNode = ALLOC_NODE(Node);
		T_Data *data = &newNode->data;
		RawAssign<T_Data>(*data, item);
		newNode->next = head;
		head = newNode;
		return data;
	}

	template <typename ...Args>
	T_Data *Push(Args && ...args)
	{
		Node *newNode = ALLOC_NODE(Node);
		T_Data *data = &newNode->data;
		new (data) T_Data(std::forward<Args>(args)...);
		newNode->next = head;
		head = newNode;
		return data;
	}

	T_Data *Pop()
	{
		if (!head) return nullptr;
		T_Data *frontItem = &head->data;
		Node *toRemove = head;
		head = head->next;
		toRemove->Clear();
		Pool_Free(toRemove, sizeof(Node));
		return frontItem;
	}

	void Clear()
	{
		if (!head) return;
		Node *pNode;
		do
		{
			pNode = head;
			head = head->next;
			pNode->Clear();
			Pool_Free(pNode, sizeof(Node));
		}
		while (head);
	}
};

template <typename T_Data> class LinkedList
{
	using Data_Arg = std::conditional_t<std::is_scalar_v<T_Data>, T_Data, T_Data&>;

	struct Node
	{
		Node		*next;
		Node		*prev;
		T_Data		data;

		void Clear() {data.~T_Data();}
	};

	Node	*head;
	Node	*tail;

	Node *GetNthNode(UInt32 index) const
	{
		Node *pNode = head;
		while (pNode)
		{
			if (!index) break;
			index--;
			pNode = pNode->next;
		}
		return pNode;
	}

	Node *PrependNew()
	{
		Node *newNode = ALLOC_NODE(Node);
		newNode->next = head;
		newNode->prev = nullptr;
		if (head) head->prev = newNode;
		else tail = newNode;
		head = newNode;
		return newNode;
	}

	Node *AppendNew()
	{
		Node *newNode = ALLOC_NODE(Node);
		newNode->next = nullptr;
		newNode->prev = tail;
		if (tail) tail->next = newNode;
		else head = newNode;
		tail = newNode;
		return newNode;
	}

	Node *InsertNew(UInt32 index)
	{
		Node *pNode = GetNthNode(index);
		if (!pNode) return AppendNew();
		Node *prev = pNode->prev, *newNode = ALLOC_NODE(Node);
		newNode->next = pNode;
		newNode->prev = prev;
		pNode->prev = newNode;
		if (prev) prev->next = newNode;
		else head = newNode;
		return newNode;
	}

	Node *FindNode(Data_Arg item) const
	{
		if (head)
		{
			Node *pNode = head;
			do
			{
				if (pNode->data == item)
					return pNode;
			}
			while (pNode = pNode->next);
		}
		return nullptr;
	}

	template <class Matcher>
	Node *FindNode(Matcher &matcher) const
	{
		if (head)
		{
			Node *pNode = head;
			do
			{
				if (matcher(pNode->data))
					return pNode;
			}
			while (pNode = pNode->next);
		}
		return nullptr;
	}

	void RemoveNode(Node *toRemove)
	{
		Node *next = toRemove->next, *prev = toRemove->prev;
		if (prev) prev->next = next;
		else head = next;
		if (next) next->prev = prev;
		else tail = prev;
		toRemove->Clear();
		Pool_Free(toRemove, sizeof(Node));
	}

public:
	LinkedList() : head(nullptr), tail(nullptr) {}
	LinkedList(std::initializer_list<T_Data> inList) : head(nullptr), tail(nullptr) {AppendList(inList);}
	~LinkedList() {Clear();}

	bool Empty() const {return !head;}

	UInt32 Size() const
	{
		if (!head) return 0;
		UInt32 size = 1;
		Node *pNode = head;
		while (pNode = pNode->next)
			size++;
		return size;
	}

	T_Data *Prepend(Data_Arg item)
	{
		Node *newNode = PrependNew();
		T_Data *data = &newNode->data;
		RawAssign<T_Data>(*data, item);
		return data;
	}

	template <typename ...Args>
	T_Data *Prepend(Args && ...args)
	{
		Node *newNode = PrependNew();
		T_Data *data = &newNode->data;
		new (data) T_Data(std::forward<Args>(args)...);
		return data;
	}

	T_Data *Append(Data_Arg item)
	{
		Node *newNode = AppendNew();
		T_Data *data = &newNode->data;
		RawAssign<T_Data>(*data, item);
		return data;
	}

	template <typename ...Args>
	T_Data *Append(Args && ...args)
	{
		Node *newNode = AppendNew();
		T_Data *data = &newNode->data;
		new (data) T_Data(std::forward<Args>(args)...);
		return data;
	}

	void AppendList(std::initializer_list<T_Data> inList)
	{
		for (auto iter = inList.begin(); iter != inList.end(); ++iter)
			Append(*iter);
	}

	T_Data *Insert(UInt32 index, Data_Arg item)
	{
		Node *newNode = InsertNew(index);
		T_Data *data = &newNode->data;
		RawAssign<T_Data>(*data, item);
		return data;
	}

	template <typename ...Args>
	T_Data *Insert(UInt32 index, Args && ...args)
	{
		Node *newNode = InsertNew(index);
		T_Data *data = &newNode->data;
		new (data) T_Data(std::forward<Args>(args)...);
		return data;
	}

	T_Data *GetNth(UInt32 index) const
	{
		Node *pNode = GetNthNode(index);
		return pNode ? &pNode->data : nullptr;
	}

	T_Data *Front() const
	{
		return head ? &head->data : nullptr;
	}

	T_Data *Back() const
	{
		return tail ? &tail->data : nullptr;
	}

	SInt32 GetIndexOf(Data_Arg item) const
	{
		if (head)
		{
			Node *pNode = head;
			SInt32 index = 0;
			do
			{
				if (pNode->data == item)
					return index;
				index++;
			}
			while (pNode = pNode->next);
		}
		return -1;
	}

	template <class Matcher>
	SInt32 GetIndexOf(Matcher &matcher) const
	{
		if (head)
		{
			Node *pNode = head;
			SInt32 index = 0;
			do
			{
				if (matcher(pNode->data))
					return index;
				index++;
			}
			while (pNode = pNode->next);
		}
		return -1;
	}

	bool IsInList(Data_Arg item) const
	{
		if (head)
		{
			Node *pNode = head;
			do
			{
				if (pNode->data == item)
					return true;
			}
			while (pNode = pNode->next);
		}
		return false;
	}

	template <class Matcher>
	bool IsInList(Matcher &matcher) const
	{
		if (head)
		{
			Node *pNode = head;
			do
			{
				if (matcher(pNode->data))
					return true;
			}
			while (pNode = pNode->next);
		}
		return false;
	}

	T_Data *Remove(Data_Arg item)
	{
		Node *pNode = FindNode(item);
		if (pNode)
		{
			RemoveNode(pNode);
			return &pNode->data;
		}
		return nullptr;
	}

	template <class Matcher>
	T_Data *Remove(Matcher &matcher)
	{
		Node *pNode = FindNode(matcher);
		if (pNode)
		{
			RemoveNode(pNode);
			return &pNode->data;
		}
		return nullptr;
	}

	T_Data *RemoveNth(UInt32 index)
	{
		Node *pNode = GetNthNode(index);
		if (pNode)
		{
			RemoveNode(pNode);
			return &pNode->data;
		}
		return nullptr;
	}

	T_Data *PopFront()
	{
		if (!head) return nullptr;
		T_Data *frontItem = &head->data;
		RemoveNode(head);
		return frontItem;
	}

	T_Data *PopBack()
	{
		if (!tail) return nullptr;
		T_Data *backItem = &tail->data;
		RemoveNode(tail);
		return backItem;
	}

	void Clear()
	{
		if (!head) return;
		Node *pNode;
		do
		{
			pNode = head;
			head = head->next;
			pNode->Clear();
			Pool_Free(pNode, sizeof(Node));
		}
		while (head);
		tail = nullptr;
	}

	class Iterator
	{
		friend LinkedList;

		Node		*pNode;

	public:
		explicit operator bool() const {return pNode != nullptr;}
		void operator++() {pNode = pNode->next;}
		void operator--() {pNode = pNode->prev;}

		Data_Arg operator*() const {return pNode->data;}
		Data_Arg operator->() const {return pNode->data;}
		Data_Arg operator()() const {return pNode->data;}
		T_Data& Ref() {return pNode->data;}

		Iterator() : pNode(nullptr) {}
		Iterator(Node *_node) : pNode(_node) {}

		Iterator& operator=(const Iterator &other)
		{
			pNode = other.pNode;
			return *this;
		}

		void Remove(LinkedList &theList, bool frwrd = true)
		{
			Node *toRemove = pNode;
			pNode = frwrd ? pNode->next : pNode->prev;
			theList.RemoveNode(toRemove);
		}
	};

	T_Data *Remove(Iterator &iter)
	{
		Node *pNode = iter.pNode;
		if (pNode)
		{
			RemoveNode(pNode);
			return &pNode->data;
		}
		return nullptr;
	}

	Iterator Begin() {return Iterator(head);}

	Iterator BeginRv() {return Iterator(tail);}

	Iterator Find(Data_Arg item)
	{
		return Iterator(FindNode(item));
	}

	template <class Matcher>
	Iterator Find(Matcher &matcher)
	{
		return Iterator(FindNode(matcher));
	}
};

template <typename T_Data> __forceinline UInt32 AlignNumAlloc(UInt32 numAlloc)
{
	switch (sizeof(T_Data) & 0xF)
	{
		case 0:
			return numAlloc;
		case 2:
		case 6:
		case 0xA:
		case 0xE:
			if (numAlloc & 7)
			{
				numAlloc &= 0xFFFFFFF8;
				numAlloc += 8;
			}
			return numAlloc;
		case 4:
		case 0xC:
			if (numAlloc & 3)
			{
				numAlloc &= 0xFFFFFFFC;
				numAlloc += 4;
			}
			return numAlloc;
		case 8:
			if (numAlloc & 1)
				numAlloc++;
			return numAlloc;
		default:
			if (numAlloc & 0xF)
			{
				numAlloc &= 0xFFFFFFF0;
				numAlloc += 0x10;
			}
			return numAlloc;
	}
}

template <typename T_Key, typename T_Data> struct MappedPair
{
	T_Key		key;
	T_Data		value;
};

template <typename T_Key> class MapKey
{
	using Key_Arg = std::conditional_t<std::is_scalar_v<T_Key>, T_Key, const T_Key&>;

	T_Key		key;

public:
	__forceinline Key_Arg Get() const {return key;}
	__forceinline void Set(Key_Arg inKey)
	{
		if (std::is_same_v<T_Key, char*>)
			*(char**)&key = CopyString(*(const char**)&inKey);
		else key = inKey;
	}
	__forceinline char Compare(Key_Arg inKey) const
	{
		if (std::is_same_v<T_Key, char*> || std::is_same_v<T_Key, const char*>)
			return StrCompare(*(const char**)&inKey, *(const char**)&key);
		if (inKey < key) return -1;
		return (key < inKey) ? 1 : 0;
	}
	__forceinline void Clear()
	{
		if (std::is_same_v<T_Key, char*>)
			free(*(char**)&key);
		else key.~T_Key();
	}
};

template <typename T_Data> class MapValue
{
	T_Data		value;

public:
	__forceinline T_Data *Init() {return &value;}
	__forceinline T_Data& Get() {return value;}
	__forceinline T_Data *Ptr() {return &value;}
	__forceinline void Clear() {value.~T_Data();}
};

template <typename T_Data> class MapValue_p
{
	T_Data		*value;

public:
	__forceinline T_Data *Init()
	{
		value = ALLOC_NODE(T_Data);
		return value;
	}
	__forceinline T_Data& Get() {return *value;}
	__forceinline T_Data *Ptr() {return value;}
	__forceinline void Clear()
	{
		value->~T_Data();
		Pool_Free(value, sizeof(T_Data));
	}
};

template <typename T_Key, typename T_Data> class Map
{
	using M_Key = MapKey<T_Key>;
	using M_Value = std::conditional_t<(sizeof(T_Data) <= 8) || (sizeof(T_Data) <= alignof(T_Key)), MapValue<T_Data>, MapValue_p<T_Data>>;
	using Key_Arg = std::conditional_t<std::is_scalar_v<T_Key>, T_Key, const T_Key&>;
	using Data_Arg = std::conditional_t<std::is_scalar_v<T_Data>, T_Data, T_Data&>;

	struct Entry
	{
		M_Key		key;
		M_Value		value;

		void Clear()
		{
			key.Clear();
			value.Clear();
		}
	};

	Entry		*entries;		// 00
	UInt32		numEntries;		// 04
	UInt32		numAlloc;		// 08

	bool GetIndex(Key_Arg key, UInt32 *outIdx) const
	{
		UInt32 lBound = 0, uBound = numEntries, index;
		char cmpr;
		while (lBound != uBound)
		{
			index = (lBound + uBound) >> 1;
			cmpr = entries[index].key.Compare(key);
			if (!cmpr)
			{
				*outIdx = index;
				return true;
			}
			if (cmpr < 0) uBound = index;
			else lBound = index + 1;
		}
		*outIdx = lBound;
		return false;
	}

	bool InsertKey(Key_Arg key, T_Data **outData)
	{
		UInt32 index;
		if (GetIndex(key, &index))
		{
			*outData = entries[index].value.Ptr();
			return false;
		}
		if (!entries)
		{
			numAlloc = AlignNumAlloc<Entry>(numAlloc);
			entries = POOL_ALLOC(numAlloc, Entry);
		}
		else if (numAlloc <= numEntries)
		{
			UInt32 newAlloc = numAlloc << 1;
			POOL_REALLOC(entries, numAlloc, newAlloc, Entry);
			numAlloc = newAlloc;
		}
		Entry *pEntry = entries + index;
		index = numEntries - index;
		if (index) memmove(pEntry + 1, pEntry, sizeof(Entry) * index);
		numEntries++;
		pEntry->key.Set(key);
		*outData = pEntry->value.Init();
		return true;
	}

	Entry *End() const {return entries + numEntries;}

public:
	Map(UInt32 _alloc = MAP_DEFAULT_ALLOC) : entries(nullptr), numEntries(0), numAlloc(_alloc) {}
	Map(std::initializer_list<MappedPair<T_Key, T_Data>> inList) : entries(nullptr), numEntries(0), numAlloc(inList.size()) {InsertList(inList);}
	~Map()
	{
		if (!entries) return;
		Clear();
		POOL_FREE(entries, numAlloc, Entry);
		entries = nullptr;
	}

	UInt32 Size() const {return numEntries;}
	bool Empty() const {return !numEntries;}

	bool Insert(Key_Arg key, T_Data **outData)
	{
		if (!InsertKey(key, outData)) return false;
		new (*outData) T_Data();
		return true;
	}

	T_Data& operator[](Key_Arg key)
	{
		T_Data *outData;
		if (InsertKey(key, &outData))
			new (outData) T_Data();
		return *outData;
	}

	template <typename ...Args>
	T_Data* Emplace(Key_Arg key, Args&& ...args)
	{
		T_Data *outData;
		if (InsertKey(key, &outData))
			new (outData) T_Data(std::forward<Args>(args)...);
		return outData;
	}

	void InsertList(std::initializer_list<MappedPair<T_Key, T_Data>> inList)
	{
		T_Data *outData;
		for (auto iter = inList.begin(); iter != inList.end(); ++iter)
		{
			InsertKey(iter->key, &outData);
			*outData = iter->value;
		}
	}

	bool HasKey(Key_Arg key) const
	{
		UInt32 index;
		return GetIndex(key, &index);
	}

	T_Data Get(Key_Arg key)
	{
		UInt32 index;
		return GetIndex(key, &index) ? entries[index].value.Get() : NULL;
	}

	T_Data* GetPtr(Key_Arg key)
	{
		UInt32 index;
		return GetIndex(key, &index) ? entries[index].value.Ptr() : nullptr;
	}

	bool Erase(Key_Arg key)
	{
		UInt32 index;
		if (!GetIndex(key, &index)) return false;
		Entry *pEntry = entries + index;
		pEntry->Clear();
		numEntries--;
		index = numEntries - index;
		if (index) memmove(pEntry, pEntry + 1, sizeof(Entry) * index);
		return true;
	}

	void Clear()
	{
		if (numEntries)
		{
			Entry *pEntry = entries, *pEnd = End();
			do
			{
				pEntry->Clear();
				pEntry++;
			}
			while (pEntry != pEnd);
		}
		numEntries = 0;
	}

	class Iterator
	{
		friend Map;

		Entry		*pEntry;
		UInt32		count;

		Iterator() {}

	public:
		Key_Arg Key() const {return pEntry->key.Get();}
		Data_Arg operator()() const {return pEntry->value.Get();}
		Data_Arg operator*() const {return pEntry->value.Get();}
		Data_Arg operator->() const {return pEntry->value.Get();}
		T_Data& Ref() {return pEntry->value.Get();}

		explicit operator bool() const {return count != 0;}
		void operator++()
		{
			pEntry++;
			count--;
		}

		void Find(Map &source, Key_Arg key)
		{
			UInt32 index;
			if (source.GetIndex(key, &index))
			{
				pEntry = source.entries + index;
				count = source.numEntries - index;
			}
			else count = 0;
		}

		Iterator(Map &source) : pEntry(source.entries), count(source.numEntries) {}
		Iterator(Map &source, Key_Arg key) {Find(source, key);}
	};

	class OpIterator : public Iterator
	{
	public:
		void operator--()
		{
			pEntry--;
			count--;
		}

		void Remove(Map &source, bool frwrd = true)
		{
			pEntry->Clear();
			UInt32 size = frwrd ? (count - 1) : (source.numEntries - count);
			source.numEntries--;
			if (size) memmove(pEntry, pEntry + 1, size * sizeof(Entry));
			if (frwrd) pEntry--;
		}

		OpIterator(Map &source)
		{
			pEntry = source.entries;
			count = source.numEntries;
		}
		OpIterator(Map &source, Key_Arg key) {Find(source, key);}
		OpIterator(Map &source, Key_Arg key, bool frwrd)
		{
			if (source.numEntries)
			{
				UInt32 index;
				bool match = source.GetIndex(key, &index);
				if (frwrd)
				{
					pEntry = source.entries + index;
					count = source.numEntries - index;
				}
				else
				{
					pEntry = source.entries + (index - !match);
					count = index + match;
				}
			}
			else count = 0;
		}
	};

	class CpIterator : public Iterator
	{
	public:
		CpIterator(Map &source)
		{
			if (count = source.numEntries)
			{
				UInt32 size = count * sizeof(Entry);
				pEntry = (Entry*)memcpy(GetAuxBuffer(s_auxBuffers[0], size), source.entries, size);
			}
		}
	};

	Iterator Begin() {return Iterator(*this);}
	Iterator Find(Key_Arg key) {return Iterator(*this, key);}

	OpIterator BeginOp() {return OpIterator(*this);}
	OpIterator FindOp(Key_Arg key) {return OpIterator(*this, key);}
	OpIterator FindOpDir(Key_Arg key, bool frwrd) {return OpIterator(*this, key, frwrd);}

	CpIterator BeginCp() {return CpIterator(*this);}
};

template <typename T_Key> class Set
{
	using M_Key = MapKey<T_Key>;
	using Key_Arg = std::conditional_t<std::is_scalar_v<T_Key>, T_Key, const T_Key&>;

	M_Key		*keys;		// 00
	UInt32		numKeys;	// 04
	UInt32		numAlloc;	// 08

	bool GetIndex(Key_Arg key, UInt32 *outIdx) const
	{
		UInt32 lBound = 0, uBound = numKeys, index;
		char cmpr;
		while (lBound != uBound)
		{
			index = (lBound + uBound) >> 1;
			cmpr = keys[index].Compare(key);
			if (!cmpr)
			{
				*outIdx = index;
				return true;
			}
			if (cmpr < 0) uBound = index;
			else lBound = index + 1;
		}
		*outIdx = lBound;
		return false;
	}

	M_Key *End() const {return keys + numKeys;}

public:
	Set(UInt32 _alloc = MAP_DEFAULT_ALLOC) : keys(nullptr), numKeys(0), numAlloc(_alloc) {}
	Set(std::initializer_list<T_Key> inList) : keys(nullptr), numKeys(0), numAlloc(inList.size()) {InsertList(inList);}
	~Set()
	{
		if (!keys) return;
		Clear();
		POOL_FREE(keys, numAlloc, M_Key);
		keys = nullptr;
	}

	UInt32 Size() const {return numKeys;}
	bool Empty() const {return !numKeys;}
	T_Key *Keys() {return reinterpret_cast<T_Key*>(keys);}

	bool Insert(Key_Arg key)
	{
		UInt32 index;
		if (GetIndex(key, &index)) return false;
		if (!keys)
		{
			numAlloc = AlignNumAlloc<M_Key>(numAlloc);
			keys = POOL_ALLOC(numAlloc, M_Key);
		}
		else if (numAlloc <= numKeys)
		{
			UInt32 newAlloc = numAlloc << 1;
			POOL_REALLOC(keys, numAlloc, newAlloc, M_Key);
			numAlloc = newAlloc;
		}
		M_Key *pKey = keys + index;
		index = numKeys - index;
		if (index) memmove(pKey + 1, pKey, sizeof(M_Key) * index);
		numKeys++;
		pKey->Set(key);
		return true;
	}

	void InsertList(std::initializer_list<T_Key> inList)
	{
		for (auto iter = inList.begin(); iter != inList.end(); ++iter)
			Insert(*iter);
	}

	bool HasKey(Key_Arg key) const
	{
		UInt32 index;
		return GetIndex(key, &index);
	}

	bool Erase(Key_Arg key)
	{
		UInt32 index;
		if (!GetIndex(key, &index)) return false;
		M_Key *pKey = keys + index;
		pKey->Clear();
		numKeys--;
		index = numKeys - index;
		if (index) memmove(pKey, pKey + 1, sizeof(M_Key) * index);
		return true;
	}

	void Clear()
	{
		if (numKeys)
		{
			M_Key *pKey = keys, *pEnd = End();
			do
			{
				pKey->Clear();
				pKey++;
			}
			while (pKey != pEnd);
		}
		numKeys = 0;
	}

	class Iterator
	{
		friend Set;

		M_Key		*pKey;
		UInt32		count;

		Iterator() {}

	public:
		Key_Arg operator()() const {return pKey->Get();}
		Key_Arg operator*() const {return pKey->Get();}
		Key_Arg operator->() const {return pKey->Get();}

		explicit operator bool() const {return count != 0;}
		void operator++()
		{
			pKey++;
			count--;
		}

		Iterator(Set &source) : pKey(source.keys), count(source.numKeys) {}
		Iterator(Set &source, Key_Arg key)
		{
			UInt32 index;
			if (source.GetIndex(key, &index))
			{
				pKey = source.keys + index;
				count = source.numKeys - index;
			}
			else count = 0;
		}
	};

	class CpIterator : public Iterator
	{
	public:
		CpIterator(Set &source)
		{
			if (count = source.numKeys)
			{
				UInt32 size = count * sizeof(M_Key);
				pKey = (M_Key*)memcpy(GetAuxBuffer(s_auxBuffers[1], size), source.keys, size);
			}
		}
	};

	void Remove(Iterator &iter)
	{
		iter.pKey->Clear();
		numKeys--;
		if (iter.count > 1)
		{
			memmove(iter.pKey, iter.pKey + 1, (iter.count - 1) * sizeof(M_Key));
			iter.pKey--;
		}
	}

	Iterator Begin() {return Iterator(*this);}
	Iterator Find(Key_Arg key) {return Iterator(*this, key);}

	CpIterator BeginCp() {return CpIterator(*this);}
};

template <typename T_Key> __forceinline UInt32 HashKey(T_Key inKey)
{
	if (std::is_same_v<T_Key, char*> || std::is_same_v<T_Key, const char*>)
		return StrHashCI(*(const char**)&inKey);
	UInt32 uKey;
	if (sizeof(T_Key) == 1)
		uKey = *(UInt8*)&inKey;
	else if (sizeof(T_Key) == 2)
		uKey = *(UInt16*)&inKey;
	else
	{
		uKey = *(UInt32*)&inKey;
		if (sizeof(T_Key) > 4)
			uKey += uKey ^ ((UInt32*)&inKey)[1];
	}
	return (uKey * 0xD) ^ (uKey >> 0xF);
}

template <typename T_Key> class HashedKey
{
	using Key_Arg = std::conditional_t<std::is_scalar_v<T_Key>, T_Key, const T_Key&>;

	T_Key		key;

public:
	__forceinline bool Match(Key_Arg inKey, UInt32) const {return key == inKey;}
	__forceinline Key_Arg Get() const {return key;}
	__forceinline void Set(Key_Arg inKey, UInt32) {key = inKey;}
	__forceinline UInt32 GetHash() const {return HashKey<T_Key>(key);}
	__forceinline void Clear() {key.~T_Key();}
};

template <> class HashedKey<const char*>
{
	UInt32		hashVal;

public:
	__forceinline bool Match(const char*, UInt32 inHash) const {return hashVal == inHash;}
	__forceinline const char *Get() const {return "";}
	__forceinline void Set(const char*, UInt32 inHash) {hashVal = inHash;}
	__forceinline UInt32 GetHash() const {return hashVal;}
	__forceinline void Clear() {}
};

template <> class HashedKey<char*>
{
	UInt32		hashVal;
	char		*key;

public:
	__forceinline bool Match(char*, UInt32 inHash) const {return hashVal == inHash;}
	__forceinline char *Get() const {return key;}
	__forceinline void Set(char *inKey, UInt32 inHash)
	{
		hashVal = inHash;
		key = CopyString(inKey);
	}
	__forceinline UInt32 GetHash() const {return hashVal;}
	__forceinline void Clear() {free(key);}
};

template <typename T_Key, typename T_Data> class UnorderedMap
{
	using H_Key = HashedKey<T_Key>;
	using Key_Arg = std::conditional_t<std::is_scalar_v<T_Key>, T_Key, const T_Key&>;
	using Data_Arg = std::conditional_t<std::is_scalar_v<T_Data>, T_Data, T_Data&>;

	struct Entry
	{
		Entry		*next;
		H_Key		key;
		T_Data		value;

		void Clear()
		{
			key.Clear();
			value.~T_Data();
		}
	};

	struct Bucket
	{
		Entry	*entries;

		void Insert(Entry *entry)
		{
			entry->next = entries;
			entries = entry;
		}

		void Remove(Entry *entry, Entry *prev)
		{
			if (prev) prev->next = entry->next;
			else entries = entry->next;
			entry->Clear();
			Pool_Free(entry, sizeof(Entry));
		}

		void Clear()
		{
			if (!entries) return;
			Entry *pEntry;
			do
			{
				pEntry = entries;
				entries = entries->next;
				pEntry->Clear();
				Pool_Free(pEntry, sizeof(Entry));
			}
			while (entries);
		}

		UInt32 Size() const
		{
			if (!entries) return 0;
			UInt32 size = 1;
			Entry *pEntry = entries;
			while (pEntry = pEntry->next)
				size++;
			return size;
		}
	};

	Bucket		*buckets;		// 00
	UInt32		numBuckets;		// 04
	UInt32		numEntries;		// 08

	Bucket *End() const {return buckets + numBuckets;}

	__declspec(noinline) void ResizeTable(UInt32 newCount)
	{
		Bucket *pBucket = buckets, *pEnd = End(), *newBuckets = (Bucket*)Pool_Alloc_Buckets(newCount);
		Entry *pEntry, *pTemp;
		newCount--;
		do
		{
			pEntry = pBucket->entries;
			while (pEntry)
			{
				pTemp = pEntry;
				pEntry = pEntry->next;
				newBuckets[pTemp->key.GetHash() & newCount].Insert(pTemp);
			}
			pBucket++;
		}
		while (pBucket != pEnd);
		Pool_Free(buckets, numBuckets * sizeof(Bucket));
		buckets = newBuckets;
		numBuckets = newCount + 1;
	}

	Entry *FindEntry(Key_Arg key) const
	{
		if (numEntries)
		{
			UInt32 hashVal = HashKey<T_Key>(key);
			for (Entry *pEntry = buckets[hashVal & (numBuckets - 1)].entries; pEntry; pEntry = pEntry->next)
				if (pEntry->key.Match(key, hashVal)) return pEntry;
		}
		return nullptr;
	}

	bool InsertKey(Key_Arg key, T_Data **outData)
	{
		if (!buckets)
		{
			numBuckets = AlignBucketCount(numBuckets);
			buckets = (Bucket*)Pool_Alloc_Buckets(numBuckets);
		}
		else if ((numEntries > numBuckets) && (numBuckets < MAP_MAX_BUCKET_COUNT))
			ResizeTable(numBuckets << 1);
		UInt32 hashVal = HashKey<T_Key>(key);
		Bucket *pBucket = &buckets[hashVal & (numBuckets - 1)];
		for (Entry *pEntry = pBucket->entries; pEntry; pEntry = pEntry->next)
		{
			if (!pEntry->key.Match(key, hashVal)) continue;
			*outData = &pEntry->value;
			return false;
		}
		numEntries++;
		Entry *newEntry = ALLOC_NODE(Entry);
		newEntry->key.Set(key, hashVal);
		pBucket->Insert(newEntry);
		*outData = &newEntry->value;
		return true;
	}

public:
	UnorderedMap(UInt32 _numBuckets = MAP_DEFAULT_BUCKET_COUNT) : buckets(nullptr), numBuckets(_numBuckets), numEntries(0) {}
	UnorderedMap(std::initializer_list<MappedPair<T_Key, T_Data>> inList) : buckets(nullptr), numBuckets(inList.size()), numEntries(0) {InsertList(inList);}
	~UnorderedMap()
	{
		if (!buckets) return;
		Clear();
		Pool_Free(buckets, numBuckets * sizeof(Bucket));
		buckets = nullptr;
	}

	UInt32 Size() const {return numEntries;}
	bool Empty() const {return !numEntries;}

	UInt32 BucketCount() const {return numBuckets;}

	void SetBucketCount(UInt32 newCount)
	{
		if (buckets)
		{
			newCount = AlignBucketCount(newCount);
			if ((numBuckets != newCount) && (numEntries <= newCount))
				ResizeTable(newCount);
		}
		else numBuckets = newCount;
	}

	float LoadFactor() const {return (float)numEntries / (float)numBuckets;}

	bool Insert(Key_Arg key, T_Data **outData)
	{
		if (!InsertKey(key, outData)) return false;
		new (*outData) T_Data();
		return true;
	}

	T_Data& operator[](Key_Arg key)
	{
		T_Data *outData;
		if (InsertKey(key, &outData))
			new (outData) T_Data();
		return *outData;
	}

	template <typename ...Args>
	T_Data* Emplace(Key_Arg key, Args&& ...args)
	{
		T_Data *outData;
		if (InsertKey(key, &outData))
			new (outData) T_Data(std::forward<Args>(args)...);
		return outData;
	}

	Data_Arg InsertNotIn(Key_Arg key, Data_Arg value)
	{
		T_Data *outData;
		if (InsertKey(key, &outData))
			RawAssign<T_Data>(*outData, value);
		return value;
	}

	void InsertList(std::initializer_list<MappedPair<T_Key, T_Data>> inList)
	{
		T_Data *outData;
		for (auto iter = inList.begin(); iter != inList.end(); ++iter)
		{
			InsertKey(iter->key, &outData);
			*outData = iter->value;
		}
	}

	bool HasKey(Key_Arg key) const {return FindEntry(key) ? true : false;}

	T_Data Get(Key_Arg key)
	{
		Entry *pEntry = FindEntry(key);
		return pEntry ? pEntry->value : NULL;
	}

	T_Data* GetPtr(Key_Arg key)
	{
		Entry *pEntry = FindEntry(key);
		return pEntry ? &pEntry->value : nullptr;
	}

	bool Erase(Key_Arg key)
	{
		if (numEntries)
		{
			UInt32 hashVal = HashKey<T_Key>(key);
			Bucket *pBucket = &buckets[hashVal & (numBuckets - 1)];
			Entry *pEntry = pBucket->entries, *prev = nullptr;
			while (pEntry)
			{
				if (pEntry->key.Match(key, hashVal))
				{
					numEntries--;
					pBucket->Remove(pEntry, prev);
					return true;
				}
				prev = pEntry;
				pEntry = pEntry->next;
			}
		}
		return false;
	}

	T_Data GetErase(Key_Arg key)
	{
		if (numEntries)
		{
			UInt32 hashVal = HashKey<T_Key>(key);
			Bucket *pBucket = &buckets[hashVal & (numBuckets - 1)];
			Entry *pEntry = pBucket->entries, *prev = nullptr;
			while (pEntry)
			{
				if (pEntry->key.Match(key, hashVal))
				{
					T_Data outVal = pEntry->value;
					numEntries--;
					pBucket->Remove(pEntry, prev);
					return outVal;
				}
				prev = pEntry;
				pEntry = pEntry->next;
			}
		}
		return NULL;
	}

	bool Clear()
	{
		if (!numEntries) return false;
		Bucket *pBucket = buckets, *pEnd = End();
		do
		{
			pBucket->Clear();
			pBucket++;
		}
		while (pBucket != pEnd);
		numEntries = 0;
		return true;
	}

	void DumpLoads()
	{
		UInt32 loadsArray[0x40];
		MemZero(loadsArray, sizeof(loadsArray));
		Bucket *pBucket = buckets;
		UInt32 maxLoad = 0, entryCount;
		for (Bucket *pEnd = End(); pBucket != pEnd; pBucket++)
		{
			entryCount = pBucket->Size();
			loadsArray[entryCount]++;
			if (maxLoad < entryCount)
				maxLoad = entryCount;
		}
		PrintDebug("Size = %d\nBuckets = %d\n----------------\n", numEntries, numBuckets);
		for (UInt32 iter = 0; iter <= maxLoad; iter++)
			PrintDebug("%d:\t%05d (%.4f%%)", iter, loadsArray[iter], 100.0 * (double)loadsArray[iter] / numEntries);
	}

	class Iterator
	{
		friend UnorderedMap;

		UnorderedMap	*table;
		Bucket			*bucket;
		Entry			*entry;

		void FindNonEmpty()
		{
			for (Bucket *pEnd = table->End(); bucket != pEnd; bucket++)
				if (entry = bucket->entries) return;
		}

	public:
		void Init(UnorderedMap &_table)
		{
			table = &_table;
			entry = nullptr;
			if (table->numEntries)
			{
				bucket = table->buckets;
				FindNonEmpty();
			}
		}

		void Find(Key_Arg key)
		{
			if (!table->numEntries)
			{
				entry = nullptr;
				return;
			}
			UInt32 hashVal = HashKey<T_Key>(key);
			bucket = &table->buckets[hashVal & (table->numBuckets - 1)];
			entry = bucket->entries;
			while (entry)
			{
				if (entry->key.Match(key, hashVal))
					break;
				entry = entry->next;
			}
		}

		UnorderedMap* Table() const {return table;}
		Key_Arg Key() const {return entry->key.Get();}
		Data_Arg operator()() const {return entry->value;}
		T_Data& Ref() {return entry->value;}
		Data_Arg operator*() const {return entry->value;}
		Data_Arg operator->() const {return entry->value;}

		explicit operator bool() const {return entry != nullptr;}
		void operator++()
		{
			if (entry)
				entry = entry->next;
			else entry = bucket->entries;
			if (!entry && table->numEntries)
			{
				bucket++;
				FindNonEmpty();
			}
		}

		bool IsValid()
		{
			if (entry)
			{
				for (Entry *temp = bucket->entries; temp; temp = temp->next)
					if (temp == entry) return true;
				entry = nullptr;
			}
			return false;
		}

		void Remove()
		{
			Entry *curr = bucket->entries, *prev = nullptr;
			do
			{
				if (curr == entry) break;
				prev = curr;
			}
			while (curr = curr->next);
			table->numEntries--;
			bucket->Remove(entry, prev);
			entry = prev;
		}

		Iterator(UnorderedMap *_table = nullptr) : table(_table), entry(nullptr) {}
		Iterator(UnorderedMap &_table) {Init(_table);}
		Iterator(UnorderedMap &_table, Key_Arg key) : table(&_table) {Find(key);}
	};

	Iterator Begin() {return Iterator(*this);}
	Iterator Find(Key_Arg key) {return Iterator(*this, key);}
};

template <typename T_Key> class UnorderedSet
{
	using H_Key = HashedKey<T_Key>;
	using Key_Arg = std::conditional_t<std::is_scalar_v<T_Key>, T_Key, const T_Key&>;

	struct Entry
	{
		Entry		*next;
		H_Key		key;

		void Clear() {key.Clear();}
	};

	struct Bucket
	{
		Entry	*entries;

		void Insert(Entry *entry)
		{
			entry->next = entries;
			entries = entry;
		}

		void Remove(Entry *entry, Entry *prev)
		{
			if (prev) prev->next = entry->next;
			else entries = entry->next;
			entry->Clear();
			Pool_Free(entry, sizeof(Entry));
		}

		void Clear()
		{
			if (!entries) return;
			Entry *pEntry;
			do
			{
				pEntry = entries;
				entries = entries->next;
				pEntry->Clear();
				Pool_Free(pEntry, sizeof(Entry));
			}
			while (entries);
		}

		UInt32 Size() const
		{
			if (!entries) return 0;
			UInt32 size = 1;
			Entry *pEntry = entries;
			while (pEntry = pEntry->next)
				size++;
			return size;
		}
	};

	Bucket		*buckets;		// 00
	UInt32		numBuckets;		// 04
	UInt32		numEntries;		// 08

	Bucket *End() const {return buckets + numBuckets;}

	__declspec(noinline) void ResizeTable(UInt32 newCount)
	{
		Bucket *pBucket = buckets, *pEnd = End(), *newBuckets = (Bucket*)Pool_Alloc_Buckets(newCount);
		Entry *pEntry, *pTemp;
		newCount--;
		do
		{
			pEntry = pBucket->entries;
			while (pEntry)
			{
				pTemp = pEntry;
				pEntry = pEntry->next;
				newBuckets[pTemp->key.GetHash() & newCount].Insert(pTemp);
			}
			pBucket++;
		}
		while (pBucket != pEnd);
		Pool_Free(buckets, numBuckets * sizeof(Bucket));
		buckets = newBuckets;
		numBuckets = newCount + 1;
	}

public:
	UnorderedSet(UInt32 _numBuckets = MAP_DEFAULT_BUCKET_COUNT) : buckets(nullptr), numBuckets(_numBuckets), numEntries(0) {}
	UnorderedSet(std::initializer_list<T_Key> inList) : buckets(nullptr), numBuckets(inList.size()), numEntries(0) {InsertList(inList);}
	~UnorderedSet()
	{
		if (!buckets) return;
		Clear();
		Pool_Free(buckets, numBuckets * sizeof(Bucket));
		buckets = nullptr;
	}

	UInt32 Size() const {return numEntries;}
	bool Empty() const {return !numEntries;}

	UInt32 BucketCount() const {return numBuckets;}

	void SetBucketCount(UInt32 newCount)
	{
		if (buckets)
		{
			newCount = AlignBucketCount(newCount);
			if ((numBuckets != newCount) && (numEntries <= newCount))
				ResizeTable(newCount);
		}
		else numBuckets = newCount;
	}

	float LoadFactor() const {return (float)numEntries / (float)numBuckets;}

	bool Insert(Key_Arg key)
	{
		if (!buckets)
		{
			numBuckets = AlignBucketCount(numBuckets);
			buckets = (Bucket*)Pool_Alloc_Buckets(numBuckets);
		}
		else if ((numEntries > numBuckets) && (numBuckets < MAP_MAX_BUCKET_COUNT))
			ResizeTable(numBuckets << 1);
		UInt32 hashVal = HashKey<T_Key>(key);
		Bucket *pBucket = &buckets[hashVal & (numBuckets - 1)];
		for (Entry *pEntry = pBucket->entries; pEntry; pEntry = pEntry->next)
			if (pEntry->key.Match(key, hashVal)) return false;
		numEntries++;
		Entry *newEntry = ALLOC_NODE(Entry);
		newEntry->key.Set(key, hashVal);
		pBucket->Insert(newEntry);
		return true;
	}

	void InsertList(std::initializer_list<T_Key> inList)
	{
		for (auto iter = inList.begin(); iter != inList.end(); ++iter)
			Insert(*iter);
	}

	bool HasKey(Key_Arg key) const
	{
		if (numEntries)
		{
			UInt32 hashVal = HashKey<T_Key>(key);
			for (Entry *pEntry = buckets[hashVal & (numBuckets - 1)].entries; pEntry; pEntry = pEntry->next)
				if (pEntry->key.Match(key, hashVal)) return true;
		}
		return false;
	}

	bool Erase(Key_Arg key)
	{
		if (numEntries)
		{
			UInt32 hashVal = HashKey<T_Key>(key);
			Bucket *pBucket = &buckets[hashVal & (numBuckets - 1)];
			Entry *pEntry = pBucket->entries, *prev = nullptr;
			while (pEntry)
			{
				if (pEntry->key.Match(key, hashVal))
				{
					numEntries--;
					pBucket->Remove(pEntry, prev);
					return true;
				}
				prev = pEntry;
				pEntry = pEntry->next;
			}
		}
		return false;
	}

	bool Clear()
	{
		if (!numEntries) return false;
		Bucket *pBucket = buckets, *pEnd = End();
		do
		{
			pBucket->Clear();
			pBucket++;
		}
		while (pBucket != pEnd);
		numEntries = 0;
		return true;
	}

	void DumpLoads()
	{
		UInt32 loadsArray[0x40];
		MemZero(loadsArray, sizeof(loadsArray));
		Bucket *pBucket = buckets;
		UInt32 maxLoad = 0, entryCount;
		for (Bucket *pEnd = End(); pBucket != pEnd; pBucket++)
		{
			entryCount = pBucket->Size();
			loadsArray[entryCount]++;
			if (maxLoad < entryCount)
				maxLoad = entryCount;
		}
		PrintDebug("Size = %d\nBuckets = %d\n----------------\n", numEntries, numBuckets);
		for (UInt32 iter = 0; iter <= maxLoad; iter++)
			PrintDebug("%d:\t%05d (%.4f%%)", iter, loadsArray[iter], 100.0 * (double)loadsArray[iter] / numEntries);
	}

	class Iterator
	{
		friend UnorderedSet;

		UnorderedSet	*table;
		Bucket			*bucket;
		Entry			*entry;

		void FindNonEmpty()
		{
			for (Bucket *pEnd = table->End(); bucket != pEnd; bucket++)
				if (entry = bucket->entries) return;
		}

	public:
		void Init(UnorderedSet &_table)
		{
			table = &_table;
			entry = nullptr;
			if (table->numEntries)
			{
				bucket = table->buckets;
				FindNonEmpty();
			}
		}

		Key_Arg operator*() const {return entry->key.Get();}
		Key_Arg operator->() const {return entry->key.Get();}

		explicit operator bool() const {return entry != nullptr;}
		void operator++()
		{
			if ((entry = entry->next) || !table->numEntries)
				return;
			bucket++;
			FindNonEmpty();
		}

		Iterator() : table(nullptr), entry(nullptr) {}
		Iterator(UnorderedSet &_table) {Init(_table);}
	};

	Iterator Begin() {return Iterator(*this);}
};

template <typename T_Data> class Vector
{
	using Data_Arg = std::conditional_t<std::is_scalar_v<T_Data>, T_Data, T_Data&>;

	T_Data		*data;		// 00
	UInt32		numItems;	// 04
	UInt32		numAlloc;	// 08

	__declspec(noinline) T_Data *AllocateData()
	{
		if (!data)
		{
			numAlloc = AlignNumAlloc<T_Data>(numAlloc);
			data = POOL_ALLOC(numAlloc, T_Data);
		}
		else if (numAlloc <= numItems)
		{
			UInt32 newAlloc = numAlloc << 1;
			POOL_REALLOC(data, numAlloc, newAlloc, T_Data);
			numAlloc = newAlloc;
		}
		return data + numItems++;
	}

	T_Data *End() const {return data + numItems;}

public:
	Vector(UInt32 _alloc = VECTOR_DEFAULT_ALLOC) : data(nullptr), numItems(0), numAlloc(_alloc) {}
	Vector(std::initializer_list<T_Data> inList) : data(nullptr), numItems(0), numAlloc(inList.size()) {AppendList(inList);}
	~Vector()
	{
		if (!data) return;
		Clear();
		POOL_FREE(data, numAlloc, T_Data);
		data = nullptr;
	}

	UInt32 Size() const {return numItems;}
	bool Empty() const {return !numItems;}

	T_Data* Data() const {return data;}

	T_Data& operator[](UInt32 index) const {return data[index];}

	T_Data *GetPtr(UInt32 index) const {return (index < numItems) ? (data + index) : nullptr;}

	Data_Arg Top() const {return data[numItems - 1];}

	T_Data* Append(Data_Arg item)
	{
		T_Data *pData = AllocateData();
		RawAssign<T_Data>(*pData, item);
		return pData;
	}

	template <typename ...Args>
	T_Data* Append(Args&& ...args)
	{
		T_Data *pData = AllocateData();
		new (pData) T_Data(std::forward<Args>(args)...);
		return pData;
	}

	void AppendList(std::initializer_list<T_Data> inList)
	{
		for (auto iter = inList.begin(); iter != inList.end(); ++iter)
			Append(*iter);
	}

	T_Data* Insert(UInt32 index, Data_Arg item)
	{
		if (index > numItems)
			index = numItems;
		UInt32 size = numItems - index;
		T_Data *pData = AllocateData();
		if (size)
		{
			pData = data + index;
			memmove(pData + 1, pData, sizeof(T_Data) * size);
		}
		RawAssign<T_Data>(*pData, item);
		return pData;
	}

	template <typename ...Args>
	T_Data* Insert(UInt32 index, Args&& ...args)
	{
		if (index > numItems)
			index = numItems;
		UInt32 size = numItems - index;
		T_Data *pData = AllocateData();
		if (size)
		{
			pData = data + index;
			memmove(pData + 1, pData, sizeof(T_Data) * size);
		}
		new (pData) T_Data(std::forward<Args>(args)...);
		return pData;
	}

	void Concatenate(const Vector &source)
	{
		if (!source.numItems) return;
		UInt32 newCount = numItems + source.numItems;
		if (!data)
		{
			if (numAlloc < newCount) numAlloc = newCount;
			numAlloc = AlignNumAlloc<T_Data>(numAlloc);
			data = POOL_ALLOC(numAlloc, T_Data);
		}
		else if (numAlloc < newCount)
		{
			newCount = AlignNumAlloc<T_Data>(newCount);
			POOL_REALLOC(data, numAlloc, newCount, T_Data);
			numAlloc = newCount;
		}
		memmove(data + numItems, source.data, sizeof(T_Data) * source.numItems);
		numItems = newCount;
	}

	UInt32 InsertSorted(Data_Arg item, bool descending = false)
	{
		UInt32 lBound = 0, uBound = numItems, index;
		bool isLT;
		while (lBound != uBound)
		{
			index = (lBound + uBound) >> 1;
			isLT = item < data[index];
			if (descending) isLT = !isLT;
			if (isLT) uBound = index;
			else lBound = index + 1;
		}
		uBound = numItems - lBound;
		T_Data *pData = AllocateData();
		if (uBound)
		{
			pData = data + lBound;
			memmove(pData + 1, pData, sizeof(T_Data) * uBound);
		}
		RawAssign<T_Data>(*pData, item);
		return lBound;
	}

	typedef bool (*SortComperator)(const T_Data &lhs, const T_Data &rhs);
	UInt32 InsertSorted(Data_Arg item, SortComperator comperator)
	{
		UInt32 lBound = 0, uBound = numItems, index;
		while (lBound != uBound)
		{
			index = (lBound + uBound) >> 1;
			if (comperator(item, data[index]))
				uBound = index;
			else lBound = index + 1;
		}
		uBound = numItems - lBound;
		T_Data *pData = AllocateData();
		if (uBound)
		{
			pData = data + lBound;
			memmove(pData + 1, pData, sizeof(T_Data) * uBound);
		}
		RawAssign<T_Data>(*pData, item);
		return lBound;
	}

	SInt32 GetIndexOf(Data_Arg item) const
	{
		if (numItems)
		{
			T_Data *pData = data, *pEnd = End();
			do
			{
				if (*pData == item)
					return pData - data;
				pData++;
			}
			while (pData != pEnd);
		}
		return -1;
	}

	template <class Matcher>
	SInt32 GetIndexOf(Matcher &matcher) const
	{
		if (numItems)
		{
			T_Data *pData = data, *pEnd = End();
			do
			{
				if (matcher(*pData))
					return pData - data;
				pData++;
			}
			while (pData != pEnd);
		}
		return -1;
	}

	template <class Matcher>
	T_Data* Find(Matcher &matcher) const
	{
		if (numItems)
		{
			T_Data *pData = data, *pEnd = End();
			do
			{
				if (matcher(*pData))
					return pData;
				pData++;
			}
			while (pData != pEnd);
		}
		return nullptr;
	}

	bool RemoveNth(UInt32 index)
	{
		if (index >= numItems) return false;
		T_Data *pData = data + index;
		pData->~T_Data();
		numItems--;
		index = numItems - index;
		if (index) memmove(pData, pData + 1, sizeof(T_Data) * index);
		return true;
	}

	bool Remove(Data_Arg item)
	{
		if (numItems)
		{
			T_Data *pData = End(), *pEnd = data;
			do
			{
				pData--;
				if (*pData != item) continue;
				numItems--;
				pData->~T_Data();
				UInt32 size = (UInt32)End() - (UInt32)pData;
				if (size) memmove(pData, pData + 1, size);
				return true;
			}
			while (pData != pEnd);
		}
		return false;
	}

	template <class Matcher>
	UInt32 Remove(Matcher &matcher)
	{
		if (numItems)
		{
			T_Data *pData = End(), *pEnd = data;
			UInt32 removed = 0, size;
			do
			{
				pData--;
				if (!matcher(*pData)) continue;
				numItems--;
				pData->~T_Data();
				size = (UInt32)End() - (UInt32)pData;
				if (size) memmove(pData, pData + 1, size);
				removed++;
			}
			while (pData != pEnd);
			return removed;
		}
		return 0;
	}

	void RemoveRange(UInt32 beginIdx, UInt32 count)
	{
		if (!count || (beginIdx >= numItems)) return;
		if (count > (numItems - beginIdx))
			count = numItems - beginIdx;
		T_Data *pBgn = data + beginIdx, *pEnd = pBgn + count, *pData = pBgn;
		do
		{
			pData->~T_Data();
			pData++;
		}
		while (pData != pEnd);
		UInt32 size = (UInt32)End() - (UInt32)pData;
		if (size) memmove(pBgn, pData, size);
		numItems -= count;
	}

	void Resize(UInt32 newSize)
	{
		if (numItems == newSize)
			return;
		T_Data *pData, *pEnd;
		if (numItems < newSize)
		{
			if (!data)
			{
				numAlloc = AlignNumAlloc<T_Data>(newSize);
				data = POOL_ALLOC(numAlloc, T_Data);
			}
			else if (numAlloc < newSize)
			{
				newSize = AlignNumAlloc<T_Data>(newSize);
				POOL_REALLOC(data, numAlloc, newSize, T_Data);
				numAlloc = newSize;
			}
			pData = data + numItems;
			pEnd = data + newSize;
			do
			{
				new (pData) T_Data();
				pData++;
			}
			while (pData != pEnd);
		}
		else
		{
			pData = data + newSize;
			pEnd = End();
			do
			{
				pData->~T_Data();
				pData++;
			}
			while (pData != pEnd);
		}
		numItems = newSize;
	}

	T_Data Pop()
	{
		if (!numItems) return NULL;
		numItems--;
		T_Data *pEnd = End();
		pEnd->~T_Data();
		return *pEnd;
	}

	void Clear()
	{
		if (numItems)
		{
			T_Data *pData = data, *pEnd = End();
			do
			{
				pData->~T_Data();
				pData++;
			}
			while (pData != pEnd);
		}
		numItems = 0;
	}

private:
	void QuickSortAscending(UInt32 p, UInt32 q)
	{
		if (p >= q) return;
		UInt32 i = p;
		for (UInt32 j = p + 1; j < q; j++)
		{
			if (data[p] < data[j])
				continue;
			i++;
			RawSwap<T_Data>(data[j], data[i]);
		}
		RawSwap<T_Data>(data[p], data[i]);
		QuickSortAscending(p, i);
		QuickSortAscending(i + 1, q);
	}

	void QuickSortDescending(UInt32 p, UInt32 q)
	{
		if (p >= q) return;
		UInt32 i = p;
		for (UInt32 j = p + 1; j < q; j++)
		{
			if (!(data[p] < data[j]))
				continue;
			i++;
			RawSwap<T_Data>(data[j], data[i]);
		}
		RawSwap<T_Data>(data[p], data[i]);
		QuickSortDescending(p, i);
		QuickSortDescending(i + 1, q);
	}

	void QuickSortCustom(UInt32 p, UInt32 q, SortComperator comperator)
	{
		if (p >= q) return;
		UInt32 i = p;
		for (UInt32 j = p + 1; j < q; j++)
		{
			if (comperator(data[p], data[j]))
				continue;
			i++;
			RawSwap<T_Data>(data[j], data[i]);
		}
		RawSwap<T_Data>(data[p], data[i]);
		QuickSortCustom(p, i, comperator);
		QuickSortCustom(i + 1, q, comperator);
	}
public:

	void Sort(bool descending = false)
	{
		if (descending)
			QuickSortDescending(0, numItems);
		else QuickSortAscending(0, numItems);
	}

	void Sort(SortComperator comperator)
	{
		QuickSortCustom(0, numItems, comperator);
	}

	void Shuffle()
	{
		UInt32 idx = numItems, rand;
		while (idx > 1)
		{
			rand = GetRandomUInt(idx);
			idx--;
			if (rand != idx)
				RawSwap<T_Data>(data[rand], data[idx]);
		}
	}

	class Iterator
	{
		friend Vector;

		T_Data		*pData;
		UInt32		count;

		Iterator() {}

	public:
		explicit operator bool() const {return count != 0;}
		void operator++()
		{
			pData++;
			count--;
		}
		void operator+=(UInt32 _count)
		{
			if (_count >= count)
				count = 0;
			else
			{
				pData += _count;
				count -= _count;
			}
		}

		Data_Arg operator*() const {return *pData;}
		Data_Arg operator->() const {return *pData;}
		Data_Arg operator()() const {return *pData;}
		T_Data& Ref() {return *pData;}

		Iterator(Vector &source) : pData(source.data), count(source.numItems) {}
		Iterator(Vector &source, UInt32 index) : count(source.numItems)
		{
			if (count <= index)
				count = 0;
			else
			{
				pData = source.data + index;
				count -= index;
			}
		}
	};

	class RvIterator : public Iterator
	{
	public:
		void operator--()
		{
			pData--;
			count--;
		}
		void operator-=(UInt32 _count)
		{
			if (_count >= count)
				count = 0;
			else
			{
				pData -= _count;
				count -= _count;
			}
		}

		void Remove(Vector &source)
		{
			pData->~T_Data();
			UInt32 size = source.numItems - count;
			source.numItems--;
			if (size) memmove(pData, pData + 1, size * sizeof(T_Data));
		}

		RvIterator(Vector &source)
		{
			if (count = source.numItems)
				pData = source.data + (count - 1);
		}
	};

	class CpIterator : public Iterator
	{
	public:
		CpIterator(Vector &source)
		{
			if (count = source.numItems)
			{
				UInt32 size = count * sizeof(T_Data);
				pData = (T_Data*)memcpy(GetAuxBuffer(s_auxBuffers[2], size), source.data, size);
			}
		}
	};

	Iterator Begin() {return Iterator(*this);}
	Iterator BeginAt(UInt32 index) {return Iterator(*this, index);}

	RvIterator BeginRv() {return RvIterator(*this);}

	CpIterator BeginCp() {return CpIterator(*this);}
};

template <typename T_Data, UInt32 size> class FixedTypeArray
{
	using Data_Arg = std::conditional_t<std::is_scalar_v<T_Data>, T_Data, T_Data&>;

	UInt32		numItems;
	T_Data		data[size];

public:
	FixedTypeArray() : numItems(0) {}

	UInt32 Size() const {return numItems;}
	bool Empty() const {return !numItems;}
	T_Data *Data() {return data;}

	bool Append(Data_Arg item)
	{
		if (numItems < size)
		{
			RawAssign<T_Data>(data[numItems++], item);
			return true;
		}
		return false;
	}

	T_Data *PopBack()
	{
		return numItems ? &data[--numItems] : nullptr;
	}

	class Iterator
	{
		friend FixedTypeArray;

		T_Data		*pData;
		UInt32		count;

	public:
		explicit operator bool() const {return count != 0;}
		void operator++()
		{
			pData++;
			count--;
		}

		Data_Arg operator*() const {return *pData;}
		Data_Arg operator->() const {return *pData;}
		Data_Arg operator()() const {return *pData;}
		T_Data& Ref() {return *pData;}

		Iterator(FixedTypeArray &source) : pData(source.data), count(source.numItems) {}
	};
};