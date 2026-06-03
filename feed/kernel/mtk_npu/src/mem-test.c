// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2025 MediaTek Inc. All Rights Reserved.
 *
 * Author: Alvin Kuo <alvin.kuo@mediatek.com>
 */

#include <linux/bits.h>
#include <linux/errno.h>
#include <linux/io.h>
#include <linux/random.h>

#include "npu/internal.h"
#include "npu/mcu.h"
#include "npu/mem-test.h"

#define ONE				(0x00000001)
#define UL_ONEBITS			(0xffffffff)
#define UL_LEN				(32)
#define CHECKERBOARD1			(0x55555555)
#define CHECKERBOARD2			(0xaaaaaaaa)
#define UL_BYTE(x)			((x | x << 8 | x << 16 | x << 24))

#define MEMTESTER_BYPASS_MASK		(GENMASK(14, 1))

#define NPU_MEM(__mname, __maddr, __mlen)				\
	{								\
		.name = __mname,					\
		.addr = __maddr,					\
		.len = __mlen,						\
	}

struct npu_memory {
	char *name;
	u32 addr;
	size_t len;
};

static const struct npu_memory mtests[] = {
	NPU_MEM("top-l2sram-com", TOP_L2SRAM_COM, L2SRAM_COM_LEN),
	NPU_MEM("top-core-m-dtcm", TOP_CORE_M_DTCM, CORE_M_XTCM_LEN),
	NPU_MEM("top-core-m-itcm", TOP_CORE_M_ITCM, CORE_M_XTCM_LEN),
	NPU_MEM("clust-l2sram-pkt", CLUST_L2SRAM_PKT, L2SRAM_PKT_LEN),
	NPU_MEM("clust-l2sram-com", CLUST_L2SRAM_COM, L2SRAM_COM_LEN),
	NPU_MEM("clust-core-0-itcm", CLUST_CORE_X_ITCM(0), CORE_X_XTCM_LEN),
	NPU_MEM("clust-core-0-dtcm", CLUST_CORE_X_DTCM(0), CORE_X_XTCM_LEN),
	NPU_MEM("clust-core-1-itcm", CLUST_CORE_X_ITCM(1), CORE_X_XTCM_LEN),
	NPU_MEM("clust-core-1-dtcm", CLUST_CORE_X_DTCM(1), CORE_X_XTCM_LEN),
	NPU_MEM("clust-core-2-itcm", CLUST_CORE_X_ITCM(2), CORE_X_XTCM_LEN),
	NPU_MEM("clust-core-2-dtcm", CLUST_CORE_X_DTCM(2), CORE_X_XTCM_LEN),
	NPU_MEM("clust-core-3-itcm", CLUST_CORE_X_ITCM(3), CORE_X_XTCM_LEN),
	NPU_MEM("clust-core-3-dtcm", CLUST_CORE_X_DTCM(3), CORE_X_XTCM_LEN),
};

static inline void mtk_npu_mem_write(u32 reg, u32 val)
{
	writel(val, npu.base + reg);
}

static inline u32 mtk_npu_mem_read(u32 reg)
{
	return readl(npu.base + reg);
}

/* porting from memtester */
static u32 compare_regions(u32 bufa, u32 bufb, size_t count)
{
	u32 p1 = bufa;
	u32 p2 = bufb;
	u32 errs = 0;
	size_t i;

	for (i = 0; i < count; i++, p1 += sizeof(u32), p2 += sizeof(u32)) {
		if (mtk_npu_mem_read(p1) != mtk_npu_mem_read(p2)) {
			NPU_ERR("FAILURE: 0x%08x (@0x%08x) != 0x%080x (@%08x)\n",
				 mtk_npu_mem_read(p1), p1, mtk_npu_mem_read(p2), p2);
			errs++;
		}
	}

	return errs;
}

static u32 test_random_value(u32 bufa, u32 bufb, size_t count)
{
	u32 p1 = bufa;
	u32 p2 = bufb;
	u32 pattern;
	size_t i;

	for (i = 0; i < count; i++, p1 += sizeof(u32), p2 += sizeof(u32)) {
		get_random_bytes(&pattern, sizeof(pattern));

		mtk_npu_mem_write(p1, pattern);
		mtk_npu_mem_write(p2, pattern);
	}

	return compare_regions(bufa, bufb, count);
}

static u32 test_xor_comparison(u32 bufa, u32 bufb, size_t count)
{
	u32 p1 = bufa;
	u32 p2 = bufb;
	u32 pattern;
	size_t i;

	get_random_bytes(&pattern, sizeof(pattern));

	for (i = 0; i < count; i++, p1 += sizeof(u32), p2 += sizeof(u32)) {
		mtk_npu_mem_write(p1, mtk_npu_mem_read(p1) ^ pattern);
		mtk_npu_mem_write(p2, mtk_npu_mem_read(p2) ^ pattern);
	}

	return compare_regions(bufa, bufb, count);
}

static u32 test_sub_comparison(u32 bufa, u32 bufb, size_t count)
{
	u32 p1 = bufa;
	u32 p2 = bufb;
	u32 pattern;
	size_t i;

	get_random_bytes(&pattern, sizeof(pattern));

	for (i = 0; i < count; i++, p1 += sizeof(u32), p2 += sizeof(u32)) {
		mtk_npu_mem_write(p1, mtk_npu_mem_read(p1) - pattern);
		mtk_npu_mem_write(p2, mtk_npu_mem_read(p2) - pattern);
	}

	return compare_regions(bufa, bufb, count);
}

static u32 test_mul_comparison(u32 bufa, u32 bufb, size_t count)
{
	u32 p1 = bufa;
	u32 p2 = bufb;
	u32 pattern;
	size_t i;

	get_random_bytes(&pattern, sizeof(pattern));

	for (i = 0; i < count; i++, p1 += sizeof(u32), p2 += sizeof(u32)) {
		mtk_npu_mem_write(p1, mtk_npu_mem_read(p1) * pattern);
		mtk_npu_mem_write(p2, mtk_npu_mem_read(p2) * pattern);
	}

	return compare_regions(bufa, bufb, count);
}

static u32 test_div_comparison(u32 bufa, u32 bufb, size_t count)
{
	u32 p1 = bufa;
	u32 p2 = bufb;
	u32 pattern;
	size_t i;

	get_random_bytes(&pattern, sizeof(pattern));

	for (i = 0; i < count; i++, p1 += sizeof(u32), p2 += sizeof(u32)) {
		if (!pattern)
			pattern++;

		mtk_npu_mem_write(p1, mtk_npu_mem_read(p1) / pattern);
		mtk_npu_mem_write(p2, mtk_npu_mem_read(p2) / pattern);
	}

	return compare_regions(bufa, bufb, count);
}

static u32 test_or_comparison(u32 bufa, u32 bufb, size_t count)
{
	u32 p1 = bufa;
	u32 p2 = bufb;
	u32 pattern;
	size_t i;

	get_random_bytes(&pattern, sizeof(pattern));

	for (i = 0; i < count; i++, p1 += sizeof(u32), p2 += sizeof(u32)) {
		mtk_npu_mem_write(p1, mtk_npu_mem_read(p1) | pattern);
		mtk_npu_mem_write(p2, mtk_npu_mem_read(p2) | pattern);
	}

	return compare_regions(bufa, bufb, count);
}

static u32 test_and_comparison(u32 bufa, u32 bufb, size_t count)
{
	u32 p1 = bufa;
	u32 p2 = bufb;
	u32 pattern;
	size_t i;

	get_random_bytes(&pattern, sizeof(pattern));

	for (i = 0; i < count; i++, p1 += sizeof(u32), p2 += sizeof(u32)) {
		mtk_npu_mem_write(p1, mtk_npu_mem_read(p1) & pattern);
		mtk_npu_mem_write(p2, mtk_npu_mem_read(p2) & pattern);
	}

	return compare_regions(bufa, bufb, count);
}

static u32 test_seqinc_comparison(u32 bufa, u32 bufb, size_t count)
{
	u32 p1 = bufa;
	u32 p2 = bufb;
	u32 pattern;
	size_t i;

	get_random_bytes(&pattern, sizeof(pattern));

	for (i = 0; i < count; i++, p1 += sizeof(u32), p2 += sizeof(u32)) {
		mtk_npu_mem_write(p1, pattern + i);
		mtk_npu_mem_write(p2, pattern + i);
	}

	return compare_regions(bufa, bufb, count);
}

static u32 test_solidbits_comparison(u32 bufa, u32 bufb, size_t count)
{
	u32 p1 = bufa;
	u32 p2 = bufb;
	u32 __pattern;
	u32 pattern;
	size_t i;
	u32 errs;
	u32 j;

	for (j = 0; j < 64; j++) {
		pattern = (j % 2) == 0 ? UL_ONEBITS : 0;
		p1 = bufa;
		p2 = bufb;

		for (i = 0; i < count; i++, p1 += sizeof(u32), p2 += sizeof(u32)) {
			__pattern = (i % 2) == 0 ? pattern : ~pattern;
			mtk_npu_mem_write(p1, __pattern);
			mtk_npu_mem_write(p2, __pattern);
		}

		errs = compare_regions(bufa, bufb, count);
		if (errs)
			return errs;
	}

	return 0;
}

static u32 test_checkerboard_comparison(u32 bufa, u32 bufb, size_t count)
{
	u32 p1 = bufa;
	u32 p2 = bufb;
	u32 __pattern;
	u32 pattern;
	size_t i;
	u32 errs;
	u32 j;

	for (j = 0; j < 64; j++) {
		pattern = (j % 2) == 0 ? CHECKERBOARD1 : CHECKERBOARD2;
		p1 = bufa;
		p2 = bufb;

		for (i = 0; i < count; i++, p1 += sizeof(u32), p2 += sizeof(u32)) {
			__pattern = (i % 2) == 0 ? pattern : ~pattern;
			mtk_npu_mem_write(p1, __pattern);
			mtk_npu_mem_write(p2, __pattern);
		}

		errs = compare_regions(bufa, bufb, count);
		if (errs)
			return errs;
	}

	return 0;
}

static u32 test_blockseq_comparison(u32 bufa, u32 bufb, size_t count)
{
	u32 p1 = bufa;
	u32 p2 = bufb;
	u32 pattern;
	size_t i;
	u32 errs;
	u32 j;

	for (j = 0; j < 256; j++) {
		p1 = bufa;
		p2 = bufb;

		for (i = 0; i < count; i++, p1 += sizeof(u32), p2 += sizeof(u32)) {
			pattern = UL_BYTE(j);
			mtk_npu_mem_write(p1, pattern);
			mtk_npu_mem_write(p2, pattern);
		}

		errs = compare_regions(bufa, bufb, count);
		if (errs)
			return errs;
	}

	return 0;
}

static u32 test_walkbits0_comparison(u32 bufa, u32 bufb, size_t count)
{
	u32 p1 = bufa;
	u32 p2 = bufb;
	u32 pattern;
	size_t i;
	u32 errs;
	u32 j;

	for (j = 0; j < UL_LEN * 2; j++) {
		p1 = bufa;
		p2 = bufb;

		for (i = 0; i < count; i++, p1 += sizeof(u32), p2 += sizeof(u32)) {
			if (j < UL_LEN) {
				/* Walk it up. */
				pattern = ONE << j;
				mtk_npu_mem_write(p1, pattern);
				mtk_npu_mem_write(p2, pattern);
			} else {
				/* Walk it back down. */
				pattern = ONE << (UL_LEN * 2 - j - 1);
				mtk_npu_mem_write(p1, pattern);
				mtk_npu_mem_write(p2, pattern);
			}
		}

		errs = compare_regions(bufa, bufb, count);
		if (errs)
			return errs;
	}

	return 0;
}

static u32 test_walkbits1_comparison(u32 bufa, u32 bufb, size_t count)
{
	u32 p1 = bufa;
	u32 p2 = bufb;
	u32 pattern;
	size_t i;
	u32 errs;
	u32 j;

	for (j = 0; j < UL_LEN * 2; j++) {
		p1 = bufa;
		p2 = bufb;

		for (i = 0; i < count; i++, p1 += sizeof(u32), p2 += sizeof(u32)) {
			if (j < UL_LEN) {
				/* Walk it up. */
				pattern = UL_ONEBITS ^ (ONE << j);
				mtk_npu_mem_write(p1, pattern);
				mtk_npu_mem_write(p2, pattern);
			} else {
				/* Walk it back down. */
				pattern = UL_ONEBITS ^ (ONE << (UL_LEN * 2 - j - 1));
				mtk_npu_mem_write(p1, pattern);
				mtk_npu_mem_write(p2, pattern);
			}
		}

		errs = compare_regions(bufa, bufb, count);
		if (errs)
			return errs;
	}

	return 0;
}

static u32 test_bitspread_comparison(u32 bufa, u32 bufb, size_t count)
{
	u32 p1 = bufa;
	u32 p2 = bufb;
	u32 pattern;
	size_t i;
	u32 errs;
	u32 j;

	for (j = 0; j < UL_LEN * 2; j++) {
		p1 = bufa;
		p2 = bufb;

		for (i = 0; i < count; i++, p1 += sizeof(u32), p2 += sizeof(u32)) {
			if (j < UL_LEN) {
				/* Walk it up. */
				pattern = i % 2 == 0 ?
					(ONE << j) | (ONE << (j + 2)) :
					UL_ONEBITS ^ ((ONE << j) | (ONE << (j + 2)));
				mtk_npu_mem_write(p1, pattern);
				mtk_npu_mem_write(p2, pattern);
			} else {
				/* Walk it back down. */
				pattern = i % 2 == 0 ?
					((ONE << (UL_LEN * 2 - 1 - j)) |
					 (ONE << (UL_LEN * 2 + 1 - j))) :
					(UL_ONEBITS ^ (ONE << (UL_LEN * 2 - 1 - j) |
					 (ONE << (UL_LEN * 2 + 1 - j))));
				mtk_npu_mem_write(p1, pattern);
				mtk_npu_mem_write(p2, pattern);
			}
		}

		errs = compare_regions(bufa, bufb, count);
		if (errs)
			return errs;
	}

	return 0;
}

static u32 test_bitflip_comparison(u32 bufa, u32 bufb, size_t count)
{
	u32 p1 = bufa;
	u32 p2 = bufb;
	u32 pattern;
	size_t i;
	u32 errs;
	u32 j;
	u32 k;

	for (k = 0; k < UL_LEN; k++) {
		pattern = ONE << k;
		for (j = 0; j < 8; j++) {
			pattern = ~pattern;
			p1 = bufa;
			p2 = bufb;

			for (i = 0; i < count; i++, p1 += sizeof(u32), p2 += sizeof(u32)) {
				pattern = i % 2 == 0 ? pattern : ~pattern;
				mtk_npu_mem_write(p1, pattern);
				mtk_npu_mem_write(p2, pattern);
			}

			errs = compare_regions(bufa, bufb, count);
			if (errs)
				return errs;
		}
	}

	return 0;
}

struct test {
	char *name;
	u32 (*fp)(u32 bufa, u32 bufb, size_t count);
};

static const struct test tests[] = {
	{ "Random Value", test_random_value },
	{ "Compare XOR", test_xor_comparison },
	{ "Compare SUB", test_sub_comparison },
	{ "Compare MUL", test_mul_comparison },
	{ "Compare DIV", test_div_comparison },
	{ "Compare OR", test_or_comparison },
	{ "Compare AND", test_and_comparison },
	{ "Sequential Increment", test_seqinc_comparison },
	{ "Solid Bits", test_solidbits_comparison },
	{ "Block Sequential", test_blockseq_comparison },
	{ "Checkerboard", test_checkerboard_comparison },
	{ "Bit Spread", test_bitspread_comparison },
	{ "Bit Flip", test_bitflip_comparison },
	{ "Walking Ones", test_walkbits1_comparison },
	{ "Walking Zeroes", test_walkbits0_comparison },
};

/* porting from U-Boot mem_test_alt */
static u32 mem_test_alt(u32 start_addr, u32 end_addr)
{
	u32 addr;
	u32 dummy;
	u32 val;
	u32 readback;
	u32 temp;
	u32 j;
	u32 offset;
	u32 test_offset;
	u32 pattern;
	u32 anti_pattern;
	u32 errs = 0;
	u32 num_words;
	static const u32 bitpattern[] = {
		0x00000001,	/* single bit */
		0x00000003,	/* two adjacent bits */
		0x00000007,	/* three adjacent bits */
		0x0000000F,	/* four adjacent bits */
		0x00000005,	/* two non-adjacent bits */
		0x00000015,	/* three non-adjacent bits */
		0x00000055,	/* four non-adjacent bits */
		0xaaaaaaaa,	/* alternating 1/0 */
	};

	num_words = (end_addr - start_addr) / sizeof(u32);
	if (num_words < 2) {
		NPU_ERR("mem test region at least 8 bytes or greater\n");
		return 0;
	}

	/*
	 * Data line test: write a pattern to the first
	 * location, write the 1's complement to a 'parking'
	 * address (changes the state of the data bus so a
	 * floating bus doesn't give a false OK), and then
	 * read the value back. Note that we read it back
	 * into a variable because the next time we read it,
	 * it might be right (been there, tough to explain to
	 * the quality guys why it prints a failure when the
	 * "is" and "should be" are obviously the same in the
	 * error message).
	 *
	 * Rather than exhaustively testing, we test some
	 * patterns by shifting '1' bits through a field of
	 * '0's and '0' bits through a field of '1's (i.e.
	 * pattern and ~pattern).
	 */
	addr = start_addr;
	dummy = start_addr + sizeof(u32);
	for (j = 0; j < ARRAY_SIZE(bitpattern); j++) {
		val = bitpattern[j];
		for (; val != 0; val <<= 1) {
			mtk_npu_mem_write(addr, val);
			mtk_npu_mem_write(dummy, ~val); /* clear the test data off the bus */
			readback = mtk_npu_mem_read(addr);
			if (readback != val) {
				NPU_ERR("FAILURE (data line): expected %08x, actual %08x\n",
					 val, readback);
				errs++;
			}

			mtk_npu_mem_write(addr, ~val);
			mtk_npu_mem_write(dummy, val);
			readback = mtk_npu_mem_read(addr);
			if (readback != ~val) {
				NPU_ERR("FAILURE (data line): is %08x, should be %08x\n",
					readback, ~val);
				errs++;
			}
		}
	}

	/*
	 * Based on code whose Original Author and Copyright
	 * information follows: Copyright (c) 1998 by Michael
	 * Barr. This software is placed into the public
	 * domain and may be used for any purpose. However,
	 * this notice must not be changed or removed and no
	 * warranty is either expressed or implied by its
	 * publication or distribution.
	 */

	/*
	 * Address line test

	 * Description: Test the address bus wiring in a
	 *              memory region by performing a walking
	 *              1's test on the relevant bits of the
	 *              address and checking for aliasing.
	 *              This test will find single-bit
	 *              address failures such as stuck-high,
	 *              stuck-low, and shorted pins. The base
	 *              address and size of the region are
	 *              selected by the caller.

	 * Notes:	For best results, the selected base
	 *              address should have enough LSB 0's to
	 *              guarantee single address bit changes.
	 *              For example, to test a 64-Kbyte
	 *              region, select a base address on a
	 *              64-Kbyte boundary. Also, select the
	 *              region size as a power-of-two if at
	 *              all possible.
	 *
	 * Returns:     0 if the test succeeds, 1 if the test fails.
	 */
	pattern = (u32)0xaaaaaaaa;
	anti_pattern = (u32)0x55555555;

	/*
	 * Write the default pattern at each of the
	 * power-of-two offsets.
	 */
	for (offset = 1; offset < num_words; offset <<= 1)
		mtk_npu_mem_write(addr + offset * sizeof(u32), pattern);

	/*
	 * Check for address bits stuck high.
	 */
	test_offset = 0;
	mtk_npu_mem_write(addr + test_offset * sizeof(u32), anti_pattern);

	for (offset = 1; offset < num_words; offset <<= 1) {
		temp = mtk_npu_mem_read(addr + offset * sizeof(u32));
		if (temp != pattern) {
			NPU_ERR("FAILURE: Address bit stuck high @ 0x%08x: expected 0x%08x, actual 0x%08x\n",
				 (u32)(addr + offset * sizeof(u32)),
				 pattern, temp);
			errs++;
		}
	}
	mtk_npu_mem_write(addr + test_offset * sizeof(u32), pattern);

	/*
	 * Check for addr bits stuck low or shorted.
	 */
	for (test_offset = 1; test_offset < num_words; test_offset <<= 1) {
		mtk_npu_mem_write(addr + test_offset * sizeof(u32), anti_pattern);

		for (offset = 1; offset < num_words; offset <<= 1) {
			temp = mtk_npu_mem_read(addr + offset * sizeof(u32));
			if ((temp != pattern) && (offset != test_offset)) {
				NPU_ERR("FAILURE: Address bit stuck low or shorted @ 0x%08x: expected 0x%08x, actual 0x%08x\n",
					(u32)(addr + offset * sizeof(u32)),
					pattern, temp);
				errs++;
			}
		}
		mtk_npu_mem_write(addr + test_offset * sizeof(u32), pattern);
	}

	/*
	 * Description: Test the integrity of a physical
	 *		memory device by performing an
	 *		increment/decrement test over the
	 *		entire region. In the process every
	 *		storage bit in the device is tested
	 *		as a zero and a one. The base address
	 *		and the size of the region are
	 *		selected by the caller.
	 *
	 * Returns:     0 if the test succeeds, 1 if the test fails.
	 */
	/*num_words++;*/

	/*
	 * Fill memory with a known pattern.
	 */
	for (pattern = 1, offset = 0; offset < num_words; pattern++, offset++)
		mtk_npu_mem_write(addr + offset * sizeof(u32), pattern);

	/*
	 * Check each location and invert it for the second pass.
	 */
	for (pattern = 1, offset = 0; offset < num_words; pattern++, offset++) {
		temp = mtk_npu_mem_read(addr + offset * sizeof(u32));
		if (temp != pattern) {
			NPU_ERR("FAILURE (read/write) @ 0x%08x: expected 0x%08x, actual 0x%08x)\n",
				(u32)(addr + offset * sizeof(u32)),
				pattern, temp);
			errs++;
		}

		anti_pattern = ~pattern;
		mtk_npu_mem_write(addr + offset * sizeof(u32), anti_pattern);
	}

	/*
	 * Check each location for the inverted pattern and zero it.
	 */
	for (pattern = 1, offset = 0; offset < num_words; pattern++, offset++) {
		anti_pattern = ~pattern;
		temp = mtk_npu_mem_read(addr + offset * sizeof(u32));
		if (temp != anti_pattern) {
			NPU_ERR("FAILURE (read/write): @ 0x=%08x: expected 0x%08x, actual 0x%08x)\n",
				(u32)(addr + offset * sizeof(u32)),
				anti_pattern, temp);
			errs++;
		}
		mtk_npu_mem_write(addr + offset * sizeof(u32), 0);
	}

	return errs;
}

static int mtk_npu_mem_tester(u32 start_addr, u32 end_addr)
{
	size_t half_len;
	int ret = 0;
	u32 addr;
	u32 errs;
	u32 idx;

	errs = mem_test_alt(start_addr, end_addr);
	if (errs) {
		NPU_ERR("complete alternative mem test failed for %u time(s)\n", errs);
		ret = -EFAULT;
		goto out;
	}

	half_len = (end_addr - start_addr) / 2;
	for (idx = 0; idx < ARRAY_SIZE(tests); idx++) {
		if (BIT(idx) & MEMTESTER_BYPASS_MASK)
			continue;

		errs = tests[idx].fp(start_addr, start_addr + half_len, half_len / sizeof(u32));
		if (errs) {
			NPU_ERR("%s mem test failed for %u time(s)\n", tests[idx].name, errs);
			ret = -EFAULT;
			goto out;
		}
	}

	for (addr = start_addr; addr < end_addr; addr += sizeof(u32))
		mtk_npu_mem_write(addr, 0);

out:
	return ret;
}

int mtk_npu_mem_test(void)
{
	u32 start;
	u32 end;
	u32 idx;
	int ret;

	for (idx = 0; idx < ARRAY_SIZE(mtests); idx++) {
		start = mtests[idx].addr;
		end = start + mtests[idx].len;

		ret = mtk_npu_mem_tester(start, end);
		if (ret) {
			NPU_ERR("mem test fail on %s\n", mtests[idx].name);
			break;
		}
	}

	return ret;
}
