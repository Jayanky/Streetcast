#ifndef SC_EMIT_MIPS_H_
#define SC_EMIT_MIPS_H_

#include <common.h>
#include <debug.h>

typedef enum {
    EMIT_MIPS_OP_CODE_LUI = 0b001111,
    EMIT_MIPS_OP_CODE_ORI = 0b001101,
    EMIT_MIPS_OP_CODE_SPECIAL = 0b000000,
    EMIT_MIPS_OP_CODE_SW = 0b101011,
} EmitMipsOpCode;

inline u32 emitMipsFormatR(u8 opcode, u8 rs, u8 rt, u8 rd, u8 shift, u8 function) {
    debugBlock(
        debugAssert(opcode < 1 << 6, "Oversized opcode!", NULL);
        debugAssert(rs < 1 << 5, "Oversized rs!", NULL);
        debugAssert(rt < 1 << 5, "Oversized rt!", NULL);
        debugAssert(rd < 1 << 5, "Oversized rd!", NULL);
        debugAssert(shift < 1 << 5, "Oversized shift!", NULL);
        debugAssert(function < 1 << 6, "Oversized function!", NULL);
    )

    return opcode << 26 | rs << 21 | rt << 16 | rd << 11 | shift << 6 | function;
}

inline u32 emitMipsFormatI(u8 opcode, u8 rs, u8 rt, u16 immediate) {
    debugBlock(
        debugAssert(opcode < 1 << 6, "Oversized opcode!", NULL);
        debugAssert(rs < 1 << 5, "Oversized rs!", NULL);
        debugAssert(rt < 1 << 5, "Oversized rt!", NULL);
    )

    return opcode << 26 | rs << 21 | rt << 16 | immediate;
}

inline u32 emitMipsFormatJ(u8 opcode, u32 immediate) {
    debugBlock(
        debugAssert(opcode < 1 << 6, "Oversized opcode!", NULL);
        debugAssert(immediate < 1 << 26, "Oversized immediate!", NULL);
    )

    return opcode << 26 | immediate;
}

inline u32 emitMipsADD(u8 rd, u8 rs, u8 rt) {
    return emitMipsFormatR(EMIT_MIPS_OP_CODE_SPECIAL, rs, rt, rd, 0, 0b100000);
}

inline u32 emitMipsJR(u8 rs) {
    return emitMipsFormatR(EMIT_MIPS_OP_CODE_SPECIAL, rs, 0, 0, 0, 0b001000);
}

inline u32 emitMipsLUI(u8 rt, u16 immediate) {
    return emitMipsFormatI(EMIT_MIPS_OP_CODE_LUI, 0, rt, immediate);
}

inline u32 emitMipsORI(u8 rt, u8 rs, u16 immediate) {
    return emitMipsFormatI(EMIT_MIPS_OP_CODE_ORI, rs, rt, immediate);
}

inline u32 emitMipsSW(u8 rt, i16 offset, u8 base) {
    return emitMipsFormatI(EMIT_MIPS_OP_CODE_SW, base, rt, (u16)offset);
}

#endif