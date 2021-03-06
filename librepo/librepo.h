/* librepo - A library providing (libcURL like) API to downloading repository
 * Copyright (C) 2012  Tomas Mlcoch
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 */

#ifndef LR_LIBREPO_H
#define LR_LIBREPO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "rcodes.h"
#include "version.h"
#include "types.h"
#include "handle.h"
#include "result.h"
#include "yum.h"
#include "util.h"
#include "checksum.h"
#include "repoutil_yum.h"
#include "package_downloader.h"
#include "repomd.h"

/** \defgroup librepo       Librepo library init function
 */

/** \ingroup librepo
 * Initialize librepo library.
 */
void lr_global_init();

/** \ingroup librepo
 * Clean up librepo library.
 */
void lr_global_cleanup();

#ifdef __cplusplus
}
#endif

#endif
