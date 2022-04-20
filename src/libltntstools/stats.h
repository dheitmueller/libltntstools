#ifndef _STATS_H
#define _STATS_H

/**
 * @file        stats.h
 * @author      Steven Toth <steven.toth@ltnglobal.com>
 * @copyright   Copyright (c) 2020-2022 LTN Global,Inc. All Rights Reserved.
 * @brief       Parse and analyze MPEG-TS transport streams, collect and expose
 *              multiplex and pid specific statistics.
 * 
 * This frame also handles generic network streams (BYTESTREAM)
 * and ATSC3.0 A/324 Common TUnneling Protocol (CTP) streams.
 * You should expect to see some API differences for these type types of streams
 * and, clearly, statsistics will be limited in these use cases.
 * 
 * Usage example, demuxing and parsing Video frames on pid 0x31:
 * 
 *    struct ltntstools_stream_statistics_s myStats;
 *    ltntstools_pid_stats_reset(&myStats);
 * 
 *    while (1) {
 *      ltntstools_pid_stats_update(&myStats, pkts, 7);
 * 
 *      // Query CC issues on an ongoing basis.
 *      uint64_t count = ltntstools_pid_stats_stream_get_cc_errors(&myStats);
 *    }
 * 
 */
#include <time.h>
#include <inttypes.h>
#include <libltntstools/clocks.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_PID 8192

/**
 * @brief A pid specific statistics container, contained within struct ltntstools_stream_statistics_s
 */
struct ltntstools_pid_statistics_s
{
	int      enabled;              /**< Boolean. is the pid available in the multiplex. */
	uint64_t packetCount;          /**< Number of packets processed. */
	uint64_t ccErrors;             /**< Number of continuity counter issues processed */
	uint64_t teiErrors;            /**< Number of transport error indicator issues processed */
	uint64_t scrambledCount;       /**< Number of times we've seen scrambled/encrypted packets */
	uint64_t pcrExceeds40ms;       /**< Number of times the PCR interval has exceeded 40ms */
	uint64_t prev_pcrExceeds40ms;  /**< Prior value of pcrExceeds40ms, updated every ltntstools_pid_stats_update() call */

	uint8_t  lastCC;               /**< Last CC value sobserved */

	time_t   pps_last_update;      /**< Maintain a packets per second count, we can convert this into Mb/ps */
	uint32_t pps;                  /**< Helper var for computing bitrate */
	uint32_t pps_window;           /**< Helper var for computing bitrate */
	double   mbps;                 /**< Updated once per second. */

	int hasPCR;                    /**< User specifically told is this PID will contain a PCR */
	int seenPCR;                   /**< Helper var to track PCR values seen, and skipped during startup for stability. */
#define ltntstools_CLOCK_PCR 0
#define ltntstools_CLOCK_PTS 1
#define ltntstools_CLOCK_DTS 2
	struct ltntstools_clock_s clocks[3]; /**< Three clocks potentially per pid. See ltntstools_CLOCK_PCR, ltntstools_CLOCK_PTS and ltntstools_CLOCK_DTS */
};

/**
 * @brief A larger statistics container, representing all pids in an entire SPTS/MPTS.
 */
struct ltntstools_stream_statistics_s
{
	struct ltntstools_pid_statistics_s pids[MAX_PID];
	uint64_t packetCount;          /**< Total number of packets processed. */
	uint64_t teiErrors;            /**< Total number of transport error indicator issues processed */
	uint64_t ccErrors;             /**< Total number of continuity counter issues processed */
	uint64_t scrambledCount;       /**< Total number of times we've seen scrambled/encrypted packets */
	uint64_t pcrExceeds40ms;       /**< Total number of times the PCR interval has exceeded 40ms */
	uint64_t prev_pcrExceeds40ms;  /**< Prior value of pcrExceeds40ms, updated every ltntstools_pid_stats_update() call */

	time_t pps_last_update;        /**< Maintain a packets per second count, we can convert this into Mb/ps */
	uint32_t pps;                  /**< Helper var for computing bitrate */
	uint32_t pps_window;           /**< Helper var for computing bitrate */
	double mbps;                   /**< Updated once per second. */

	uint16_t a324_sequence_number; /**< A/324 - Last seqeuence number observed. */

	time_t Bps_last_update;        /**< A/324 Maintain a packets per second count, we can convert this into Mb/ps */
	uint32_t Bps;                  /**< Helper var for computing bitrate */
	uint32_t Bps_window;           /**< Helper var for computing bitrate */
	double a324_mbps;              /**< Updated once per second. */
	double a324_bps;               /**< Updated once per second. */
};

/**
 * @brief       For a given packet, and a known previous continuity counter value, determine
 *              if pkt is sequentually continious, or not.
 * @param[in]   const uint8_t *pkt - A fully aligned single transport packet.
 * @param[in]   uint8_t oldCC - Previous CC value for this packet pid.
 * @return      Boolean.
 */
int ltntstools_isCCInError(const uint8_t *pkt, uint8_t oldCC);

/**
 * @brief       Write an entire MPTS into the framework, update the stream and pid statistics.
 * @param[in]   struct ltntstools_stream_statistics_s *stream - Handle / context.
 * @param[in]   const uint8_t *pkts - one or more aligned transport packets
 * @param[in]   uint32_t packetCount - number of packets
 */
void ltntstools_pid_stats_update(struct ltntstools_stream_statistics_s *stream, const uint8_t *pkts, uint32_t packetCount);

/**
 * @brief       Write a basic ascii pid report to the file descriptor;
 * @param[in]   struct ltntstools_stream_statistics_s *stream - Handle / context.
 * @param[in]   int fd - file descriptor
 */
void ltntstools_pid_stats_dprintf(struct ltntstools_stream_statistics_s *stream, int fd);

/**
 * @brief       Reset all statistics.
 * @param[in]   struct ltntstools_stream_statistics_s *stream - Handle / context.
 */
void ltntstools_pid_stats_reset(struct ltntstools_stream_statistics_s *stream);

/**
 * @brief       Query CTP stream bitrate in Mb/ps
 * @param[in]   struct ltntstools_stream_statistics_s *stream - Handle / context.
 * @return      double - bitrate
 */
double   ltntstools_ctp_stats_stream_get_mbps(struct ltntstools_stream_statistics_s *stream);

/**
 * @brief       Query BYTESTREAM stream bitrate in Mb/ps
 * @param[in]   struct ltntstools_stream_statistics_s *stream - Handle / context.
 * @return      double - bitrate
 */
double   ltntstools_bytestream_stats_stream_get_mbps(struct ltntstools_stream_statistics_s *stream);

/**
 * @brief       Query TRANSPORT stream bitrate in Mb/ps
 * @param[in]   struct ltntstools_stream_statistics_s *stream - Handle / context.
 * @return      double - bitrate
 */
double   ltntstools_pid_stats_stream_get_mbps(struct ltntstools_stream_statistics_s *stream);

/**
 * @brief       Query TRANSPORT stream - transport packets per second.
 * @param[in]   struct ltntstools_stream_statistics_s *stream - Handle / context.
 * @return      uint32_t  - packets per second
 */
uint32_t ltntstools_pid_stats_stream_get_pps(struct ltntstools_stream_statistics_s *stream);

/**
 * @brief       Query TRANSPORT stream - transport bits per second.
 * @param[in]   struct ltntstools_stream_statistics_s *stream - Handle / context.
 * @return      uint32_t  - bits per second
 */
uint32_t ltntstools_pid_stats_stream_get_bps(struct ltntstools_stream_statistics_s *stream);

/**
 * @brief       Query CTP stream - transport bits per second.
 * @param[in]   struct ltntstools_stream_statistics_s *stream - Handle / context.
 * @return      uint32_t  - bits per second
 */
uint32_t ltntstools_ctp_stats_stream_get_bps(struct ltntstools_stream_statistics_s *stream);

/**
 * @brief       Query BYTESTREAM stream - transport bits per second.
 * @param[in]   struct ltntstools_stream_statistics_s *stream - Handle / context.
 * @return      uint32_t  - bits per second
 */
uint32_t ltntstools_bytestream_stats_stream_get_bps(struct ltntstools_stream_statistics_s *stream);

/**
 * @brief       Query TRANSPORT stream - CC error count since last ltntstools_pid_stats_reset()
 * @param[in]   struct ltntstools_stream_statistics_s *stream - Handle / context.
 * @return      uint64_t - count
 */
uint64_t ltntstools_pid_stats_stream_get_cc_errors(struct ltntstools_stream_statistics_s *stream);

/**
 * @brief       Query TRANSPORT stream - Transport error indicator count since last ltntstools_pid_stats_reset()
 * @param[in]   struct ltntstools_stream_statistics_s *stream - Handle / context.
 * @return      uint64_t - count
 */
uint64_t ltntstools_pid_stats_stream_get_tei_errors(struct ltntstools_stream_statistics_s *stream);

/**
 * @brief       Query TRANSPORT stream - Scrambled packets detected since last ltntstools_pid_stats_reset()
 * @param[in]   struct ltntstools_stream_statistics_s *stream - Handle / context.
 * @return      uint64_t - count
 */
uint64_t ltntstools_pid_stats_stream_get_scrambled_count(struct ltntstools_stream_statistics_s *stream);

/**
 * @brief       Query TRANSPORT stream - overall stream padding percentage for the entire mux.
 * @param[in]   struct ltntstools_stream_statistics_s *stream - Handle / context.
 * @return      uint32_t - percent
 */
uint32_t ltntstools_pid_stats_stream_padding_pct(struct ltntstools_stream_statistics_s *stream);

/**
 * @brief       Query TRANSPORT stream - Did any pids violate PCR transport timing windows?
 * @param[in]   struct ltntstools_stream_statistics_s *stream - Handle / context.
 * @return      Boolean.
 */
int      ltntstools_pid_stats_stream_did_violate_pcr_timing(struct ltntstools_stream_statistics_s *stream);

/**
 * @brief       Query TRANSPORT, bitrate in Mb/ps, specifically for input pid.
 * @param[in]   struct ltntstools_stream_statistics_s *stream - Handle / context.
 * @param[in]   uint16_t pidnr - pid
 * @return      double - bitrate
 */
double   ltntstools_pid_stats_pid_get_mbps(struct ltntstools_stream_statistics_s *stream, uint16_t pidnr);

/**
 * @brief       Query TRANSPORT, packets per second (188), specifically for input pid.
 * @param[in]   struct ltntstools_stream_statistics_s *stream - Handle / context.
 * @param[in]   uint16_t pidnr - pid
 * @return      packets per second
 */
uint32_t ltntstools_pid_stats_pid_get_pps(struct ltntstools_stream_statistics_s *stream, uint16_t pidnr);

/**
 * @brief       Query TRANSPORT, bits per second (188), specifically for input pid.
 * @param[in]   struct ltntstools_stream_statistics_s *stream - Handle / context.
 * @param[in]   uint16_t pidnr - pid
 * @return      bps
 */
uint32_t ltntstools_pid_stats_pid_get_bps(struct ltntstools_stream_statistics_s *stream, uint16_t pidnr);

/**
 * @brief       Query TRANSPORT, packet count, specifically for input pid.
 * @param[in]   struct ltntstools_stream_statistics_s *stream - Handle / context.
 * @param[in]   uint16_t pidnr - pid
 * @return      packet count
 */
uint64_t ltntstools_pid_stats_pid_get_packet_count(struct ltntstools_stream_statistics_s *stream, uint16_t pidnr);

/**
 * @brief       Query TRANSPORT, last time the pid statistics were updated, specifically for input pid.
 * @param[in]   struct ltntstools_stream_statistics_s *stream - Handle / context.
 * @param[in]   uint16_t pidnr - pid
 * @return      time_t lastUpdate
 */
time_t   ltntstools_pid_stats_pid_get_last_update(struct ltntstools_stream_statistics_s *stream, uint16_t pidnr);

/**
 * @brief       Query TRANSPORT - Did input pid violate PCR transport timing windows?
 * @param[in]   struct ltntstools_stream_statistics_s *stream - Handle / context.
 * @param[in]   uint16_t pidnr - pid
 * @return      Boolean.
 */
int ltntstools_pid_stats_pid_did_violate_pcr_timing(struct ltntstools_stream_statistics_s *stream, uint16_t pidnr);

/**
 * @brief       Query TRANSPORT - Inform framework that this pid contains a PCR and PCR clocks math should be performed.
 * @param[in]   struct ltntstools_stream_statistics_s *stream - Handle / context.
 * @param[in]   uint16_t pidnr - pid
 */
void ltntstools_pid_stats_pid_set_contains_pcr(struct ltntstools_stream_statistics_s *stream, uint16_t pidnr);

/**
 * @brief       Query TRANSPORT - Check if input PID is expected to have a PCR.
 * @param[in]   struct ltntstools_stream_statistics_s *stream - Handle / context.
 * @param[in]   uint16_t pidnr - pid
 * @return      Boolean.
 */
int ltntstools_pid_stats_pid_get_contains_pcr(struct ltntstools_stream_statistics_s *stream, uint16_t pidnr);

/**
 * @brief       Write a CTP buffer into the stats layer.
 *              Limited but useful stats will be collected and exposed.
 *              ATSC3.0 A/324 stats wedged into this framework, better than nothing.
 * @param[in]   struct ltntstools_stream_statistics_s *stream - Handle / context.
 * @param[in]   const uint8_t *buf - CTP buffer of bytes
 * @param[in]   uint32_t lengthBytes - length of CTP buffer in bytes
 */
void ltntstools_ctp_stats_update(struct ltntstools_stream_statistics_s *stream, const uint8_t *buf, uint32_t lengthBytes);

/**
 * @brief       Write a generic BYTESTREAM buffer into the stats layer.
 *              Limited but useful stats will be collected and exposed.
 *              Use for SMPTE2110 for example.
 * @param[in]   struct ltntstools_stream_statistics_s *stream - Handle / context.
 * @param[in]   const uint8_t *buf - CTP buffer of bytes
 * @param[in]   uint32_t lengthBytes - length of CTP buffer in bytes
 */
void ltntstools_bytestream_stats_update(struct ltntstools_stream_statistics_s *stream, const uint8_t *buf, uint32_t lengthBytes);

#ifdef __cplusplus
};
#endif

#endif /* _STATS_H */
