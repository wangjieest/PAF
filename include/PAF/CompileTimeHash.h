#pragma once
// Compile Time MurmurHash64B in C++
// It's easy to make sure implementation which has no conflicts on different names.

#include <cstdint>

// 64-bit hash for 32-bit platforms
namespace murmur64b
{
#define MURMUR_NOEXCEPT
constexpr auto SEED_B = 0x5bd1e995ull;
constexpr uint32_t hash_sub(uint32_t h, uint32_t k) MURMUR_NOEXCEPT
{
	return h ^ (SEED_B * (k ^ (k >> 24)));
}
constexpr uint32_t hash_helper(uint32_t h, uint32_t k) MURMUR_NOEXCEPT
{
	return hash_sub(SEED_B * h, SEED_B * k);
}
constexpr uint64_t hash_final_4(uint32_t x, uint32_t y) MURMUR_NOEXCEPT
{
	return ((uint64_t)x << 32) | (uint64_t)y;
}
constexpr uint64_t hash_final_3(uint32_t x, uint32_t y) MURMUR_NOEXCEPT
{
	return hash_final_4(x, SEED_B * ((x >> 19) ^ y));
}
constexpr uint64_t hash_final_2(uint32_t x, uint32_t y) MURMUR_NOEXCEPT
{
	return hash_final_3(SEED_B * ((y >> 17) ^ x), y);
}
constexpr uint64_t hash_final_1(uint32_t x, uint32_t y) MURMUR_NOEXCEPT
{
	return hash_final_2(x, SEED_B * ((x >> 22) ^ y));
}
constexpr uint64_t hash_final(uint32_t x, uint32_t y) MURMUR_NOEXCEPT
{
	return hash_final_1(SEED_B * ((y >> 18) ^ x), y);
}
template <typename... C>
constexpr uint64_t hash_update(uint32_t h1, uint32_t h2, C... c) MURMUR_NOEXCEPT;
template <>
constexpr uint64_t hash_update(uint32_t h1, uint32_t h2) MURMUR_NOEXCEPT
{
	return hash_final(h1, h2);
}
template <>
constexpr uint64_t hash_update(uint32_t h1, uint32_t h2, uint8_t c0) MURMUR_NOEXCEPT
{
	return hash_update(h1, SEED_B * (h2 ^ c0));
}
template <>
constexpr uint64_t hash_update(uint32_t h1, uint32_t h2, uint8_t c0, uint8_t c1) MURMUR_NOEXCEPT
{
	return hash_update(h1, h2 ^ ((uint32_t)c1 << 8), c0);
}
template <>
constexpr uint64_t hash_update(uint32_t h1,
							   uint32_t h2,
							   uint8_t c0,
							   uint8_t c1,
							   uint8_t c2) MURMUR_NOEXCEPT
{
	return hash_update(h1, h2 ^ ((uint32_t)c2 << 16), c0, c1);
}
template <typename... C>
constexpr uint64_t hash_update(uint32_t h1,
							   uint32_t h2,
							   uint8_t c0,
							   uint8_t c1,
							   uint8_t c2,
							   uint8_t c3,
							   C... c) MURMUR_NOEXCEPT
{
	return hash_update(
		hash_helper(h1,
					((uint32_t)c3 << 24) | ((uint32_t)c2 << 16) | ((uint32_t)c1 << 8) | (uint32_t)c0),
		h2,
		c...);
}
template <typename... C>
constexpr uint64_t hash_update(uint32_t h1,
							   uint32_t h2,
							   uint8_t c0,
							   uint8_t c1,
							   uint8_t c2,
							   uint8_t c3,
							   uint8_t c4,
							   uint8_t c5,
							   uint8_t c6,
							   uint8_t c7,
							   C... c) MURMUR_NOEXCEPT
{
	return hash_update(
		hash_helper(h1,
					((uint32_t)c3 << 24) | ((uint32_t)c2 << 16) | ((uint32_t)c1 << 8) | (uint32_t)c0),
		hash_helper(h2,
					((uint32_t)c7 << 24) | ((uint32_t)c6 << 16) | ((uint32_t)c5 << 8) | (uint32_t)c4),
		c...);
}

template <uint32_t...>
struct Sequence
{
};
template <uint32_t N, uint32_t... S>
struct CreateSequence : CreateSequence<N - 1, N - 1, S...>
{
};
template <uint32_t... S>
struct CreateSequence<0, S...>
{
	using Type = Sequence<S...>;
};
template <uint32_t N, uint32_t... S>
constexpr uint64_t Unpack(uint32_t seed, const char (&s)[N], Sequence<S...>) MURMUR_NOEXCEPT
{
	return hash_update(seed ^ (N - 1), 0, (uint8_t)s[S]...);
}
template <uint32_t N>
constexpr uint64_t MurmurHash64B(const char (&s)[N], uint32_t seed = 0) MURMUR_NOEXCEPT
{
	return murmur64b::Unpack(seed, s, typename CreateSequence<N - 1>::Type());
}
template <uint64_t N>
struct constexprN
{
	enum : uint64_t
	{
		value = N
	};
};

inline uint64_t MurmurHash64B(const void* key, uint32_t len, uint32_t seed = 0) MURMUR_NOEXCEPT
{
	const uint32_t m = 0x5bd1e995;
	const int r = 24;
	uint32_t h1 = seed ^ len;
	uint32_t h2 = 0;
	const uint32_t* data = (const uint32_t*)key;
	while (len >= 8)
	{
		uint32_t k1 = *data++;
		k1 *= m;
		k1 ^= k1 >> r;
		k1 *= m;
		h1 *= m;
		h1 ^= k1;
		len -= 4;
		uint32_t k2 = *data++;
		k2 *= m;
		k2 ^= k2 >> r;
		k2 *= m;
		h2 *= m;
		h2 ^= k2;
		len -= 4;
	}
	if (len >= 4)
	{
		uint32_t k1 = *data++;
		k1 *= m;
		k1 ^= k1 >> r;
		k1 *= m;
		h1 *= m;
		h1 ^= k1;
		len -= 4;
	}
	switch (len)
	{
	case 3:
		h2 ^= (((char*)data)[2]) << 16;
	case 2:
		h2 ^= (((char*)data)[1]) << 8;
	case 1:
		h2 ^= (((char*)data)[0]);
		h2 *= m;
	};
	h1 ^= h2 >> 18;
	h1 *= m;
	h2 ^= h1 >> 22;
	h2 *= m;
	h1 ^= h2 >> 17;
	h1 *= m;
	h2 ^= h1 >> 19;
	h2 *= m;
	uint64_t h = h1;
	h = (h << 32) | h2;
	return h;
}
inline uint32_t MurmurHash32(const void* key, uint32_t len, uint32_t seed = 0) MURMUR_NOEXCEPT
{
	// 'm' and 'r' are mixing constants generated offline.
	// They're not really 'magic', they just happen to work well.
	const uint32_t m = 0xc6a4a793;
	const int r = 16;
	// Initialize the hash to a 'random' value
	uint32_t h = seed ^ (len * m);
	// Mix 4 bytes at a time into the hash
	const unsigned char* data = (const unsigned char*)key;
	while (len >= 4)
	{
		h += *(unsigned int*)data;
		h *= m;
		h ^= h >> r;
		data += 4;
		len -= 4;
	}
	// Handle the last few bytes of the input array
	switch (len)
	{
	case 3:
		h += data[2] << 16;
	case 2:
		h += data[1] << 8;
	case 1:
		h += data[0];
		h *= m;
		h ^= h >> r;
	};
	// Do a few final mixes of the hash to ensure the last few
	// bytes are well-incorporated.
	h *= m;
	h ^= h >> 10;
	h *= m;
	h ^= h >> 17;
	return h;
}
} // namespace murmur64b

#define PAF_TO_STR_(name) #name
#define PAF_TO_STR(name) PAF_TO_STR_(name)

#define COMPILE_TIME_HASH_STR(str) murmur64b::constexprN<murmur64b::MurmurHash64B(str)>::value
#define COMPILE_TIME_HASH(name) COMPILE_TIME_HASH_STR(PAF_TO_STR(name))
