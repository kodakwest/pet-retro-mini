#include "cpu.h"

#include <string.h>

enum addr_mode {
    AM_IMP, AM_ACC, AM_IMM, AM_ZP, AM_ZPX, AM_ZPY, AM_ABS, AM_ABX, AM_ABY,
    AM_IND, AM_INX, AM_INY, AM_REL
};

static uint8_t rd(cpu_t *c, uint16_t a) { return c->read(c->user, a); }
static void wr(cpu_t *c, uint16_t a, uint8_t v) { c->write(c->user, a, v); }
static uint8_t fetch(cpu_t *c) { return rd(c, c->pc++); }
static uint16_t fetch16(cpu_t *c) { uint8_t l = fetch(c); return (uint16_t)(l | ((uint16_t)fetch(c) << 8)); }
static uint16_t read16(cpu_t *c, uint16_t a) { return (uint16_t)(rd(c, a) | ((uint16_t)rd(c, (uint16_t)(a + 1)) << 8)); }
static uint16_t read16_bug(cpu_t *c, uint16_t a) { return (uint16_t)(rd(c, a) | ((uint16_t)rd(c, (uint16_t)((a & 0xff00) | ((a + 1) & 0x00ff))) << 8)); }
static int getf(cpu_t *c, uint8_t f) { return (c->p & f) != 0; }
static void setf(cpu_t *c, uint8_t f, int v) { if (v) c->p |= f; else c->p &= (uint8_t)~f; c->p |= CPU_FLAG_U; }
static void setzn(cpu_t *c, uint8_t v) { setf(c, CPU_FLAG_Z, v == 0); setf(c, CPU_FLAG_N, (v & 0x80) != 0); }
static void push(cpu_t *c, uint8_t v) { wr(c, (uint16_t)(0x0100 | c->sp--), v); }
static uint8_t pop(cpu_t *c) { c->sp++; return rd(c, (uint16_t)(0x0100 | c->sp)); }
static void push16(cpu_t *c, uint16_t v) { push(c, (uint8_t)(v >> 8)); push(c, (uint8_t)v); }
static uint16_t pop16(cpu_t *c) { uint8_t l = pop(c); return (uint16_t)(l | ((uint16_t)pop(c) << 8)); }

static uint16_t addr(cpu_t *c, enum addr_mode m)
{
    uint16_t a;
    switch (m) {
    case AM_ZP: return fetch(c);
    case AM_ZPX: return (uint8_t)(fetch(c) + c->x);
    case AM_ZPY: return (uint8_t)(fetch(c) + c->y);
    case AM_ABS: return fetch16(c);
    case AM_ABX: return (uint16_t)(fetch16(c) + c->x);
    case AM_ABY: return (uint16_t)(fetch16(c) + c->y);
    case AM_IND: return read16_bug(c, fetch16(c));
    case AM_INX: a = (uint8_t)(fetch(c) + c->x); return (uint16_t)(rd(c, a) | ((uint16_t)rd(c, (uint8_t)(a + 1)) << 8));
    case AM_INY: a = fetch(c); return (uint16_t)((rd(c, a) | ((uint16_t)rd(c, (uint8_t)(a + 1)) << 8)) + c->y);
    default: return 0;
    }
}

static uint8_t val(cpu_t *c, enum addr_mode m)
{
    if (m == AM_IMM) return fetch(c);
    if (m == AM_ACC) return c->a;
    return rd(c, addr(c, m));
}

static void branch(cpu_t *c, int cond)
{
    int8_t off = (int8_t)fetch(c);
    if (cond) c->pc = (uint16_t)(c->pc + off);
}

static void adc(cpu_t *c, uint8_t v)
{
    uint16_t sum;
    if (getf(c, CPU_FLAG_D)) {
        uint8_t lo = (uint8_t)((c->a & 0x0f) + (v & 0x0f) + getf(c, CPU_FLAG_C));
        uint8_t hi = (uint8_t)((c->a >> 4) + (v >> 4));
        if (lo > 9) { lo = (uint8_t)(lo + 6); hi++; }
        setf(c, CPU_FLAG_Z, ((c->a + v + getf(c, CPU_FLAG_C)) & 0xff) == 0);
        setf(c, CPU_FLAG_N, (hi & 0x08) != 0);
        setf(c, CPU_FLAG_V, (~(c->a ^ v) & (c->a ^ ((hi << 4) | (lo & 0x0f))) & 0x80) != 0);
        if (hi > 9) hi = (uint8_t)(hi + 6);
        setf(c, CPU_FLAG_C, hi > 15);
        c->a = (uint8_t)((hi << 4) | (lo & 0x0f));
        return;
    }
    sum = (uint16_t)c->a + v + getf(c, CPU_FLAG_C);
    setf(c, CPU_FLAG_C, sum > 0xff);
    setf(c, CPU_FLAG_V, (~(c->a ^ v) & (c->a ^ sum) & 0x80) != 0);
    c->a = (uint8_t)sum;
    setzn(c, c->a);
}

static void sbc(cpu_t *c, uint8_t v) { adc(c, (uint8_t)(v ^ 0xff)); }
static void cmp(cpu_t *c, uint8_t r, uint8_t v) { uint16_t t = (uint16_t)r - v; setf(c, CPU_FLAG_C, r >= v); setzn(c, (uint8_t)t); }
static uint8_t asl(cpu_t *c, uint8_t v) { setf(c, CPU_FLAG_C, v & 0x80); v = (uint8_t)(v << 1); setzn(c, v); return v; }
static uint8_t lsr(cpu_t *c, uint8_t v) { setf(c, CPU_FLAG_C, v & 1); v = (uint8_t)(v >> 1); setzn(c, v); return v; }
static uint8_t rol(cpu_t *c, uint8_t v) { int carry = getf(c, CPU_FLAG_C); setf(c, CPU_FLAG_C, v & 0x80); v = (uint8_t)((v << 1) | carry); setzn(c, v); return v; }
static uint8_t ror(cpu_t *c, uint8_t v) { int carry = getf(c, CPU_FLAG_C); setf(c, CPU_FLAG_C, v & 1); v = (uint8_t)((v >> 1) | (carry ? 0x80 : 0)); setzn(c, v); return v; }

static void interrupt(cpu_t *c, uint16_t vector, int brk)
{
    push16(c, c->pc);
    push(c, (uint8_t)(c->p | CPU_FLAG_U | (brk ? CPU_FLAG_B : 0)));
    setf(c, CPU_FLAG_I, 1);
    c->pc = read16(c, vector);
}

void cpu_init(cpu_t *cpu, cpu_read_fn read_fn, cpu_write_fn write_fn, void *user)
{
    memset(cpu, 0, sizeof(*cpu));
    cpu->read = read_fn;
    cpu->write = write_fn;
    cpu->user = user;
    cpu->sp = 0xff;
    cpu->p = CPU_FLAG_U | CPU_FLAG_I;
}

void cpu_reset(cpu_t *cpu)
{
    cpu->sp = 0xfd;
    cpu->p = CPU_FLAG_U | CPU_FLAG_I;
    cpu->pc = read16(cpu, 0xfffc);
    cpu->stopped = 0;
}

int cpu_step(cpu_t *c)
{
    uint8_t op;
    uint16_t a;
    uint8_t v;
    if (c->stopped) return 0;
    op = fetch(c);
    c->cycles++;
    switch (op) {
    case 0x00: c->pc++; interrupt(c, 0xfffe, 1); break;
    case 0xea: break;
    case 0xa9: c->a = val(c, AM_IMM); setzn(c, c->a); break;
    case 0xa5: c->a = val(c, AM_ZP); setzn(c, c->a); break;
    case 0xb5: c->a = val(c, AM_ZPX); setzn(c, c->a); break;
    case 0xad: c->a = val(c, AM_ABS); setzn(c, c->a); break;
    case 0xbd: c->a = val(c, AM_ABX); setzn(c, c->a); break;
    case 0xb9: c->a = val(c, AM_ABY); setzn(c, c->a); break;
    case 0xa1: c->a = val(c, AM_INX); setzn(c, c->a); break;
    case 0xb1: c->a = val(c, AM_INY); setzn(c, c->a); break;
    case 0xa2: c->x = val(c, AM_IMM); setzn(c, c->x); break;
    case 0xa6: c->x = val(c, AM_ZP); setzn(c, c->x); break;
    case 0xb6: c->x = val(c, AM_ZPY); setzn(c, c->x); break;
    case 0xae: c->x = val(c, AM_ABS); setzn(c, c->x); break;
    case 0xbe: c->x = val(c, AM_ABY); setzn(c, c->x); break;
    case 0xa0: c->y = val(c, AM_IMM); setzn(c, c->y); break;
    case 0xa4: c->y = val(c, AM_ZP); setzn(c, c->y); break;
    case 0xb4: c->y = val(c, AM_ZPX); setzn(c, c->y); break;
    case 0xac: c->y = val(c, AM_ABS); setzn(c, c->y); break;
    case 0xbc: c->y = val(c, AM_ABX); setzn(c, c->y); break;
    case 0x85: wr(c, addr(c, AM_ZP), c->a); break;
    case 0x95: wr(c, addr(c, AM_ZPX), c->a); break;
    case 0x8d: wr(c, addr(c, AM_ABS), c->a); break;
    case 0x9d: wr(c, addr(c, AM_ABX), c->a); break;
    case 0x99: wr(c, addr(c, AM_ABY), c->a); break;
    case 0x81: wr(c, addr(c, AM_INX), c->a); break;
    case 0x91: wr(c, addr(c, AM_INY), c->a); break;
    case 0x86: wr(c, addr(c, AM_ZP), c->x); break;
    case 0x96: wr(c, addr(c, AM_ZPY), c->x); break;
    case 0x8e: wr(c, addr(c, AM_ABS), c->x); break;
    case 0x84: wr(c, addr(c, AM_ZP), c->y); break;
    case 0x94: wr(c, addr(c, AM_ZPX), c->y); break;
    case 0x8c: wr(c, addr(c, AM_ABS), c->y); break;
    case 0x69: adc(c, val(c, AM_IMM)); break;
    case 0x65: adc(c, val(c, AM_ZP)); break;
    case 0x75: adc(c, val(c, AM_ZPX)); break;
    case 0x6d: adc(c, val(c, AM_ABS)); break;
    case 0x7d: adc(c, val(c, AM_ABX)); break;
    case 0x79: adc(c, val(c, AM_ABY)); break;
    case 0x61: adc(c, val(c, AM_INX)); break;
    case 0x71: adc(c, val(c, AM_INY)); break;
    case 0xe9: case 0xeb: sbc(c, val(c, AM_IMM)); break;
    case 0xe5: sbc(c, val(c, AM_ZP)); break;
    case 0xf5: sbc(c, val(c, AM_ZPX)); break;
    case 0xed: sbc(c, val(c, AM_ABS)); break;
    case 0xfd: sbc(c, val(c, AM_ABX)); break;
    case 0xf9: sbc(c, val(c, AM_ABY)); break;
    case 0xe1: sbc(c, val(c, AM_INX)); break;
    case 0xf1: sbc(c, val(c, AM_INY)); break;
    case 0x29: c->a &= val(c, AM_IMM); setzn(c, c->a); break;
    case 0x25: c->a &= val(c, AM_ZP); setzn(c, c->a); break;
    case 0x35: c->a &= val(c, AM_ZPX); setzn(c, c->a); break;
    case 0x2d: c->a &= val(c, AM_ABS); setzn(c, c->a); break;
    case 0x3d: c->a &= val(c, AM_ABX); setzn(c, c->a); break;
    case 0x39: c->a &= val(c, AM_ABY); setzn(c, c->a); break;
    case 0x21: c->a &= val(c, AM_INX); setzn(c, c->a); break;
    case 0x31: c->a &= val(c, AM_INY); setzn(c, c->a); break;
    case 0x09: c->a |= val(c, AM_IMM); setzn(c, c->a); break;
    case 0x05: c->a |= val(c, AM_ZP); setzn(c, c->a); break;
    case 0x15: c->a |= val(c, AM_ZPX); setzn(c, c->a); break;
    case 0x0d: c->a |= val(c, AM_ABS); setzn(c, c->a); break;
    case 0x1d: c->a |= val(c, AM_ABX); setzn(c, c->a); break;
    case 0x19: c->a |= val(c, AM_ABY); setzn(c, c->a); break;
    case 0x01: c->a |= val(c, AM_INX); setzn(c, c->a); break;
    case 0x11: c->a |= val(c, AM_INY); setzn(c, c->a); break;
    case 0x49: c->a ^= val(c, AM_IMM); setzn(c, c->a); break;
    case 0x45: c->a ^= val(c, AM_ZP); setzn(c, c->a); break;
    case 0x55: c->a ^= val(c, AM_ZPX); setzn(c, c->a); break;
    case 0x4d: c->a ^= val(c, AM_ABS); setzn(c, c->a); break;
    case 0x5d: c->a ^= val(c, AM_ABX); setzn(c, c->a); break;
    case 0x59: c->a ^= val(c, AM_ABY); setzn(c, c->a); break;
    case 0x41: c->a ^= val(c, AM_INX); setzn(c, c->a); break;
    case 0x51: c->a ^= val(c, AM_INY); setzn(c, c->a); break;
    case 0xc9: cmp(c, c->a, val(c, AM_IMM)); break;
    case 0xc5: cmp(c, c->a, val(c, AM_ZP)); break;
    case 0xd5: cmp(c, c->a, val(c, AM_ZPX)); break;
    case 0xcd: cmp(c, c->a, val(c, AM_ABS)); break;
    case 0xdd: cmp(c, c->a, val(c, AM_ABX)); break;
    case 0xd9: cmp(c, c->a, val(c, AM_ABY)); break;
    case 0xc1: cmp(c, c->a, val(c, AM_INX)); break;
    case 0xd1: cmp(c, c->a, val(c, AM_INY)); break;
    case 0xe0: cmp(c, c->x, val(c, AM_IMM)); break;
    case 0xe4: cmp(c, c->x, val(c, AM_ZP)); break;
    case 0xec: cmp(c, c->x, val(c, AM_ABS)); break;
    case 0xc0: cmp(c, c->y, val(c, AM_IMM)); break;
    case 0xc4: cmp(c, c->y, val(c, AM_ZP)); break;
    case 0xcc: cmp(c, c->y, val(c, AM_ABS)); break;
    case 0x0a: c->a = asl(c, c->a); break;
    case 0x06: a = addr(c, AM_ZP); wr(c, a, asl(c, rd(c, a))); break;
    case 0x16: a = addr(c, AM_ZPX); wr(c, a, asl(c, rd(c, a))); break;
    case 0x0e: a = addr(c, AM_ABS); wr(c, a, asl(c, rd(c, a))); break;
    case 0x1e: a = addr(c, AM_ABX); wr(c, a, asl(c, rd(c, a))); break;
    case 0x4a: c->a = lsr(c, c->a); break;
    case 0x46: a = addr(c, AM_ZP); wr(c, a, lsr(c, rd(c, a))); break;
    case 0x56: a = addr(c, AM_ZPX); wr(c, a, lsr(c, rd(c, a))); break;
    case 0x4e: a = addr(c, AM_ABS); wr(c, a, lsr(c, rd(c, a))); break;
    case 0x5e: a = addr(c, AM_ABX); wr(c, a, lsr(c, rd(c, a))); break;
    case 0x2a: c->a = rol(c, c->a); break;
    case 0x26: a = addr(c, AM_ZP); wr(c, a, rol(c, rd(c, a))); break;
    case 0x36: a = addr(c, AM_ZPX); wr(c, a, rol(c, rd(c, a))); break;
    case 0x2e: a = addr(c, AM_ABS); wr(c, a, rol(c, rd(c, a))); break;
    case 0x3e: a = addr(c, AM_ABX); wr(c, a, rol(c, rd(c, a))); break;
    case 0x6a: c->a = ror(c, c->a); break;
    case 0x66: a = addr(c, AM_ZP); wr(c, a, ror(c, rd(c, a))); break;
    case 0x76: a = addr(c, AM_ZPX); wr(c, a, ror(c, rd(c, a))); break;
    case 0x6e: a = addr(c, AM_ABS); wr(c, a, ror(c, rd(c, a))); break;
    case 0x7e: a = addr(c, AM_ABX); wr(c, a, ror(c, rd(c, a))); break;
    case 0xe6: a = addr(c, AM_ZP); v = (uint8_t)(rd(c, a) + 1); wr(c, a, v); setzn(c, v); break;
    case 0xf6: a = addr(c, AM_ZPX); v = (uint8_t)(rd(c, a) + 1); wr(c, a, v); setzn(c, v); break;
    case 0xee: a = addr(c, AM_ABS); v = (uint8_t)(rd(c, a) + 1); wr(c, a, v); setzn(c, v); break;
    case 0xfe: a = addr(c, AM_ABX); v = (uint8_t)(rd(c, a) + 1); wr(c, a, v); setzn(c, v); break;
    case 0xc6: a = addr(c, AM_ZP); v = (uint8_t)(rd(c, a) - 1); wr(c, a, v); setzn(c, v); break;
    case 0xd6: a = addr(c, AM_ZPX); v = (uint8_t)(rd(c, a) - 1); wr(c, a, v); setzn(c, v); break;
    case 0xce: a = addr(c, AM_ABS); v = (uint8_t)(rd(c, a) - 1); wr(c, a, v); setzn(c, v); break;
    case 0xde: a = addr(c, AM_ABX); v = (uint8_t)(rd(c, a) - 1); wr(c, a, v); setzn(c, v); break;
    case 0xe8: c->x++; setzn(c, c->x); break;
    case 0xc8: c->y++; setzn(c, c->y); break;
    case 0xca: c->x--; setzn(c, c->x); break;
    case 0x88: c->y--; setzn(c, c->y); break;
    case 0xaa: c->x = c->a; setzn(c, c->x); break;
    case 0xa8: c->y = c->a; setzn(c, c->y); break;
    case 0x8a: c->a = c->x; setzn(c, c->a); break;
    case 0x98: c->a = c->y; setzn(c, c->a); break;
    case 0xba: c->x = c->sp; setzn(c, c->x); break;
    case 0x9a: c->sp = c->x; break;
    case 0x48: push(c, c->a); break;
    case 0x68: c->a = pop(c); setzn(c, c->a); break;
    case 0x08: push(c, (uint8_t)(c->p | CPU_FLAG_B | CPU_FLAG_U)); break;
    case 0x28: c->p = (uint8_t)((pop(c) & ~CPU_FLAG_B) | CPU_FLAG_U); break;
    case 0x4c: c->pc = addr(c, AM_ABS); break;
    case 0x6c: c->pc = addr(c, AM_IND); break;
    case 0x20: a = fetch16(c); push16(c, (uint16_t)(c->pc - 1)); c->pc = a; break;
    case 0x60: c->pc = (uint16_t)(pop16(c) + 1); break;
    case 0x40: c->p = (uint8_t)((pop(c) & ~CPU_FLAG_B) | CPU_FLAG_U); c->pc = pop16(c); break;
    case 0x10: branch(c, !getf(c, CPU_FLAG_N)); break;
    case 0x30: branch(c, getf(c, CPU_FLAG_N)); break;
    case 0x50: branch(c, !getf(c, CPU_FLAG_V)); break;
    case 0x70: branch(c, getf(c, CPU_FLAG_V)); break;
    case 0x90: branch(c, !getf(c, CPU_FLAG_C)); break;
    case 0xb0: branch(c, getf(c, CPU_FLAG_C)); break;
    case 0xd0: branch(c, !getf(c, CPU_FLAG_Z)); break;
    case 0xf0: branch(c, getf(c, CPU_FLAG_Z)); break;
    case 0x18: setf(c, CPU_FLAG_C, 0); break;
    case 0x38: setf(c, CPU_FLAG_C, 1); break;
    case 0x58: setf(c, CPU_FLAG_I, 0); break;
    case 0x78: setf(c, CPU_FLAG_I, 1); break;
    case 0xb8: setf(c, CPU_FLAG_V, 0); break;
    case 0xd8: setf(c, CPU_FLAG_D, 0); break;
    case 0xf8: setf(c, CPU_FLAG_D, 1); break;
    case 0x24: v = val(c, AM_ZP); setf(c, CPU_FLAG_Z, (c->a & v) == 0); setf(c, CPU_FLAG_N, v & 0x80); setf(c, CPU_FLAG_V, v & 0x40); break;
    case 0x2c: v = val(c, AM_ABS); setf(c, CPU_FLAG_Z, (c->a & v) == 0); setf(c, CPU_FLAG_N, v & 0x80); setf(c, CPU_FLAG_V, v & 0x40); break;
    default:
        /* Undefined opcodes are treated as NOPs so a stray byte cannot wedge the UI. */
        break;
    }
    return 1;
}

void cpu_execute(cpu_t *cpu, int instructions)
{
    while (instructions-- > 0 && cpu_step(cpu)) {
    }
}

void cpu_nmi(cpu_t *cpu) { interrupt(cpu, 0xfffa, 0); }
void cpu_irq(cpu_t *cpu) { if (!getf(cpu, CPU_FLAG_I)) interrupt(cpu, 0xfffe, 0); }
