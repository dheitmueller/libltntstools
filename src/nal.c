#include "libltntstools/nal.h"
#include "libltntstools/ts.h"
#include <inttypes.h>

#include <libavutil/internal.h>
#include <libavcodec/golomb.h>

int ltn_nal_findHeader(const uint8_t *buffer, int lengthBytes, int *offset)
{
	const uint8_t sig[] = { 0, 0, 1 };

	for (int i = (*offset + 1); i < lengthBytes - sizeof(sig); i++) {
		if (memcmp(buffer + i, sig, sizeof(sig)) == 0) {

			/* Check for the forbidden zero bit, it's illegal to be high in a nal (conflicts with PES headers. */
			if (*(buffer + i + 3) & 0x80)
				continue;

			*offset = i;
			return 0; /* Success */
		}
	}

	return -1; /* Not found */
}

static struct hevcNal_s {
	const char *name;
	const char *type;
} hevcNals[] = {
	[ 0] = { "TRAIL_N", .type = "AUTO" },
	[ 1] = { "TRAIL_R", .type = "IDR" },
	[ 2] = {   "TSA_N", .type = "I" },
	[ 3] = {   "TSA_R", .type = "P" },
	[ 4] = {  "STSA_N", .type = "BREF" },
	[ 5] = {  "STSA_R", .type = "B" },
	[ 6] = { "RADL_N" },
	[ 7] = { "RADL_R" },
	[ 8] = { "RASL_N" },
	[ 9] = { "RASL_R" },

	[19] = { "IDR_W_RADL", .type = "IDR" },
	[20] = { "IDR_N_LP" },
	[21] = { "CRA" },

	[32] = { "VPS" },
	[33] = { "SPS" },
	[34] = { "PPS" },
	[35] = { "AUD" },
	[36] = { "EOS" },
	[37] = { "EOB" },
	[38] = { "FD" },
	[39] = { "PREFIX_SEI" },
	[40] = { "SUFFIX_SEI" },
};

const char *hevcNals_lookupName(int nalType)
{
	return hevcNals[nalType].name;
}

const char *hevcNals_lookupType(int nalType)
{
	return hevcNals[nalType].type;
}

char *ltn_nal_hevc_findNalTypes(const uint8_t *buffer, int lengthBytes)
{
	char *arr = malloc(128);
	arr[0] = 0;

	int items = 0;
	int offset = -1;
	while (ltn_nal_findHeader(buffer, lengthBytes, &offset) == 0) {
		unsigned int nalType = (buffer[offset + 3] >> 1) & 0x3f;
		const char *nalName = hevcNals_lookupName(nalType);
		const char *nalTypeDesc = hevcNals_lookupType(nalType);

		if (items++ > 0)
			sprintf(arr + strlen(arr), ", ");

		sprintf(arr + strlen(arr), "%s", nalName);
#if 0
		printf("%6d: %02x %02x %02x %02x : type %2d (%s)\n",
			offset,
			buffer[offset + 0],
			buffer[offset + 1],
			buffer[offset + 2],
			buffer[offset + 3],
			nalType,
			nalName);
#endif
	}

	return arr;
}

static struct h264Nal_s {
	const char *name;
	const char *type;
} h264Nals[] = {
	[ 0] = { "UNSPECIFIED", .type = "AUTO" },
	[ 1] = { "slice_layer_without_partitioning_rbsp non-IDR", .type = "P" },
	[ 2] = { "slice_data_partition_a_layer_rbsp(", .type = "P" },
	[ 3] = { "slice_data_partition_b_layer_rbsp(", .type = "P" },
	[ 4] = { "slice_data_partition_c_layer_rbsp(", .type = "P" },
	[ 5] = { "slice_layer_without_partitioning_rbsp IDR", .type = "IDR" },
	[ 6] = { "SEI" },
	[ 7] = { "SPS" },
	[ 8] = { "PPS" },
	[ 9] = { "AUD" },
	[10] = { "EO SEQ" },
	[11] = { "EO STREAM" },
	[12] = { "FILLER" },
	[13] = { "SPS-EX" },
	[14] = { "PNU" },
	[15] = { "SSPS" },
	[16] = { "DPS" },
	[19] = { "ACP" },
	[20] = { "CSE" },
	[21] = { "CSEDV" },
};

const char *h264Nals_lookupName(int nalType)
{
	return h264Nals[nalType].name;
}

const char *h264Nals_lookupType(int nalType)
{
	return h264Nals[nalType].type;
}

char *ltn_nal_h264_findNalTypes(const uint8_t *buffer, int lengthBytes)
{
	char *arr = calloc(1, 128);
	arr[0] = 0;

	int items = 0;
	int offset = -1;
	while (ltn_nal_findHeader(buffer, lengthBytes, &offset) == 0) {
		unsigned int nalType = buffer[offset + 3] & 0x1f;
		const char *nalName = h264Nals_lookupName(nalType);
		const char *nalTypeDesc = h264Nals_lookupType(nalType);

		if (items++ > 0)
			sprintf(arr + strlen(arr), ", ");

		sprintf(arr + strlen(arr), "%s", nalName);
#if 0
		printf("%6d: %02x %02x %02x %02x : type %2d (%s)\n",
			offset,
			buffer[offset + 0],
			buffer[offset + 1],
			buffer[offset + 2],
			buffer[offset + 3],
			nalType,
			nalName);
#endif
	}
	
	if (items == 0) {
		free(arr);
		return NULL;
	}

	return arr;
}


struct h264_slice_data_s
{
	uint32_t  slice_type;
	uint64_t  count;
	char     *name;
};

#define MAX_H264_SLICE_TYPES 10
struct h264_slice_data_s slice_defaults[MAX_H264_SLICE_TYPES] = {
	{ 0, 0, "P", },
	{ 1, 0, "B", },
	{ 2, 0, "I", },
	{ 3, 0, "p", },
	{ 4, 0, "i", },
	{ 5, 0, "P", },
	{ 6, 0, "B", },
	{ 7, 0, "I", },
	{ 8, 0, "p", },
	{ 9, 0, "i", },
};

const char *h274_slice_name_ascii(int slice_type)
{
	return &slice_defaults[ slice_type % MAX_H264_SLICE_TYPES ].name[0];
}

struct h264_slice_counter_s
{
	uint16_t pid;
	struct h264_slice_data_s slice[MAX_H264_SLICE_TYPES];

	int nextHistoryPos;
    char sliceHistory[H264_SLICE_COUNTER_HISTORY_LENGTH];
};

void h264_slice_counter_reset(void *ctx)
{
	struct h264_slice_counter_s *s = (struct h264_slice_counter_s *)ctx;
	memcpy(s->slice, slice_defaults, sizeof(slice_defaults));
	for (int i = 0; i < H264_SLICE_COUNTER_HISTORY_LENGTH; i++) {
		s->sliceHistory[i] = ' ';
	}
	s->sliceHistory[H264_SLICE_COUNTER_HISTORY_LENGTH] = 0;
}

void *h264_slice_counter_alloc(uint16_t pid)
{
	struct h264_slice_counter_s *s = malloc(sizeof(*s));
	s->pid = pid;
	h264_slice_counter_reset(s);
	return (void *)s;
}

void h264_slice_counter_free(void *ctx)
{
	struct h264_slice_counter_s *s = (struct h264_slice_counter_s *)ctx;
	free(s);
}

void h264_slice_counter_update(void *ctx, int slice_type)
{
	struct h264_slice_counter_s *s = (struct h264_slice_counter_s *)ctx;
	s->slice[ slice_type ].count++;

    s->sliceHistory[s->nextHistoryPos++ % H264_SLICE_COUNTER_HISTORY_LENGTH] = s->slice[ slice_type ].name[0];
}

void h264_slice_counter_dprintf(void *ctx, int fd, int printZeroCounts)
{
	struct h264_slice_counter_s *s = (struct h264_slice_counter_s *)ctx;
	dprintf(fd, "Type  Name  Count (H264 slice types for pid 0x%04x)\n", s->pid);
	for (int i = MAX_H264_SLICE_TYPES - 1; i >= 0 ; i--) {
		struct h264_slice_data_s *sl = &s->slice[i];
		if (sl->count == 0 && !printZeroCounts)
			continue;
		dprintf(fd, "%4d  %4s  %" PRIu64 "\n", sl->slice_type, sl->name, sl->count);
	}
}

static void h264_slice_counter_write_packet(void *ctx, const unsigned char *pkt)
{

	struct h264_slice_counter_s *s = (struct h264_slice_counter_s *)ctx;
	GetBitContext gb;

	int offset = -1;
	while (offset < ((1 * 188) - 5)) {
		if (ltn_nal_findHeader(pkt, 188, &offset) == 0) {
			unsigned int nalType = *(pkt + offset + 3) & 0x1f;
#if 0
			printf("nal at 0x%04x: ", offset);
			for (int i = 0; i < 6; i++)
				printf("%02x ", *(pkt + offset + i));

			printf("NalType : %02x - %s ", nalType, h264Nals_lookupName(nalType));
#endif
			switch (nalType) {
			case 1: /* slice_layer_without_partitioning_rbsp */
			case 2: /* slice_data_partition_a_layer_rbsp */
#if 0
			/* No slice headers at the beginnig of these structs */
			case 3: /* slice_data_partition_b_layer_rbsp */
			case 4: /* slice_data_partition_c_layer_rbsp */

			// Problems from audio packets that were mi-interpreted in 0x2000 mode.
			offset  52, nal? 19 slice 10: 00 00 01 13 8b 3c f3 55 51 45 58 61
			offset  76, nal? 19 slice 14: 00 00 01 53 8f 4c f3 d6 55 25 94 49
			offset 100, nal? 19 slice 14: 00 00 01 13 8f 44 f3 90 41 24 10 44
			offset 100, nal?  1 slice 27: 00 00 01 01 87 1b 88 7c 80 ec 17 8b
#endif
			case 5: /* slice_layer_without_partitioning_rbsp */
			case 19: /* slice_layer_without_partitioning_rbsp */
				init_get_bits8(&gb, pkt + offset + 4, 4);
				get_ue_golomb(&gb); /* first_mb_in_slice */
				int slice_type = get_ue_golomb(&gb);
				if (slice_type < MAX_H264_SLICE_TYPES) {
					h264_slice_counter_update(s, slice_type);
					//h264_slice_counter_dprintf(s, 0, 0);
				} else {
					/* Malformed stream, not a video stream probably.
					 * in 0x2000 mode we catch audio using this filter
					 * and we need to ignore it.
					 */
					printf("PKT : ");
					for (int i = 0; i < 8; i++)
						printf("%02x ", *(pkt + i));
					printf("\n  -> offset %3d, nal? %2d slice %2d: ", offset, nalType, slice_type);
					for (int i = 0; i < 12; i++)
						printf("%02x ", *(pkt + offset + i));
					printf("\n");
				}
				break;
			}
#if 0
			printf("\n");
#endif
		} else
			break;
	}
}

void h264_slice_counter_write(void *ctx, const unsigned char *pkts, int pktCount)
{
	struct h264_slice_counter_s *s = (struct h264_slice_counter_s *)ctx;
	for (int i = 0; i < pktCount; i++) {
		uint16_t pid = ltntstools_pid(pkts + (i * 188));
		if (s->pid == 0x2000 || pid == s->pid) {
			h264_slice_counter_write_packet(s, pkts + (i * 188));
		}
	}
}

void h264_slice_counter_query(void *ctx, struct h264_slice_counter_results_s *results)
{
	struct h264_slice_counter_s *s = (struct h264_slice_counter_s *)ctx;
	results->i = s->slice[2].count + s->slice[7].count;
	results->b = s->slice[1].count + s->slice[6].count;
	results->p = s->slice[0].count + s->slice[5].count;
	results->si = s->slice[4].count + s->slice[9].count;
	results->sp = s->slice[3].count + s->slice[8].count;

	int p = s->nextHistoryPos;
	for (int i = 0; i < H264_SLICE_COUNTER_HISTORY_LENGTH; i++) {
		results->sliceHistory[i] = s->sliceHistory[p++ % H264_SLICE_COUNTER_HISTORY_LENGTH];
	}
	results->sliceHistory[H264_SLICE_COUNTER_HISTORY_LENGTH] = 0x0;
}

uint16_t h264_slice_counter_get_pid(void *ctx)
{
	struct h264_slice_counter_s *s = (struct h264_slice_counter_s *)ctx;
	return s->pid;
}

void h264_slice_counter_reset_pid(void *ctx, uint16_t pid)
{
	struct h264_slice_counter_s *s = (struct h264_slice_counter_s *)ctx;
	s->pid = pid;
	h264_slice_counter_reset(ctx);
}