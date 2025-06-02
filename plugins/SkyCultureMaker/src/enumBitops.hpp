#include <type_traits>

namespace generic_enum_bitops
{
// Extension point
template <typename T> struct allow_bitops
{
	static constexpr bool value = false;
};
}  // namespace generic_enum_bitops

template <typename T, std::enable_if_t<generic_enum_bitops::allow_bitops<T>::value, int> = 0> T operator|(T a, T b)
{
	using I = std::underlying_type_t<T>;
	return static_cast<T>(static_cast<I>(a) | static_cast<I>(b));
}

template <typename T, std::enable_if_t<generic_enum_bitops::allow_bitops<T>::value, int> = 0> T operator&(T a, T b)
{
	using I = std::underlying_type_t<T>;
	return static_cast<T>(static_cast<I>(a) & static_cast<I>(b));
}

template <typename T, std::enable_if_t<generic_enum_bitops::allow_bitops<T>::value, int> = 0> bool hasFlag(T a, T b)
{
	using I = std::underlying_type_t<T>;
	return static_cast<bool>(static_cast<I>(a) & static_cast<I>(b));
}