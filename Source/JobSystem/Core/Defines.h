#pragma once
#include <type_traits>

#define NONCOPYABLE(Type) \
private: \
	Type(const Type&) = delete; \
	Type& operator=(const Type&) = delete;

#define NONMOVABLE(Type) \
private: \
	Type(Type&&) = delete; \
	Type& operator=(Type&&) = delete;

#define NONCOPYABLE_NONMOVABLE(Type) \
	NONCOPYABLE(Type); \
	NONMOVABLE(Type)






#define ENUM_CLASS_FLAGS(Enum) \
	inline constexpr Enum& operator~ (Enum e) {return (Enum)~(std::underlying_type_t(Enum))e; }




// Core/Misc
template<typename Enum>
constexpr bool EnumHasAnyFlags(Enum flagsSet, Enum flagsSubset)
{
	using UnderlyingType = std::underlying_type_t(Enum);
	return ((UnderlyingType)flagsSet & (UnderlyingType)flagsSubset) == (UnderlyingType)flagsSubset;
}
