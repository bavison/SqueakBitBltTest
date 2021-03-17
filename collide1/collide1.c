/*
 * Copyright © 2014 Raspberry Pi Foundation
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

#include "BitBltDispatch.h"

#define sqInt int

extern sqInt initialiseModule(void);

static bool allOK = true;

static void test(compare_operation_t *op, src_or_dest_t *srcA, src_or_dest_t * srcB, uint32_t shouldBe)
{
    uint32_t result;
    op->srcA = *srcA;
    op->srcB = *srcB;
    result = compareColorsDispatch(op);
    if (result != shouldBe)
    {
        allOK = false;
        printf("Rule %s, width %u, height %u\n", (const char *[]) { "pixelMatch", "notAnotB", "notAmatchB" } [op->matchRule], op->width, op->height);
        printf("A: %u bpp, %s-endian, stride %u, x %u, y %u\n", srcA->depth, srcA->msb ? "big" : "little", srcA->pitch, srcA->x, srcA->y);
        printf("B: %u bpp, %s-endian, stride %u, x %u, y %u\n", srcB->depth, srcB->msb ? "big" : "little", srcB->pitch, srcB->x, srcB->y);
        printf("Result = %u, should be %u\n\n", result, shouldBe);
    }
}

int main(int argc, char *argv[])
{
    (void) argc;
    (void) argv;
#define BOX_SIZE 5
//#define pixel_t uint32_t
//#define pixel_t uint16_t
#define pixel_t uint8_t

	initialiseCopyBits();
	initialiseModule();

#define SRC1_STRIDE     ((sizeof (pixel_t) * BOX_SIZE * 5 + 3) &~ 3)
#define SRC1_STRIDE_PIX (SRC1_STRIDE / sizeof (pixel_t))
#define SRC2_STRIDE     ((sizeof (pixel_t) * BOX_SIZE * 3 + 3) &~ 3)
#define SRC2_STRIDE_PIX (SRC2_STRIDE / sizeof (pixel_t))
    pixel_t src1bits[SRC1_STRIDE_PIX * BOX_SIZE*5];
    pixel_t src2bits[SRC2_STRIDE_PIX * BOX_SIZE*3];

    /* Fill most of each image with "transparent" */
    memset(src1bits, 0, sizeof src1bits);
    memset(src2bits, 0, sizeof src2bits);
    /* And a central box with "white" */
    for (size_t j = 0; j < BOX_SIZE; j++)
    {
        for (size_t i = 0; i < BOX_SIZE; i++)
        {
            size_t index1 = (BOX_SIZE*2+j)*SRC1_STRIDE_PIX + BOX_SIZE*2+i;
            size_t index2 = (BOX_SIZE*1+j)*SRC2_STRIDE_PIX + BOX_SIZE*1+i;
            /* Now swap the endianness */
            index1 ^= (4 / sizeof (pixel_t)) - 1;
            index2 ^= (4 / sizeof (pixel_t)) - 1;
            src1bits[index1] = (pixel_t) -1u;
            src2bits[index2] = (pixel_t) -1u;
        }
    }

    src_or_dest_t src1;
    src1.bits = src1bits;
    src1.depth = 8*sizeof(pixel_t);
    src1.pitch = SRC1_STRIDE;
    src1.msb = true;
    src_or_dest_t src2;
    src2.bits = src2bits;
    src2.depth = 8*sizeof(pixel_t);
    src2.pitch = SRC2_STRIDE;
    src2.msb = true;
    src2.x = 0;
    src2.y = 0;
    compare_operation_t op;
    op.tally = true;
    op.width = BOX_SIZE*3;
    op.height = BOX_SIZE*3;

    /* Try a range of offsets */
    for (src1.y = 0; src1.y <= BOX_SIZE*2; src1.y++)
    {
        for (src1.x = 0; src1.x <= BOX_SIZE*2; src1.x++)
        {
            uint32_t shouldBe = (BOX_SIZE - abs(src1.x - BOX_SIZE)) * (BOX_SIZE - abs(src1.y - BOX_SIZE));
            /* Try all rules */
            for (op.matchRule = MR_pixelMatch; op.matchRule <= MR_notAmatchB; op.matchRule++)
            {
                op.colorA = op.matchRule == MR_pixelMatch ? (pixel_t) -1u : 0;
                op.colorB = op.matchRule == MR_notAnotB ? 0 : (pixel_t) -1u;
                /* Try images in both orders */
                test(&op, &src1, &src2, shouldBe);
                test(&op, &src2, &src1, shouldBe);
            }
        }
    }

    if (allOK)
        printf("Passes tests OK\n");
    exit(EXIT_SUCCESS);
}
