/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2023 Ingenic
 */

#if !defined(_AIP_V20_TRACE_H_) || defined(TRACE_HEADER_MULTI_READ)
#define _AIP_V20_TRACE_H_

#include <linux/stringify.h>
#include <linux/types.h>
#include <linux/tracepoint.h>
#include <linux/string.h>

#define JZ_AIP_JOB_NAME_LEN             8

#undef TRACE_SYSTEM
#define TRACE_SYSTEM aip_v20
#define TRACE_INCLUDE_FILE jz_aip_v20_trace

TRACE_EVENT(aip_v20_job,
	    TP_PROTO(const char *dev_name, char *job_name,
		    uint64_t seqno,
		    int64_t total_time,
		    int64_t run_time,
		    uint32_t hw_time),
	    TP_ARGS(dev_name, job_name, seqno, total_time, run_time, hw_time),

	    TP_STRUCT__entry(
			     __field(const char*, dev_name);
			     __field(char, job_name[JZ_AIP_JOB_NAME_LEN]);
			     __field(u64, seqno)
			     __field(s64, total_time)
			     __field(s64, run_time)
			     __field(u32, hw_time)
			     ),

	    TP_fast_assign(
			   __entry->dev_name = dev_name;
			   memcpy(__entry->job_name, job_name, sizeof(__entry->job_name));
			   __entry->seqno = seqno;
			   __entry->total_time = total_time;
			   __entry->run_time = run_time;
			   __entry->hw_time = hw_time;
			   ),

	    TP_printk("%s[%s] %lld, %lld, %lld, %d",
			__entry->dev_name,
			__entry->job_name,
			__entry->seqno,
			__entry->total_time,
			__entry->run_time,
			__entry->hw_time)
);

TRACE_EVENT(nna_v20_job,
	    TP_PROTO(const char *dev_name, char *job_name,
		    uint64_t seqno,
		    int64_t sched_time,
		    int64_t run_time,
		    int64_t total_time),
	    TP_ARGS(dev_name, job_name, seqno, sched_time, run_time, total_time),

	    TP_STRUCT__entry(
			     __field(const char*, dev_name);
			     __field(char, job_name[JZ_AIP_JOB_NAME_LEN]);
			     __field(u64, seqno)
			     __field(s64, sched_time)
			     __field(s64, run_time)
			     __field(s64, total_time)
			     ),

	    TP_fast_assign(
			   __entry->dev_name = dev_name;
			   memcpy(__entry->job_name, job_name, sizeof(__entry->job_name));
			   __entry->seqno = seqno;
			   __entry->sched_time = sched_time;
			   __entry->run_time = run_time;
			   __entry->total_time = total_time;
			   ),

	    TP_printk("%s[%s](%lld): %lld, %lld, %lld",
		     __entry->dev_name,
		     __entry->job_name,
		     __entry->seqno,
		     __entry->sched_time,
		     __entry->run_time,
		     __entry->total_time)
);

#endif /* _AIP_V20_TRACE_H_ */

/* This part must be outside protection */
#undef TRACE_INCLUDE_PATH
#define TRACE_INCLUDE_PATH ../../drivers/video/ingenic_aip_v20
#include <trace/define_trace.h>
