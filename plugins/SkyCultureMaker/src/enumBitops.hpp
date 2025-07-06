/*
 * Sky Culture Maker plug-in for Stellarium
 *
 * Copyright (C) 2025 Vincent Gerlach
 * Copyright (C) 2025 Luca-Philipp Grumbach
 * Copyright (C) 2025 Fabian Hofer
 * Copyright (C) 2025 Mher Mnatsakanyan
 * Copyright (C) 2025 Richard Hofmann
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <type_traits>

namespace generic_enum_bitops
{
// Extension point
template<typename T>
struct allow_bitops
{
	static constexpr bool value = false;
};
} // namespace generic_enum_bitops

template<typename T, std::enable_if_t<generic_enum_bitops::allow_bitops<T>::value, int> = 0>
T operator|(T a, T b)
{
	using I = std::underlying_type_t<T>;
	return static_cast<T>(static_cast<I>(a) | static_cast<I>(b));
}

template<typename T, std::enable_if_t<generic_enum_bitops::allow_bitops<T>::value, int> = 0>
T operator&(T a, T b)
{
	using I = std::underlying_type_t<T>;
	return static_cast<T>(static_cast<I>(a) & static_cast<I>(b));
}

template<typename T, std::enable_if_t<generic_enum_bitops::allow_bitops<T>::value, int> = 0>
bool hasFlag(T a, T b)
{
	using I = std::underlying_type_t<T>;
	return static_cast<bool>(static_cast<I>(a) & static_cast<I>(b));
}
