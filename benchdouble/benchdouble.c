/*
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

#include <sys/time.h>

#include "PixelDouble.h"

#define WIDTH  480
#define HEIGHT 360
#define CACHELINE_LEN 32
#define BPP 16

static uint32_t buffer[HEIGHT * WIDTH * 9 + CACHELINE_LEN];
static uint32_t *src;
static uint32_t *dst;
static uint32_t *dst2;

void PixelDouble16_480_360(uint16_t *dst, const uint16_t *src)
{
    for (int y = HEIGHT - 1; y >= 0; y--)
    {
        for (int x = WIDTH - 1; x >= 0; x--)
        {
            uint16_t c = src[(y * WIDTH + x) ^ 1];
            dst[4 * y * WIDTH + 2 * x] = c;
            dst[4 * y * WIDTH + 2 * x + 1] = c;
            dst[(4 * y + 2) * WIDTH + 2 * x] = c;
            dst[(4 * y + 2) * WIDTH + 2 * x + 1] = c;
        }
    }
}

static uint64_t gettime(void)
{
	struct timeval tv;

	gettimeofday (&tv, NULL);
	return tv.tv_sec * 1000000 + tv.tv_usec;
}

extern void armSimdPixelDouble16_32_16_wide(uint32_t width, uint32_t height, uint32_t *dst, uint32_t dstStride, const uint32_t *src, uint32_t srcStride);
#define PixelDouble16(dst, src) armSimdPixelDouble16_32_16_wide(WIDTH, HEIGHT, dst, WIDTH, src, 0)
extern void armSimdPixelDouble16_32_32_wide(uint32_t width, uint32_t height, uint32_t *dst, uint32_t dstStride, const uint32_t *src, uint32_t srcStride);
#define PixelDouble32(dst, src) armSimdPixelDouble16_32_32_wide(WIDTH * 2, HEIGHT, dst, WIDTH * 2, src, 0)

int main(int argc, char *argv[])
{
	uint64_t t1, t2;
	size_t iterations = 25;
	(void) argc;
	(void) argv;
	src = (uint32_t *)(((uintptr_t) buffer + CACHELINE_LEN - 1) &~ (CACHELINE_LEN - 1));
//	src++;
	dst = src + WIDTH * HEIGHT;
	dst2 = dst + WIDTH * HEIGHT * 4;

	for (uint32_t i = 0; i < WIDTH * HEIGHT; i++)
	    src[i] = i;
	memset(dst, 0xFF, WIDTH * 2 * HEIGHT * 2 * 4);

#if 0
	/* This is to verify that the two versions produce the same results */
#if BPP == 16
    PixelDouble16_480_360((uint16_t *) dst, (uint16_t *) src);
    PixelDouble16(dst2, src);
    printf("%d\n", memcmp(dst, dst2, WIDTH * HEIGHT * 4 * 2));
#else // BPP == 32
    PixelDouble32_480_360(dst, src);
    PixelDouble32(dst2, src);
    printf("%d\n", memcmp(dst, dst2, WIDTH * HEIGHT * 4 * 4));
#endif
#endif

	while (iterations--)
	{
		t1 = gettime();
#define TIMES 100
		for (uint32_t loop = TIMES; loop > 0; loop--)
#if 0
#if BPP == 16
            PixelDouble16_480_360((uint16_t *) dst, (uint16_t *) src);
#else
            PixelDouble32_480_360(dst, src);
#endif
#else
#if BPP == 16
            PixelDouble16(dst, src);
#else
		    PixelDouble32(dst, src);
#endif
#endif
		t2 = gettime();
		printf("%6.2f\n", ((double)5*WIDTH*HEIGHT*TIMES) / (t2 - t1));
		fflush(stdout);
	}
	exit(EXIT_SUCCESS);
}
