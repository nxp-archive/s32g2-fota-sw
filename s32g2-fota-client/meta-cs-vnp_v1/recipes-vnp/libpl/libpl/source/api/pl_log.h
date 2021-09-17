/*
 * Copyright 2018-2021 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause

 */
#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#define	PL_LOG_ERR 		1
#define PL_LOG_WARNING  2
#define PL_LOG_NOTICE	3
#define PL_LOG_INFO		4
#define PL_LOG_DEBUG	5

#ifndef ENABLE_LOG_LEVEL 
#define ENABLE_LOG_LEVEL PL_LOG_WARNING
#endif

struct pl_logm;

/*
 * @logm: NULL for global log
 * @level: see PL_LOG_XXX
 * @return: bytes number been logged.
 */
int pl_log(
		struct pl_logm *logm,
		int level,
		const char *fmt, ...);

void pl_set_loglevel(struct pl_logm *logm, int level);

#define PL_LOG(level, fmt, ...) 			pl_log(0, (level), fmt, ##__VA_ARGS__)
#define PL_LOG_LOC(level, fmt, ...) 		pl_log(0, (level), "%s.%d: " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define PL_M_LOG(m, level, fmt, ...)   		pl_log((m), (level), fmt, ##__VA_ARGS__)
#define PL_M_LOG_LOC(m, level, fmt, ...)	pl_log((m), (level), "%s.%d: " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)

#if ENABLE_LOG_LEVEL >= PL_LOG_ERR
#define PL_ERR(fmt, ...) 			pl_log( 0 , PL_LOG_ERR, fmt, ##__VA_ARGS__)
#define PL_ERR_LOC(fmt, ...)		pl_log( 0 , PL_LOG_ERR, "%s.%d: " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define PL_M_ERR(m, fmt, ...)   	pl_log((m), PL_LOG_ERR, fmt, ##__VA_ARGS__)
#define PL_M_ERR_LOC(m, fmt, ...)	pl_log((m), PL_LOG_ERR, "%s.%d: " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#else
#define PL_ERR(fmt, ...) 	
#define PL_ERR_LOC(fmt, ...)
#define PL_M_ERR(m, fmt, ...) 
#define PL_M_ERR_LOC(m, fmt, ...)
#endif

#if ENABLE_LOG_LEVEL >= PL_LOG_WARNING
#define PL_WARN(fmt, ...) 			pl_log( 0 , PL_LOG_WARNING, fmt, ##__VA_ARGS__)
#define PL_WARN_LOC(fmt, ...)		pl_log( 0 , PL_LOG_WARNING, "%s.%d: " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define PL_M_WARN(m, fmt, ...)   	pl_log((m), PL_LOG_WARNING, fmt, ##__VA_ARGS__)
#define PL_M_WARN_LOC(m, fmt, ...)	pl_log((m), PL_LOG_WARNING, "%s.%d: " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#else
#define PL_WARN(fmt, ...) 	
#define PL_WARN_LOC(fmt, ...)
#define PL_M_WARN(m, fmt, ...) 
#define PL_M_WARN_LOC(m, fmt, ...)
#endif

#if ENABLE_LOG_LEVEL >= PL_LOG_NOTICE
#define PL_NOTICE(fmt, ...) 		pl_log( 0 , PL_LOG_NOTICE, fmt, ##__VA_ARGS__)
#define PL_NOTICE_LOC(fmt, ...)		pl_log( 0 , PL_LOG_NOTICE, "%s.%d: " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define PL_M_NOTICE(m, fmt, ...)   	pl_log((m), PL_LOG_NOTICE, fmt, ##__VA_ARGS__)
#define PL_M_NOTICE_LOC(m, fmt, ...)	pl_log((m), PL_LOG_NOTICE, "%s.%d: " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#else
#define PL_NOTICE(fmt, ...) 	
#define PL_NOTICE_LOC(fmt, ...)
#define PL_M_NOTICE(m, fmt, ...) 
#define PL_M_NOTICE_LOC(m, fmt, ...)
#endif

#if ENABLE_LOG_LEVEL >= PL_LOG_INFO
#define PL_INFO(fmt, ...) 			pl_log( 0 , PL_LOG_INFO, fmt, ##__VA_ARGS__)
#define PL_INFO_LOC(fmt, ...)		pl_log( 0 , PL_LOG_INFO, "%s.%d: " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define PL_M_INFO(m, fmt, ...)   	pl_log((m), PL_LOG_INFO, fmt, ##__VA_ARGS__)
#define PL_M_INFO_LOC(m, fmt, ...)	pl_log((m), PL_LOG_INFO, "%s.%d: " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#else
#define PL_INFO(fmt, ...) 	
#define PL_INFO_LOC(fmt, ...)
#define PL_M_INFO(m, fmt, ...) 
#define PL_M_INFO_LOC(m, fmt, ...)

#endif

#if ENABLE_LOG_LEVEL >= PL_LOG_DEBUG
#define PL_DEBUG(fmt, ...) 			pl_log( 0 , PL_LOG_DEBUG, fmt, ##__VA_ARGS__)
#define PL_DEBUG_LOC(fmt, ...)		pl_log( 0 , PL_LOG_DEBUG, "%s.%d: " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define PL_M_DEBUG(m, fmt, ...)   	pl_log((m), PL_LOG_DEBUG, fmt, ##__VA_ARGS__)
#define PL_M_DEBUG_LOC(m, fmt, ...)	pl_log((m), PL_LOG_DEBUG, "%s.%d: " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#else
#define PL_DEBUG(fmt, ...) 	
#define PL_DEBUG_LOC(fmt, ...)
#define PL_M_DEBUG(m, fmt, ...) 
#define PL_M_DEBUG_LOC(m, fmt, ...)
#endif

#ifdef __cplusplus
}
#endif
