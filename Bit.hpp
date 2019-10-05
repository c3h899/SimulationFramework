#ifndef BIT_H_
#define BIT_H_

#include <cinttypes>

class Bit{
	/*
	 * CPP Should express intent.
	 * ---
	 * This Class provides bit manipulation of bytes (uint8_t)
	 * Ideally, this entire class is optimized away.
	 */
	public:
		typedef uint8_t byte;
		// GCC Throws -Wconversion when using any of the stock bitwise operators
		static constexpr byte pos(int position) { return flag(position); }
		static constexpr void set(byte& rhs, int pos) { rhs = rhs | flag(pos) ; }
		static constexpr void clear(byte& rhs, int pos) { rhs = rhs & nflag(pos); }
		static constexpr byte is_set(byte const& rhs, int pos) { return rhs & flag(pos); }
	private:
		static constexpr byte Pos[8] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};
		static constexpr byte flag(int position) { return Pos[position & 0x0F]; }
		static constexpr byte nflag(int position) { return byte(~Pos[position & 0x0F]); }
};

#endif

