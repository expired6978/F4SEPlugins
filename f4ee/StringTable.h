#pragma once

#include <memory>
#include <unordered_map>
#include <algorithm>
#include <string>

#include "f4se/GameTypes.h"

struct F4SESerializationInterface;

// Class is similar to BSFixedString, it is meant to not interfere with base game string caching when loading data early on
class F4EEFixedString
{
public:
	static size_t lowercase_hash(const char * str)
	{
		std::string xform(str);
		std::transform(xform.begin(), xform.end(), xform.begin(), ::tolower);
		return std::hash<std::string>()(xform);
	}

	F4EEFixedString() : m_internal() { }
	F4EEFixedString(const char * str) : m_internal(str), m_hash(lowercase_hash(str)) { }
	F4EEFixedString(const BSFixedString & str) : m_internal(str), m_hash(lowercase_hash(str.c_str())) { }

	bool operator==(const F4EEFixedString& x) const
	{
		return m_hash == x.m_hash;
	}

	operator BSFixedString() const { return BSFixedString(m_internal.c_str()); }
	BSFixedString AsBSFixedString() const { return operator BSFixedString(); }

	const char * c_str() const { return operator const char *(); }
	operator const char *() const { return m_internal.c_str(); }

	size_t GetHash() const { return m_hash; }

protected:
	std::string		m_internal;
	size_t			m_hash;
};

typedef std::shared_ptr<F4EEFixedString> StringTableItem;
typedef std::weak_ptr<F4EEFixedString> WeakTableItem;

namespace std {
	template <> struct hash < F4EEFixedString >
	{
		size_t operator()(const F4EEFixedString & x) const
		{
			return x.GetHash();
		}
	};
	template <> struct hash<StringTableItem>
	{
		size_t operator()(const StringTableItem & x) const
		{
			return x->GetHash();
		}
	};
}

class StringTable
{
public:

	enum
	{
		kSerializationVersion = 1,
	};

	void Save(const F4SESerializationInterface * intfc, UInt32 kVersion);
	bool Load(const F4SESerializationInterface * intfc, UInt32 kVersion, std::unordered_map<UInt32, StringTableItem> & stringTable);
	void Revert();

	StringTableItem GetString(const F4EEFixedString & str);

	UInt32 GetStringID(const StringTableItem & str);

	void RemoveString(const F4EEFixedString & str);

protected:
	std::unordered_map<F4EEFixedString, WeakTableItem> m_table;
	std::vector<WeakTableItem> m_tableVector;
	SimpleLock	m_lock;
};