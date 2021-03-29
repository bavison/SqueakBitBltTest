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

#include <getopt.h>

#include "BitBltDispatch.h"

#define sqInt int

#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))

extern sqInt initialiseModule(void);

#define LOG2_1 0
#define LOG2_2 1
#define LOG2_4 2
#define LOG2_8 3
#define LOG2_16 4
#define LOG2_32 5

const char *matchRuleName[] = {
        "pixelMatch",
        "notAnotB",
        "notAmatchB"
};

#define MAXWIDTH (1920)
#define MAXHEIGHT (16)

/* Public domain CRC function */
static uint32_t compute_crc32 (uint32_t in_crc32, const void *buf, size_t buf_len)
{
    static const uint32_t crc_table[256] = {
	0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F,
	0xE963A535, 0x9E6495A3, 0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
	0x09B64C2B, 0x7EB17CBD,	0xE7B82D07, 0x90BF1D91, 0x1DB71064, 0x6AB020F2,
	0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
	0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC,	0x14015C4F, 0x63066CD9,
	0xFA0F3D63, 0x8D080DF5, 0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
	0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B, 0x35B5A8FA, 0x42B2986C,
	0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
	0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423,
	0xCFBA9599, 0xB8BDA50F, 0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
	0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D, 0x76DC4190, 0x01DB7106,
	0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
	0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D,
	0x91646C97, 0xE6635C01, 0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
	0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457, 0x65B0D9C6, 0x12B7E950,
	0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
	0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7,
	0xA4D1C46D, 0xD3D6F4FB, 0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
	0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9, 0x5005713C, 0x270241AA,
	0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
	0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81,
	0xB7BD5C3B, 0xC0BA6CAD, 0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
	0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683, 0xE3630B12, 0x94643B84,
	0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
	0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB,
	0x196C3671, 0x6E6B06E7, 0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
	0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5, 0xD6D6A3E8, 0xA1D1937E,
	0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
	0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55,
	0x316E8EEF, 0x4669BE79, 0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
	0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F, 0xC5BA3BBE, 0xB2BD0B28,
	0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
	0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F,
	0x72076785, 0x05005713, 0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
	0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21, 0x86D3D2D4, 0xF1D4E242,
	0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
	0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69,
	0x616BFFD3, 0x166CCF45, 0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
	0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB, 0xAED16A4A, 0xD9D65ADC,
	0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
	0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD70693,
	0x54DE5729, 0x23D967BF, 0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
	0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
    };

    uint32_t              crc32;
    unsigned char *       byte_buf;
    size_t                i;

    /* accumulate crc32 for buffer */
    crc32 = in_crc32 ^ 0xFFFFFFFF;
    byte_buf = (unsigned char*) buf;

    for (i = 0; i < buf_len; i++)
	crc32 = (crc32 >> 8) ^ crc_table[(crc32 ^ byte_buf[i]) & 0xFF];

    return (crc32 ^ 0xFFFFFFFF);
}

static uint32_t randLog2Depth(void)
{
	/* Emphasise 32, 16, 8 bpp over < 8bpp */
	uint32_t result = rand() % 4 + 2;
	if (result == 2)
		result = rand() % 3;
	return result;
}

static void fillWithRand(uint32_t *buf, size_t nWords)
{
	/* Fill a block with random data, but prefer runs of all 1 or 0 */
	size_t totalRemain = nWords * 32;
	uint32_t word = 0;
	size_t wordRemain = 32;
	do {
		enum {
			FILL_ZEROS,
			FILL_RAND,
			FILL_ONES
		} blockType = rand() % 3;
		size_t blockRemain = rand() % 64 + 1;
		blockRemain = MIN(blockRemain, totalRemain);
		do {
			size_t bitsThisTime = MIN(blockRemain, wordRemain);
			if (blockType == FILL_RAND && bitsThisTime > 16)
				bitsThisTime = 16;
			word = bitsThisTime == 32 ? 0 : word << bitsThisTime;
			if (blockType == FILL_ONES) {
				word |= (1ul << bitsThisTime) - 1;
			} else if (blockType == FILL_RAND) {
				word |= rand() & ((1ul << bitsThisTime) - 1);
			}
			totalRemain -= bitsThisTime;
			blockRemain -= bitsThisTime;
			wordRemain -= bitsThisTime;
			if (wordRemain == 0) {
				*buf++ = word;
				wordRemain = 32;
			}
		} while (blockRemain > 0);
	} while (totalRemain > 0);
}

static void dumpBuffer(uint32_t *buf, size_t wordsPerRow, size_t rows, uint32_t log2bpp, bool bigEndian)
{
	uint32_t pixPerWord = 32 >> log2bpp;
	uint32_t bpp = 1u << log2bpp;
	uint32_t pixExtractShift = bigEndian ? 32 - bpp : 0;
	uint32_t pixExtractMask = (1ul << bpp) - 1;
	const char *fmt[6] = { "%s%u", "%s%u", "%s%X", "%s%02X", "%s%04X", "%s%08X" };
	while (rows-- > 0) {
		const char *sep = "";
		for (size_t words = wordsPerRow; words > 0; words--) {
			uint32_t word = *buf++;
			for (size_t pix = pixPerWord; pix > 0; pix--) {
				printf(fmt[log2bpp], sep, (word >> pixExtractShift) & pixExtractMask);
//				if (log2bpp >= 3)
					sep = " ";
				if (bigEndian)
					word <<= bpp;
				else
					word >>= bpp;
			}
		}
		printf("\n");
	}
}

void warning(const char *message)
{
    (void) message;
//    fprintf(stderr, "warning: %s\n", message);
}

int main(int argc, char *argv[])
{
	size_t min_iter = 0;
	size_t max_iter = 524288;
	bool help = false;
	uint32_t verbose = 0;

	bool check = true;
	uint32_t cumulative_crc = 0;
	size_t check_index = 0;
	uint32_t check_table[] = {
			0x066EA0BC, // first 1
			0x178C58C9, // first 2
			0x67E9BB0A, // first 4
			0xBAB1EAD3, // first 8
			0xEF3BBE62, // first 16
			0x370A2867, // first 32
			0xAFBB1455, // first 64
			0xC2047F34, // first 128
			0x73980AA9, // first 256
			0x3A5EDD6E, // first 512
			0x8C11E5F0, // first 1024
			0x70CD9956, // first 2048
			0x2826B20A, // first 4096
			0x3408FE46, // first 8192
			0x290C8B83, // first 16384
			0xFAAF41CA, // first 32768
			0x5FC6DE82, // first 65536
			0xA6788B63, // first 131072
			0x008CD456, // first 262144
			0x4F085D8E, // first 524288
	};
	bool failed = false;

	int opt;
	while ((opt = getopt(argc, argv, "hm:v")) != -1) {
		switch (opt) {
		case 'h': help = true; break;
		case 'm': max_iter = atoi(optarg); break;
		case 'v': verbose++; break;
		}
	}
	if (help || optind < argc-1) {
		fprintf(stderr, "Syntax: %s [-h] [-m max_iterations] [-v] [-v] [-v] [iteration]\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	if (optind == argc-1) {
		min_iter = atoi(argv[optind]);
		max_iter = min_iter + 1;
		verbose = MAX(verbose,2);
		check = false;
	}

	initialiseCopyBits();
	initialiseModule();

	uint32_t srcA[MAXWIDTH * MAXHEIGHT];
    uint32_t srcB[MAXWIDTH * MAXHEIGHT];

	size_t iter;
	for (iter = min_iter; iter < max_iter; iter++) {
		srand(iter ^ (iter << 16));

        size_t srcA_w = rand() % MAXWIDTH + 1;
        size_t srcA_h = rand() % MAXHEIGHT + 1;
		size_t srcB_w = rand() % MAXWIDTH + 1;
		size_t srcB_h = rand() % MAXHEIGHT + 1;

		compare_operation_t op;
		op.matchRule = rand() % (MR_notAmatchB + 1);
		op.tally = rand() & 1;
        op.srcA.bits = srcA;
		op.srcB.bits = srcB;
		uint32_t log2srcADepth = randLog2Depth(), log2srcBDepth = randLog2Depth();
		op.srcA.depth = 1u << log2srcADepth;
        op.srcB.depth = 1u << log2srcBDepth;
		op.srcA.pitch = ((srcA_w * op.srcA.depth + 31) >> 3) &~ 3;
        op.srcB.pitch = ((srcB_w * op.srcB.depth + 31) >> 3) &~ 3;
		/* It's believed that MSB is always true in practice */
		if (rand() % 4 != 0) {
			op.srcA.msb = op.srcB.msb = true;
		} else {
			op.srcA.msb = rand() & 1;
            op.srcB.msb = rand() & 1;
		}
        op.srcA.x = rand() % srcA_w;
		op.srcB.x = rand() % srcB_w;
        op.srcA.y = rand() % srcA_h;
		op.srcB.y = rand() % srcB_h;
		fillWithRand(srcA, srcA_h * op.srcA.pitch / 4);
		fillWithRand(srcB, srcB_h * op.srcB.pitch / 4);
		op.width = (rand() % MIN(srcA_w-op.srcA.x, srcB_w-op.srcB.x)) + 1;
		op.height = (rand() % MIN(srcA_h-op.srcA.y, srcB_h-op.srcB.y)) + 1;
		switch (rand() % 3)
		{
		case 0: op.colorA = 0; break;
		case 1: op.colorA = rand() ^ (rand() << 16); break;
		case 2: op.colorA = -1u; break;
		}
		op.colorA &= (1ul << op.srcA.depth) - 1;
        switch (rand() % 3)
        {
        case 0: op.colorB = 0; break;
        case 1: op.colorB = rand() ^ (rand() << 16); break;
        case 2: op.colorB = -1u; break;
        }
        op.colorB &= (1ul << op.srcB.depth) - 1;

		if (verbose >= 2) {
			printf("Test #%zu\n", iter);
			printf("matchRule = %u (%s), tally = %u\n",
			        op.matchRule,
			        matchRuleName[op.matchRule],
			        op.tally);
            printf("source A    %2"PRIuSQINT" bpp %cE, %4zu x %zu\n",
                    op.srcA.depth, op.srcA.msb ? 'B' : 'L', srcA_w, srcA_h);
			printf("source B    %2"PRIuSQINT" bpp %cE, %4zu x %zu\n",
					op.srcB.depth, op.srcB.msb ? 'B' : 'L', srcB_w, srcB_h);
			printf("offsets %"PRIuSQINT",%"PRIuSQINT" and %"PRIuSQINT",%"PRIuSQINT", size %"PRIuSQINT" x %"PRIuSQINT"\n",
					op.srcA.x, op.srcA.y, op.srcB.x, op.srcB.y, op.width, op.height);
			printf("colours %08"PRIXSQINT", %08"PRIXSQINT"\n", op.colorA, op.colorB);
			if (verbose >= 3) {
                printf("Source A:\n");
                dumpBuffer(srcA, op.srcA.pitch / 4, srcA_h, log2srcADepth, op.srcA.msb);
				printf("Source B:\n");
				dumpBuffer(srcB, op.srcB.pitch / 4, srcB_h, log2srcBDepth, op.srcB.msb);
			}
		}

		uint32_t result = compareColorsDispatch(&op);

        if (verbose == 1)
            printf("%zu:%08X\n", iter, result);
        else if (verbose >= 2)
            printf("Result:\n0x%08X\n\n", result);

		if (check) {
		    cumulative_crc = compute_crc32(cumulative_crc, &result, sizeof result);
			if (check_index < sizeof check_table / sizeof *check_table &&
					(iter & (iter+1)) == 0) {
				if (cumulative_crc != check_table[check_index])
				{
					failed = true;
					fprintf(stderr, "Error found before iteration %zu: cumulative CRC %08X should be %08X\n", iter+1, cumulative_crc, check_table[check_index]);
				}
				check_index++;
			}
		}
	}
	if (check && !failed)
		printf("Passes checks OK\n");
	exit(check && failed ? EXIT_FAILURE : EXIT_SUCCESS);
}
