
class ByteOrder {
public:
	//==============================================================================
	/** Swaps the upper and lower bytes of a 16-bit integer. */
	constexpr static uint16_t swap(uint16_t value) noexcept;

	/** Swaps the upper and lower bytes of a 16-bit integer. */
	constexpr static int16_t swap(int16_t value) noexcept;

	/** Reverses the order of the 4 bytes in a 32-bit integer. */
	static uint32_t swap(uint32_t value) noexcept;

	/** Reverses the order of the 4 bytes in a 32-bit integer. */
	static int32_t swap(int32_t value) noexcept;

	/** Returns a garbled float which has the reverse byte-order of the original. */
	static float swap(float value) noexcept;

	/** Returns a garbled double which has the reverse byte-order of the original. */
	static double swap(double value) noexcept;

	//==============================================================================
	/** Swaps the byte order of a signed or unsigned integer if the CPU is big-endian */
	template<typename Type> static Type swapIfBigEndian(Type value) noexcept { return value; }

	/** Swaps the byte order of a signed or unsigned integer if the CPU is little-endian */
	template<typename Type> static Type swapIfLittleEndian(Type value) noexcept { return swap(value); }

	//==============================================================================
	/** Turns 4 bytes into a little-endian integer. */
	constexpr static uint32_t littleEndianInt(const void *bytes) noexcept;

	/** Turns 2 bytes into a little-endian integer. */
	constexpr static uint16_t littleEndianShort(const void *bytes) noexcept;

	/** Converts 3 little-endian bytes into a signed 24-bit value (which is sign-extended to 32 bits). */
	constexpr static int littleEndian24Bit(const void *bytes) noexcept;

	/** Copies a 24-bit number to 3 little-endian bytes. */
	static void littleEndian24BitToChars(int32_t value, void *destBytes) noexcept;

	//==============================================================================
	/** Turns 4 bytes into a big-endian integer. */
	constexpr static uint32_t bigEndianInt(const void *bytes) noexcept;

	/** Turns 2 bytes into a big-endian integer. */
	constexpr static uint16_t bigEndianShort(const void *bytes) noexcept;

	/** Converts 3 big-endian bytes into a signed 24-bit value (which is sign-extended to 32 bits). */
	constexpr static int bigEndian24Bit(const void *bytes) noexcept;

	/** Copies a 24-bit number to 3 big-endian bytes. */
	static void bigEndian24BitToChars(int32_t value, void *destBytes) noexcept;

	//==============================================================================
	/** Constructs a 16-bit integer from its constituent bytes, in order of significance. */
	constexpr static uint16_t makeInt(uint8_t leastSig, uint8_t mostSig) noexcept;

	/** Constructs a 32-bit integer from its constituent bytes, in order of significance. */
	constexpr static uint32_t makeInt(uint8_t leastSig, uint8_t byte1, uint8_t byte2, uint8_t mostSig) noexcept;

private:
	ByteOrder() = delete;
};

//==============================================================================
constexpr inline uint16_t ByteOrder::swap(uint16_t v) noexcept
{
	return static_cast<uint16_t>((v << 8) | (v >> 8));
}
constexpr inline int16_t ByteOrder::swap(int16_t v) noexcept
{
	return static_cast<int16_t>(swap(static_cast<uint16_t>(v)));
}
inline int32_t ByteOrder::swap(int32_t v) noexcept
{
	return static_cast<int32_t>(swap(static_cast<uint32_t>(v)));
}

inline float ByteOrder::swap(float v) noexcept
{
	union {
		uint32_t asUInt;
		float asFloat;
	} n;
	n.asFloat = v;
	n.asUInt = swap(n.asUInt);
	return n.asFloat;
}

#pragma intrinsic(_byteswap_ulong)

inline uint32_t ByteOrder::swap(uint32_t n) noexcept
{
	return _byteswap_ulong(n);
}

constexpr inline uint16_t ByteOrder::makeInt(uint8_t b0, uint8_t b1) noexcept
{
	return static_cast<uint16_t>(static_cast<uint16_t>(b0) | (static_cast<uint16_t>(b1) << 8));
}

constexpr inline uint32_t ByteOrder::makeInt(uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3) noexcept
{
	return static_cast<uint32_t>(b0) | (static_cast<uint32_t>(b1) << 8) | (static_cast<uint32_t>(b2) << 16) |
	       (static_cast<uint32_t>(b3) << 24);
}

constexpr inline uint16_t ByteOrder::littleEndianShort(const void *bytes) noexcept
{
	return makeInt(static_cast<const uint8_t *>(bytes)[0], static_cast<const uint8_t *>(bytes)[1]);
}
constexpr inline uint32_t ByteOrder::littleEndianInt(const void *bytes) noexcept
{
	return makeInt(static_cast<const uint8_t *>(bytes)[0], static_cast<const uint8_t *>(bytes)[1],
		       static_cast<const uint8_t *>(bytes)[2], static_cast<const uint8_t *>(bytes)[3]);
}

constexpr inline uint16_t ByteOrder::bigEndianShort(const void *bytes) noexcept
{
	return makeInt(static_cast<const uint8_t *>(bytes)[1], static_cast<const uint8_t *>(bytes)[0]);
}
constexpr inline uint32_t ByteOrder::bigEndianInt(const void *bytes) noexcept
{
	return makeInt(static_cast<const uint8_t *>(bytes)[3], static_cast<const uint8_t *>(bytes)[2],
		       static_cast<const uint8_t *>(bytes)[1], static_cast<const uint8_t *>(bytes)[0]);
}

constexpr inline int32_t ByteOrder::littleEndian24Bit(const void *bytes) noexcept
{
	return (int32_t)((((uint32_t) static_cast<const int8_t *>(bytes)[2]) << 16) |
			 (((uint32_t) static_cast<const uint8_t *>(bytes)[1]) << 8) |
			 ((uint32_t) static_cast<const uint8_t *>(bytes)[0]));
}
constexpr inline int32_t ByteOrder::bigEndian24Bit(const void *bytes) noexcept
{
	return (int32_t)((((uint32_t) static_cast<const int8_t *>(bytes)[0]) << 16) |
			 (((uint32_t) static_cast<const uint8_t *>(bytes)[1]) << 8) |
			 ((uint32_t) static_cast<const uint8_t *>(bytes)[2]));
}

inline void ByteOrder::littleEndian24BitToChars(int32_t value, void *destBytes) noexcept
{
	static_cast<uint8_t *>(destBytes)[0] = (uint8_t)value;
	static_cast<uint8_t *>(destBytes)[1] = (uint8_t)(value >> 8);
	static_cast<uint8_t *>(destBytes)[2] = (uint8_t)(value >> 16);
}
inline void ByteOrder::bigEndian24BitToChars(int32_t value, void *destBytes) noexcept
{
	static_cast<uint8_t *>(destBytes)[0] = (uint8_t)(value >> 16);
	static_cast<uint8_t *>(destBytes)[1] = (uint8_t)(value >> 8);
	static_cast<uint8_t *>(destBytes)[2] = (uint8_t)value;
}
