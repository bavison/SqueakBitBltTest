/*
 * Copyright © 2013 Raspberry Pi Foundation
 * Copyright © 2013 RISC OS Open Ltd
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of the copyright holders not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  The copyright holders make no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include <getopt.h>
#include <sys/time.h>

#include "BitBltDispatch.h"

#define sqInt int

extern sqInt initialiseModule(void);

#define CACHELINELEN (32)
#define L1CACHESIZE (16*1024)
#define L2CACHESIZE (128*1024)
#define KILOBYTE (1024)
#define MEGABYTE (1024*1024)

#define HALFL2CACHE (L2CACHESIZE/2 - 4*KILOBYTE)
#define TESTSIZE (40*MEGABYTE)

#define SCREENWIDTH (1920)
#define SCREENHEIGHT (1080)
#define TILEWIDTH (32)
#define TINYWIDTH (8)

static unsigned int  maskTable58[4] = { 0x7C00, 0x03E0, 0x001F, 0x0000 };
static          int shiftTable58[4] = {      9,      6,      3,      0 };
static unsigned int  maskTable85[4] = { 0xF80000, 0x00F800, 0x0000F8, 0x000000 };
static          int shiftTable85[4] = {       -9,       -6,       -3,        0 };
static uint32_t      lookupTable[32768];

static uint32_t src[SCREENHEIGHT][SCREENWIDTH];
static uint32_t dest[SCREENHEIGHT][SCREENWIDTH];

static operation_t op;

static const struct {
	const char *string;
	combination_rule_t number;
} crTable[] = {
	{ "clearWord",            CR_clearWord,            },
	{ "bitAnd",               CR_bitAnd,               },
	{ "bitAndInvert",         CR_bitAndInvert,         },
	{ "sourceWord",           CR_sourceWord,           },
	{ "bitInvertAnd",         CR_bitInvertAnd,         },
	{ "destinationWord",      CR_destinationWord,      },
	{ "bitXor",               CR_bitXor,               },
	{ "bitOr",                CR_bitOr,                },
	{ "bitInvertAndInvert",   CR_bitInvertAndInvert,   },
	{ "bitInvertXor",         CR_bitInvertXor,         },
	{ "bitInvertDestination", CR_bitInvertDestination, },
	{ "bitOrInvert",          CR_bitOrInvert,          },
	{ "bitInvertSource",      CR_bitInvertSource,      },
	{ "bitInvertOr",          CR_bitInvertOr,          },
	{ "bitInvertOrInvert",    CR_bitInvertOrInvert,    },
	{ "addWord",              CR_addWord,              },
	{ "subWord",              CR_subWord,              },
	{ "rgbAdd",               CR_rgbAdd,               },
	{ "rgbSub",               CR_rgbSub,               },
	{ "OLDrgbDiff",           CR_OLDrgbDiff,           },
	{ "OLDtallyIntoMap",      CR_OLDtallyIntoMap,      },
	{ "alphaBlend",           CR_alphaBlend,           },
	{ "pixPaint",             CR_pixPaint,             },
	{ "pixMask",              CR_pixMask,              },
	{ "rgbMax",               CR_rgbMax,               },
	{ "rgbMin",               CR_rgbMin,               },
	{ "rgbMinInvert",         CR_rgbMinInvert,         },
	{ "alphaBlendConst",      CR_alphaBlendConst,      },
	{ "alphaPaintConst",      CR_alphaPaintConst,      },
	{ "rgbDiff",              CR_rgbDiff,              },
	{ "tallyIntoMap",         CR_tallyIntoMap,         },
	{ "alphaBlendScaled",     CR_alphaBlendScaled,     },
	{ "rgbMul",               CR_rgbMul,               },
	{ "pixSwap",              CR_pixSwap,              },
	{ "pixClear",             CR_pixClear,             },
	{ "fixAlpha",             CR_fixAlpha,             },
	{ "rgbComponentAlpha",    CR_rgbComponentAlpha,    },
};

/* Just used for cancelling out the overheads */
static void control(size_t s_x, size_t s_y, size_t d_x, size_t d_y, size_t w, size_t h)
{
	(void) s_x;
	(void) s_y;
	(void) d_x;
	(void) d_y;
	(void) w;
	(void) h;
}

static void plot(size_t s_x, size_t s_y, size_t d_x, size_t d_y, size_t w, size_t h)
{
	op.src.x = s_x;
	op.src.y = s_y;
	op.dest.x = d_x;
	op.dest.y = d_y;
	op.width = w;
	op.height = h;

	copyBitsDispatch(&op);
}

static uint64_t gettime(void)
{
	struct timeval tv;

	gettimeofday (&tv, NULL);
	return tv.tv_sec * 1000000 + tv.tv_usec;
}

static uint32_t bench_L(void (*test)(), uint32_t log2Bpp, bool l2)
{
	uint32_t width = SCREENWIDTH - 64;
	uint32_t height = l2 ? (HALFL2CACHE / SCREENWIDTH) >> log2Bpp : 1;
	uint32_t times = TESTSIZE / ((width * height) << log2Bpp);
	uint32_t words = (SCREENWIDTH * height) >> (2 - log2Bpp);
	int i, j, x = 0, q = 0;
	volatile int qx;
	for (i = times; i >= 0; i--)
	{
		/* Ensure the destination is in cache (if it gets flushed out, source gets reloaded anyway) */
		for (j = 0; (unsigned) j < words; j += CACHELINELEN / sizeof **dest)
			q += dest[0][j];
		q += dest[0][words-1];

		x = (x + 1) & 63;
		test(x, 0, 63 - x, 0, width, height);
	}
	qx = q;
	(void) qx;
	return (width * height * times) << log2Bpp;
}

static uint32_t bench_M(void (*test)(), uint32_t log2Bpp)
{
	uint32_t width = SCREENWIDTH - 64;
	uint32_t height = SCREENHEIGHT;
	uint32_t times = TESTSIZE / ((width * height) << log2Bpp);
	int i, x = 0;
	for (i = times; i >= 0; i--)
	{
		x = (x + 1) & 63;
		test(x, 0, 63 - x, 0, width, height);
	}
	return (width * height * times) << log2Bpp;
}

void warning(const char *message)
{
    (void) message;
//    fprintf(stderr, "warning: %s\n", message);
}

int main(int argc, char *argv[])
{
	uint64_t t1, t2, t3;
	uint32_t byte_cnt;
	size_t iterations = 1;
	bool scalarHalftone = false;

	bool help = false;
	int opt;
	while ((opt = getopt(argc, argv, "hi:ns")) != -1) {
		switch (opt) {
		case 'h': help = true; break;
		case 'i': iterations = atoi(optarg); break;
		case 'n': op.noSource = true; break;
		case 's': scalarHalftone = true; break;
		}
	}
	if (help || optind == argc) {
bad_syntax:
		fprintf(stderr, "Syntax: %s [-h] [-i iterations] [-n] [-s] combinationRule [srcDepth] destDepth\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	size_t i;
	for (i = 0; i < sizeof crTable / sizeof *crTable; i++) {
		if (strcmp(argv[optind], crTable[i].string) == 0) {
			op.combinationRule = crTable[i].number;
			break;
		}
	}
	if (i == sizeof crTable / sizeof *crTable) {
		fprintf(stderr, "Unrecognised combinationRule\n");
		exit(EXIT_FAILURE);
	}
	if (op.combinationRule == CR_clearWord || op.combinationRule == CR_destinationWord || op.combinationRule == CR_bitInvertDestination)
		op.noSource = true;
	if ((op.noSource && optind != argc - 2) || (!op.noSource && optind != argc - 3))
		goto bad_syntax;
	op.src.depth = atoi(argv[optind + 1]);
	op.dest.depth = atoi(argv[argc - 1]);
	if (op.src.depth < 1 || op.src.depth > 32 || (op.src.depth & (op.src.depth-1)) != 0 || op.dest.depth < 1 || op.dest.depth > 32 || (op.dest.depth & (op.dest.depth-1)) != 0) {
		fprintf(stderr, "Bad colour depth\n");
		exit(EXIT_FAILURE);
	}

	op.src.bits = src;
	op.src.pitch = (SCREENWIDTH * op.src.depth / 8 + 3) &~ 3;
	op.src.msb = 1;
	op.dest.bits = dest;
	op.dest.pitch = (SCREENWIDTH * op.dest.depth / 8 + 3) &~ 3;
	op.dest.msb = 1;
	if (op.dest.depth == op.src.depth) {
		op.cmFlags = 0;
		op.cmMaskTable = NULL;
		op.cmShiftTable = NULL;
		op.cmMask = 0;
		op.cmLookupTable = NULL;
	} else if (op.dest.depth == 32 && op.src.depth == 16) {
		op.cmFlags = ColorMapPresent | ColorMapFixedPart;
		op.cmMaskTable = &maskTable58;
		op.cmShiftTable = &shiftTable58;
		op.cmMask = 0;
		op.cmLookupTable = NULL;
	} else if (op.dest.depth == 16 && op.src.depth == 32) {
		op.cmFlags = ColorMapPresent | ColorMapFixedPart;
		op.cmMaskTable = &maskTable85;
		op.cmShiftTable = &shiftTable85;
		op.cmMask = 0;
		op.cmLookupTable = NULL;
	} else if (op.src.depth == 32) {
		op.cmFlags = ColorMapPresent | ColorMapFixedPart | ColorMapIndexedPart;
		op.cmMaskTable = &maskTable85;
		op.cmShiftTable = &shiftTable85;
		op.cmMask = 0x7FFF;
		op.cmLookupTable = &lookupTable;
	} else {
		op.cmFlags = ColorMapPresent | ColorMapIndexedPart;
		op.cmMaskTable = NULL;
		op.cmShiftTable = NULL;
		op.cmMask = op.src.depth == 16 ? 0x7FFF : (1u << op.src.depth) - 1;
		op.cmLookupTable = &lookupTable;
	}
	if (scalarHalftone) {
		uint32_t oneWord[1] = { 0x55555555 };
		op.noHalftone = 0;
		op.halftoneHeight = 1;
		op.halftoneBase = &oneWord;
	} else {
		op.noHalftone = 1;
		op.halftoneHeight = 0;
		op.halftoneBase = NULL;
	}

	uint32_t log2destBpp = 0;
	switch (op.dest.depth) {
	case 8:  log2destBpp = 0; break;
	case 16: log2destBpp = 1; break;
	case 32: log2destBpp = 2; break;
	/* TODO: what about smaller than that? */
	}

	initialiseCopyBits();
	initialiseModule();

	memset(src, 0x5A, sizeof src);
	memset(dest, 0xA5, sizeof dest);

	printf("L1,     L2,     M\n");

	while (iterations--)
	{
		memcpy(dest, src, sizeof dest);

		t1 = gettime();
		bench_L(control, log2destBpp, false);
		t2 = gettime();
		byte_cnt = bench_L(plot, log2destBpp, false);
		t3 = gettime();
		printf("%6.2f, ", ((double)byte_cnt) / ((t3 - t2) - (t2 - t1)));
		fflush(stdout);

		memcpy(dest, src, sizeof dest);

		t1 = gettime();
		bench_L(control, log2destBpp, true);
		t2 = gettime();
		byte_cnt = bench_L(plot, log2destBpp, true);
		t3 = gettime();
		printf("%6.2f, ", ((double)byte_cnt) / ((t3 - t2) - (t2 - t1)));
		fflush(stdout);

		memcpy(dest, src, sizeof dest);

		t1 = gettime();
		bench_M(control, log2destBpp);
		t2 = gettime();
		byte_cnt = bench_M(plot, log2destBpp);
		t3 = gettime();
		printf("%6.2f\n", ((double)byte_cnt) / ((t3 - t2) - (t2 - t1)));
		fflush(stdout);
	}
	exit(EXIT_SUCCESS);
}
