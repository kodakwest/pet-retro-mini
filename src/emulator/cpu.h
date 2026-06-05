#ifndef PET_CPU_H
#define PET_CPU_H

#include <stdint.h>

#define CPU_FLAG_C 0x01
#define CPU_FLAG_Z 0x02
#define CPU_FLAG_I 0x04
#define CPU_FLAG_D 0x08
#define CPU_FLAG_B 0x10
#define CPU_FLAG_U 0x20
#define CPU_FLAG_V 0x40
#define CPU_FLAG_N 0x80

typedef struct cpu cpu_t;

typedef uint8_t (*cpu_read_fn)(void *user, uint16_t addr);
typedef void (*cpu_write_fn)(void *user, uint16_t addr, uint8_t value);

struct cpu {
    uint8_t a;
    uint8_t x;
    uint8_t y;
    uint8_t sp;
    uint8_t p;
    uint16_t pc;
    uint64_t cycles;
    cpu_read_fn read;
    cpu_write_fn write;
    void *user;
    int stopped;
};

void cpu_init(cpu_t *cpu, cpu_read_fn read_fn, cpu_write_fn write_fn, void *user);
void cpu_reset(cpu_t *cpu);
int cpu_step(cpu_t *cpu);
void cpu_execute(cpu_t *cpu, int instructions);
void cpu_nmi(cpu_t *cpu);
void cpu_irq(cpu_t *cpu);

#endif
