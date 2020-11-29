/*
 * Copyright (c) 2019 Ramesh Subramonian <subramonian@gmail.com>
 * All rights reserved.
 *
 * Use is subject to license terms, as specified in the LICENSE file.
 */

#ifndef _HMAP_H
#define _HMAP_H

#include "q_incs.h"
#include "fastdiv.h"
#include "hmap_struct.h"
#include "hmap_utils.h"

#define	Q_RHM_SET  1
#define	Q_RHM_ADD 2

#define	HASH_MIN_SIZE		(1024)
#define	HASH_MAX_SIZE		(0) // 0 => no max
#define	MAX_GROWTH_STEP		(1024U * 1024)

#define	LOW_WATER_MARK  0.2
#define	HIGH_WATER_MARK 0.85

#endif
