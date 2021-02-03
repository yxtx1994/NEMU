#ifndef __COMMON_H__
#define __COMMON_H__

#include <generated/autoconf.h>

/* You will define this macro in PA2 */
#ifdef __ICS_EXPORT
//#define HAS_IOE
#else
#define HAS_IOE
#endif

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>

#include <macro.h>

typedef MUXDEF(CONFIG_ISA64, uint64_t, uint32_t) word_t;
typedef MUXDEF(CONFIG_ISA64, int64_t, int32_t)  sword_t;
#define FMT_WORD MUXDEF(CONFIG_ISA64, "0x%016lx", "0x%08x")

typedef word_t rtlreg_t;
typedef word_t vaddr_t;
typedef uint32_t paddr_t;
typedef uint16_t ioaddr_t;

#include <debug.h>

#endif
