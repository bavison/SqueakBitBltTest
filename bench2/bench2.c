/*
 * Copyright © 2014 Raspberry Pi FouAndation
 * Copyright © 2014 RISC OS Open Ltd
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

static uint32_t srcA[SCREENHEIGHT][SCREENWIDTH];
static uint32_t srcB[SCREENHEIGHT][SCREENWIDTH];
static uint32_t elsewhere[SCREENHEIGHT][SCREENWIDTH];

static compare_operation_t op;

static const struct {
	const char *string;
	match_rule_t number;
	uint32_t colorA;
	uint32_t colorB;
} mrTable[] = {
    /* The colours here are chosen so that we never time the early-exit test case */
	{ "pixelMatch", MR_pixelMatch, 0xFFFFFFFF, 0xFFFFFFFF },
	{ "notAnotB",   MR_notAnotB,   0x00000000, 0x00000000 },
	{ "notAmatchB", MR_notAmatchB, 0x00000000, 0xFFFFFFFF },
};

/* Just used for cancelling out the overheads */
static void control(size_t A_x, size_t A_y, size_t B_x, size_t B_y, size_t w, size_t h)
{
	(void) A_x;
	(void) A_y;
	(void) B_x;
	(void) B_y;
	(void) w;
	(void) h;
}

static void compare(size_t A_x, size_t A_y, size_t B_x, size_t B_y, size_t w, size_t h)
{
	op.srcA.x = A_x;
	op.srcA.y = A_y;
	op.srcB.x = B_x;
	op.srcB.y = B_y;
	op.width = w;
	op.height = h;

	(void) compareColorsDispatch(&op);
}

static uint64_t gettime(void)
{
	struct timeval tv;

	gettimeofday (&tv, NULL);
	return tv.tv_sec * 1000000 + tv.tv_usec;
}

static uint32_t bench_L(void (*test)(), uint32_t log2BppA, uint32_t log2BppB, bool l2)
{
    uint32_t combinedBpp = (1 << log2BppA) + (1 << log2BppB);
	uint32_t width = l2 ? SCREENWIDTH - 64 : SCREENWIDTH / 2;
	uint32_t height = l2 ? HALFL2CACHE / (SCREENWIDTH * combinedBpp) : 1;
	uint32_t times = TESTSIZE / (width * height * combinedBpp);
    uint32_t wordsA = (l2 ? (SCREENWIDTH * height) : width) >> (2 - log2BppA);
    uint32_t wordsB = (l2 ? (SCREENWIDTH * height) : width) >> (2 - log2BppB);
	int i, j, x = 0, q = 0;
	volatile int qx;
	for (i = times; i >= 0; i--)
	{
		/* Ensure the buffers are in the L1/L2 cache as appropriate */
		for (j = 0; (unsigned) j < wordsA; j += CACHELINELEN / sizeof **srcA)
			q += srcA[0][j];
		q += srcA[0][wordsA-1];
        for (j = 0; (unsigned) j < wordsB; j += CACHELINELEN / sizeof **srcB)
            q += srcB[0][j];
        q += srcB[0][wordsB-1];

		x = (x + 1) & 63;
		test(x, 0, 63 - x, 0, width, height);
	}
	qx = q;
	(void) qx;
	return width * height * times * combinedBpp;
}

static uint32_t bench_M(void (*test)(), uint32_t log2BppA, uint32_t log2BppB)
{
    uint32_t combinedBpp = (1 << log2BppA) + (1 << log2BppB);
	uint32_t width = SCREENWIDTH - 64;
	uint32_t height = SCREENHEIGHT;
	uint32_t times = TESTSIZE / (width * height * combinedBpp);
	int i, x = 0;
	for (i = times; i >= 0; i--)
	{
		x = (x + 1) & 63;
		test(x, 0, 63 - x, 0, width, height);
	}
	return width * height * times * combinedBpp;
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

	op.tally = 1;

	bool help = false;
	int opt;
	while ((opt = getopt(argc, argv, "hi:t:")) != -1) {
		switch (opt) {
		case 'h': help = true; break;
		case 'i': iterations = atoi(optarg); break;
        case 't': op.tally = atoi(optarg); break;
		}
	}
	if (help || optind != argc - 3) {
		fprintf(stderr, "Syntax: %s [-h] [-i iterations] [-t tallyFlag] matchRule depthA depthB\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	size_t i;
	for (i = 0; i < sizeof mrTable / sizeof *mrTable; i++) {
		if (strcmp(argv[optind], mrTable[i].string) == 0) {
			op.matchRule = mrTable[i].number;
            op.colorA = mrTable[i].colorA;
            op.colorB = mrTable[i].colorB;
			break;
		}
	}
	if (i == sizeof mrTable / sizeof *mrTable) {
		fprintf(stderr, "Unrecognised matchRule\n");
		exit(EXIT_FAILURE);
	}

	op.srcA.depth = atoi(argv[optind + 1]);
	op.srcB.depth = atoi(argv[optind + 2]);
	if (op.srcA.depth < 1 || op.srcA.depth > 32 || (op.srcA.depth & (op.srcA.depth-1)) != 0 || op.srcB.depth < 1 || op.srcB.depth > 32 || (op.srcB.depth & (op.srcB.depth-1)) != 0) {
		fprintf(stderr, "Bad colour depth\n");
		exit(EXIT_FAILURE);
	}

	op.srcA.bits = srcA;
	op.srcA.pitch = (SCREENWIDTH * op.srcA.depth / 8 + 3) &~ 3;
	op.srcA.msb = 1;
	op.srcB.bits = srcB;
	op.srcB.pitch = (SCREENWIDTH * op.srcB.depth / 8 + 3) &~ 3;
	op.srcB.msb = 1;

	uint32_t log2srcABpp = 0;
	switch (op.srcA.depth) {
	case 8:  log2srcABpp = 0; break;
	case 16: log2srcABpp = 1; break;
	case 32: log2srcABpp = 2; break;
	/* TODO: what about smaller than that? */
	}

    uint32_t log2srcBBpp = 0;
    switch (op.srcB.depth) {
    case 8:  log2srcBBpp = 0; break;
    case 16: log2srcBBpp = 1; break;
    case 32: log2srcBBpp = 2; break;
    /* TODO: what about smaller than that? */
    }

	initialiseCopyBits();
	initialiseModule();

	memset(srcA, 0, sizeof srcA);
	memset(srcB, 0, sizeof srcB);
    memset(elsewhere, 0, sizeof elsewhere);

	printf("L1,     L2,     M\n");

	while (iterations--)
	{
        memcpy(srcA, elsewhere, sizeof elsewhere);

		t1 = gettime();
		bench_L(control, log2srcABpp, log2srcBBpp, false);
		t2 = gettime();
		byte_cnt = bench_L(compare, log2srcABpp, log2srcBBpp, false);
		t3 = gettime();
		printf("%6.2f, ", ((double)byte_cnt) / ((t3 - t2) - (t2 - t1)));
		fflush(stdout);

        memcpy(srcA, elsewhere, sizeof elsewhere);

		t1 = gettime();
		bench_L(control, log2srcABpp, log2srcBBpp, true);
		t2 = gettime();
		byte_cnt = bench_L(compare, log2srcABpp, log2srcBBpp, true);
		t3 = gettime();
		printf("%6.2f, ", ((double)byte_cnt) / ((t3 - t2) - (t2 - t1)));
		fflush(stdout);

        memcpy(srcA, elsewhere, sizeof elsewhere);

		t1 = gettime();
		bench_M(control, log2srcABpp, log2srcBBpp);
		t2 = gettime();
		byte_cnt = bench_M(compare, log2srcABpp, log2srcBBpp);
		t3 = gettime();
		printf("%6.2f\n", ((double)byte_cnt) / ((t3 - t2) - (t2 - t1)));
		fflush(stdout);
	}
	exit(EXIT_SUCCESS);
}
