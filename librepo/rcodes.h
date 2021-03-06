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

#ifndef LR_RETURN_CODES_H
#define LR_RETURN_CODES_H

#ifdef __cplusplus
extern "C" {
#endif

/** \defgroup   rcodes      Librepo return codes
 */

/** \ingroup rcodes
 * Return/Error codes
 */
typedef enum {
    LRE_OK,                         /*!< (0) everything is ok */
    LRE_BADFUNCARG,                 /*!< (1) bad function argument */

    /* lr_setopt specific */
    LRE_BADOPTARG,                  /*!< (2) bad argument of the option */
    LRE_UNKNOWNOPT,                 /*!< (3) library doesn't know the option */
    LRE_CURLSETOPT,                 /*!< (4) cURL doesn't know the option.
                                         Too old curl version? */
    LRE_ALREADYUSEDRESULT,          /*!< (5) lr_Result object is not clean */
    LRE_INCOMPLETERESULT,           /*!< (6) lr_Result doesn't contain all what
                                         is needed */
    LRE_CURLDUP,                    /*!< (7) cannot duplicate curl handle */
    LRE_CURL,                       /*!< (8) cURL error. Use the
                                         lr_last_curl_error to get CURLcode */
    LRE_CURLM,                      /*!< (9) cULR multi handle error. Use the
                                         lr_last_mculr_error to get CURLMcode */
    LRE_BADSTATUS,                  /*!< (10) HTTP or FTP returned status code
                                         which do not represent success
                                         (file doesn't exists, etc.) */
    LRE_TEMPORARYERR,               /*!< (11) some error that should be temporary
                                         and next try could work
                                         (HTTP status codes 500, 502-504,
                                          operation timeout, ...) */
    LRE_NOTLOCAL,                   /*!< (12) URL is not a local address */
    LRE_CANNOTCREATEDIR,            /*!< (13) cannot create a directory in output
                                         dir (the directory already exists?) */
    LRE_IO,                         /*!< (14) input output error */
    LRE_MLBAD,                      /*!< (15) bad mirrorlist/metalink file
                                         (metalink doesn't contain needed file,
                                         mirrorlist doesn't contain urls, ..) */
    LRE_MLXML,                      /*!< (16) metalink XML parse error */
    LRE_BADCHECKSUM,                /*!< (17) bad checksum */
    LRE_REPOMDXML,                  /*!< (18) repomd XML parse error */
    LRE_NOURL,                      /*!< (19) usable URL not found */
    LRE_CANNOTCREATETMP,            /*!< (20) cannot create tmp directory */
    LRE_UNKNOWNCHECKSUM,            /*!< (21) unknown type of checksum is needed
                                         for verification one or more files */
    LRE_BADURL,                     /*!< (22) bad URL specified */
    LRE_GPGNOTSUPPORTED,            /*!< (23) OpenPGP protocol is not supported */
    LRE_GPGERROR,                   /*!< (24) GPG error */
    LRE_BADGPG,                     /*!< (25) Bad GPG signature */
    LRE_INCOMPLETEREPO,             /*!< (26) Repository metadata are not complete */
    LRE_UNKNOWNERROR,               /*!< unknown error - sentinel of
                                         error codes enum */
} lr_Rc; /*!< Return codes */

const char *lr_strerror(int rc);

#ifdef __cplusplus
}
#endif

#endif
