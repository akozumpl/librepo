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

#define _POSIX_SOURCE   200112L
#define _BSD_SOURCE

#include <assert.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <curl/curl.h>

#include "setup.h"
#include "curl.h"
#include "rcodes.h"
#include "util.h"
#include "handle_internal.h"
#include "curltargetlist.h"

/* Callback stuff */

struct _lr_SharedCallbackData {
    short *counted;     /*!< wich downloads have already been included into
                             the total_size*/
    int count;          /*!< number of downloaded files*/
    int advertise;      /*!< advertise total_size -
                             0 if at least one download has unknown total size */
    double downloaded;  /*!< yet downloaded  */
    double total_size;  /*!< total size to download */

    lr_ProgressCb cb;   /*!< pointer to user callback */
    void *user_data;    /*!< user callback data */
};
typedef struct _lr_SharedCallbackData * lr_SharedCallbackData;

struct _lr_CallbackData {
    int id;                         /*!< id of the current callback*/
    double downloaded;              /*!< yet downloaded */
    lr_SharedCallbackData scb_data; /*!< shared callback data */
};
typedef struct _lr_CallbackData * lr_CallbackData;

struct _lr_SingleCallbackData {
    lr_ProgressCb cb;   /*!< pointer to user callback */
    void *user_data;    /*!< user callback data */
};
typedef struct _lr_SingleCallbackData * lr_SingleCallbackData;

int
lr_progress_func(void* ptr,
                 double total_to_download,
                 double now_downloaded,
                 double total_to_upload,
                 double now_uploaded)
{
    double delta;
    double total_size = 0.0;
    lr_CallbackData cb_data = ptr;
    lr_SharedCallbackData scd_data = cb_data->scb_data;

    LR_UNUSED(total_to_upload);
    LR_UNUSED(now_uploaded);

    if (total_to_download) {
        if (!scd_data->counted[cb_data->id]) {
            int advertise = 1;
            scd_data->counted[cb_data->id] = 1;
            scd_data->total_size += total_to_download;
            for (int x = 0; x < scd_data->count; x++)
                if (scd_data->counted[x] == 0) advertise = 0;
            scd_data->advertise = advertise;
        }
    }


    delta = now_downloaded - cb_data->downloaded;
    cb_data->downloaded = now_downloaded;
    assert(delta >= 0.0);
    if (delta < 0.001)
        return 0;  /* Little step - Do not advertize */
    scd_data->downloaded += delta;
    total_size = scd_data->advertise ? scd_data->total_size : 0.0;

    /* Call user callback */
    DEBUGASSERT(scd_data->cb);
    return scd_data->cb(scd_data->user_data, total_size, scd_data->downloaded);
}

int
lr_single_progress_func(void* ptr,
                        double total_to_download,
                        double now_downloaded,
                        double total_to_upload,
                        double now_uploaded)
{
    lr_SingleCallbackData cb_data = ptr;

    LR_UNUSED(total_to_upload);
    LR_UNUSED(now_uploaded);
    return cb_data->cb(cb_data->user_data, total_to_download, now_downloaded);
}

/* End of callback stuff */

int
lr_curl_single_download_resume(lr_Handle handle,
                               const char *url,
                               int fd,
                               long long offset,
                               int use_cb)
{
    CURLcode c_rc = CURLE_OK;
    CURL *c_h = NULL;
    FILE *f = NULL;
    long status_code = 0;
    int ret = LRE_OK;
    int retries = 0;
    lr_SingleCallbackData cb_data = NULL;

    if (!url) {
        DPRINTF("%s: No url specified", __func__);
        return LRE_NOURL;
    }

    c_h = curl_easy_duphandle(handle->curl_handle);
    if (!c_h) {
        DPRINTF("%s: Cannot dup CURL handle\n", __func__);
        return LRE_CURLDUP;
    }

    c_rc = curl_easy_setopt(c_h, CURLOPT_URL, url);
    if (c_rc != CURLE_OK) {
        curl_easy_cleanup(c_h);
        handle->last_curl_error = c_rc;
        DPRINTF("%s: curl_easy_setopt: %s\n", __func__, curl_easy_strerror(c_rc));
        return LRE_CURL;
    }

    f = fdopen(dup(fd), "w");
    if (!f) {
        curl_easy_cleanup(c_h);
        DPRINTF("%s: fdopen: %s\n", __func__, strerror(errno));
        return LRE_IO;
    }

    c_rc = curl_easy_setopt(c_h, CURLOPT_WRITEDATA, f);
    if (c_rc != CURLE_OK) {
        curl_easy_cleanup(c_h);
        handle->last_curl_error = c_rc;
        fclose(f);
        DPRINTF("%s: curl_easy_setopt: %s\n", __func__, curl_easy_strerror(c_rc));
        return LRE_CURL;
    }

    /* Set offset for resume download if offset param is passed */
    if (offset != 0) {
        DPRINTF("%s: download resume offset: %lld\n", __func__, offset);
        if (offset == -1) {
            /* determine offset */
            fseek(f, 0L, SEEK_END);
            offset = (long long) ftell(f);
            DPRINTF("%s: determined offset for download resume: %lld\n",
                    __func__, offset);
        }

        c_rc = curl_easy_setopt(c_h, CURLOPT_RESUME_FROM_LARGE, (curl_off_t) offset);
        if (c_rc != CURLE_OK) {
            curl_easy_cleanup(c_h);
            handle->last_curl_error = c_rc;
            fclose(f);
            DPRINTF("%s: curl_easy_setopt: %s\n", __func__, curl_easy_strerror(c_rc));
            return LRE_CURL;
        }
    }

    /* Header cb */
//    curl_easy_setopt(c_h, CURLOPT_HEADERFUNCTION, header_cb);

    /* Use callback if desired */
    if (use_cb && handle->user_cb) {
        DPRINTF("%s: callback used\n", __func__);

        cb_data = lr_malloc0(sizeof(struct _lr_SingleCallbackData));
        cb_data->cb = handle->user_cb;
        cb_data->user_data = handle->user_data;

        curl_easy_setopt(c_h, CURLOPT_PROGRESSFUNCTION, lr_single_progress_func);
        curl_easy_setopt(c_h, CURLOPT_NOPROGRESS, 0);
        curl_easy_setopt(c_h, CURLOPT_PROGRESSDATA, cb_data);
     }


    /* Main downloading loop - loop because of retries */

    for (;;) {
        ret = LRE_OK;
        status_code = 0;

        c_rc = curl_easy_perform(c_h);

        if (c_rc != CURLE_OK && c_rc != CURLE_HTTP_RETURNED_ERROR) {
            ret = LRE_CURL;
            if ((c_rc == CURLE_OPERATION_TIMEDOUT) ||
                (c_rc == CURLE_COULDNT_RESOLVE_HOST) ||
                (c_rc == CURLE_COULDNT_RESOLVE_PROXY) ||
                (c_rc == CURLE_FTP_ACCEPT_TIMEOUT)) {
                /* Temporary error => retry */
                ret = LRE_TEMPORARYERR;
            }
            goto retry;
        }

        /* Even if CULRE_OK is returned we have to check status code */
        curl_easy_getinfo(c_h, CURLINFO_RESPONSE_CODE, &status_code);
        if (status_code) {
            char *effective_url = NULL;
            curl_easy_getinfo(c_h, CURLINFO_EFFECTIVE_URL, &effective_url);

            if (effective_url && !strncmp(effective_url, "http", 4)) {
                /* HTTP(S) */
                if (status_code == 200)
                    break; /* No error */
                if (offset && status_code == 206)
                    break; /* No error */

                ret = LRE_BADSTATUS;
                if ((status_code == 500) || /* Internal Server Error */
                    (status_code == 502) || /* Bad Gateway */
                    (status_code == 503) || /* Service Unavailable */
                    (status_code == 504)) { /* Gateway Timeout */
                    ret = LRE_TEMPORARYERR;
                }
            } else if (effective_url) {
                /* FTP */
                if (status_code/100 == 2)
                    break; /* No error */

                ret = LRE_BADSTATUS;
                if (status_code/100 == 4) {
                    /* This is typically when the FTP server only allows a certain
                     * amount of users and we are not one of them.  All 4xx codes
                     * are transient. */
                    ret = LRE_TEMPORARYERR;
                }
            } else {
                ret = LRE_BADSTATUS;
            }

            goto retry;
        }

       if (ret == LRE_OK && c_rc == CURLE_OK)
            /* Downloading was successfull */
            break;

retry:
        if (status_code) {
            handle->status_code = status_code;
            DPRINTF("%s: bad status code: %ld\n", __func__, status_code);
        }

        if (c_rc != CURLE_OK) {
            handle->last_curl_error = c_rc;
            DPRINTF("%s: curl_easy_perform: %s\n", __func__, curl_easy_strerror(c_rc));
        }

        /* Discart all downloaded data which were downloaded now (truncate) */
        /* The downloaded data are problably only server error message! */
        lseek(fd, (off_t) offset, SEEK_SET);
        ftruncate(fd, (off_t) offset);
        fseek(f, offset, SEEK_SET);

        if (ret != LRE_TEMPORARYERR)
            /* No temporary error - end */
            break;

        retries++;
        if (retries >= handle->retries)
            /* No more retries */
            break;

        /* Try download again */
        DPRINTF("%s: Temporary download error - trying again (%d)\n",
                __func__, retries);

        /* Sleep for 500ms before next try */
        usleep(500000);
    }

    fclose(f);
    curl_easy_cleanup(c_h);
    lr_free(cb_data);
    return ret;
}

int
lr_curl_single_mirrored_download_resume(lr_Handle handle,
                                        const char *path,
                                        int fd,
                                        lr_ChecksumType checksum_type,
                                        const char *checksum,
                                        long long offset,
                                        int use_cb)
{
    int rc;
    int mirrors;
    lr_InternalMirrorlist iml = handle->internal_mirrorlist;

    if (!iml)
        return LRE_NOURL;

    mirrors = lr_internalmirrorlist_len(handle->internal_mirrorlist);
    if (mirrors < 1)
        return LRE_NOURL;

    DPRINTF("%s: Downloading %s\n", __func__, path);

    for (int x=0; x < mirrors; x++) {
        char *full_url;
        char *url = lr_internalmirrorlist_get_url(iml, x);
        DPRINTF("%s: Trying mirror: %s\n", __func__, url);

        lseek(fd, 0, SEEK_SET);
        if (offset == 0)
            ftruncate(fd, 0);

        full_url = lr_pathconcat(url, path, NULL);
        rc = lr_curl_single_download_resume(handle, full_url, fd, offset, use_cb);
        lr_free(full_url);

        DPRINTF("%s: Download rc: %d (%s)\n", __func__, rc, lr_strerror(rc));

        if (rc == LRE_OK) {
            /* Download successful */
            DPRINTF("%s: Download successful\n", __func__);

            /* Check checksum */
            if (checksum && handle->checks & LR_CHECK_CHECKSUM
                &&  checksum_type != LR_CHECKSUM_UNKNOWN)
            {
                DPRINTF("%s: Checking checksum\n", __func__);
                lseek(fd, 0, SEEK_SET);
                if (lr_checksum_fd_cmp(checksum_type, fd, checksum)) {
                    DPRINTF("%s: Bad checksum\n", __func__);
                    rc = LRE_BADCHECKSUM;
                    if (offset) {
                        /* If download was successfull but checksum doesn't match
                         * In next run, do not try to resume download and download
                         * whole file instead*/
                        offset = 0;
                        /* Try againt this mirror, but this time download
                         * whole file (no resume) */
                        x--;
                    }
                    continue; /* Try next mirror */
                }
            }

            /* Store used mirror into the handler */
            if (handle->used_mirror)
                lr_free(handle->used_mirror);
            handle->used_mirror = lr_strdup(url);
            break;
        }
    }

    return rc;
}

int
lr_curl_multi_download(lr_Handle handle, lr_CurlTargetList targets)
{
    int not = lr_curltargetlist_len(targets);  /* Number Of Targets */
    int nom;            /* Number Of Mirrors */
    int ret = LRE_OK, last_ret = LRE_OK;
    long last_code = 0; // last error HTTP of FTP return code
    int failed_downloads = 0;
    CURL *curl_easy_interfaces[not];
    FILE *open_files[not];
    CURLMcode cm_rc;
    CURLM *cm_h = NULL; /* Curl Multi Handle */
    lr_InternalMirrorlist iml;

    struct _lr_SharedCallbackData shared_cb_data;
    struct _lr_CallbackData cb_data[not];

    assert(handle);

    iml = handle->internal_mirrorlist;
    if (!iml)
        return LRE_NOURL;

    nom = lr_internalmirrorlist_len(iml);
    if (nom < 1)
        return LRE_NOURL;

    if (not == 0) {
        /* Maybe user callback shoud be called here */
        return LRE_OK;
    }

    /* Initialize shared callback data */
    shared_cb_data.counted = lr_malloc0(sizeof(short) * not);
    shared_cb_data.count = not;
    shared_cb_data.advertise = 0;
    shared_cb_data.downloaded = 0;
    shared_cb_data.total_size = 0;
    shared_cb_data.cb = handle->user_cb;
    shared_cb_data.user_data = handle->user_data;

    /* --- Since here it shoud be safe to use "goto cleanup" --- */

    for (int i = 0; i < nom; i++) {
        int used = 0;
        char *mirror = lr_internalmirrorlist_get_url(iml, i);
        int still_running;  /* Number of still running downloads */
        CURLMsg *msg;  /* for picking up messages with the transfer status */
        int msgs_left; /* how many messages are left */

        DPRINTF("%s: Using mirror %s\n", __func__, mirror);

        failed_downloads = 0;

        cm_h = curl_multi_init();
        if (!cm_h) {
            ret = LRE_CURLDUP;
            goto cleanup;
        }

        DPRINTF("%s: CURL multi handle targets:\n", __func__);
        for (int x = 0; x < not; x++) {
            /*  Prepare curl_easy_handlers for every target (file) which
             *  haven't been already downloaded */
            FILE *f;
            char *url;
            CURL *c_h;
            CURLcode c_rc;
            lr_CurlTarget t = lr_curltargetlist_get(targets, x);

            curl_easy_interfaces[x] = NULL;
            open_files[x] = NULL;

            if (t->downloaded)
                /* Already downloaded */
                continue;

            c_h = curl_easy_duphandle(handle->curl_handle);
            if (!c_h) {
                ret = LRE_CURLDUP;
                DPRINTF("%s: Cannot dup CURL handle\n", __func__);
                goto cleanup;
            }
            curl_easy_interfaces[x] = c_h;

            lseek(t->fd, 0, SEEK_SET);
            ftruncate(t->fd, 0);
            f = fdopen(dup(t->fd), "w");
            if (!f) {
                ret = LRE_IO;
                DPRINTF("%s: Cannot dup fd %d\n", __func__, t->fd);
                goto cleanup;
            }
            open_files[x] = f;

            url = lr_pathconcat(mirror, t->path, NULL);
            DPRINTF("%s:  %s\n", __func__, url);

            c_rc = curl_easy_setopt(c_h, CURLOPT_URL, url);
            lr_free(url);
            if (c_rc != CURLE_OK) {
                handle->last_curl_error = c_rc;
                ret = LRE_CURLDUP;
                DPRINTF("%s: Cannot set CURLOPT_URL\n", __func__);
                goto cleanup;
            }

            c_rc = curl_easy_setopt(c_h, CURLOPT_WRITEDATA, f);
            if (c_rc != CURLE_OK) {
                handle->last_curl_error = c_rc;
                ret = LRE_CURLDUP;
                DPRINTF("%s: Cannot set CURLOPT_WRITEDATA\n", __func__);
                goto cleanup;
            }

            /* Prepare callback and its data */
            if (handle->user_cb) {
                lr_CallbackData data = &cb_data[x];
                data->id = x;
                data->downloaded = 0.0;
                data->scb_data = &shared_cb_data;
                c_rc = curl_easy_setopt(c_h, CURLOPT_PROGRESSFUNCTION, lr_progress_func);
                c_rc = curl_easy_setopt(c_h, CURLOPT_NOPROGRESS, 0);
                c_rc = curl_easy_setopt(c_h, CURLOPT_PROGRESSDATA, data);
            }

            cm_rc = curl_multi_add_handle(cm_h, c_h);
            if (cm_rc != CURLM_OK) {
                handle->last_curlm_error = cm_rc;
                ret = LRE_CURLM;
                DPRINTF("%s: Cannot add curl_easy hadle to multi handle\n", __func__);
                goto cleanup;
            }
            used++;
        }

        if (used == 0) {
            DPRINTF("%s: All files were downloaded\n", __func__);
            break;  /* Nothing to download */
        }

        /* Perform */
        DPRINTF("%s: curl_multi_perform\n", __func__);
        curl_multi_perform(cm_h, &still_running);

        do {
            int rc;
            int maxfd = -1;
            long curl_timeo = -1;
            fd_set fdread;
            fd_set fdwrite;
            fd_set fdexcep;
            struct timeval timeout;

            FD_ZERO(&fdread);
            FD_ZERO(&fdwrite);
            FD_ZERO(&fdexcep);

            /* Set timeout */
            timeout.tv_sec = 1;
            timeout.tv_usec = 0;

            curl_multi_timeout(cm_h, &curl_timeo);
            if (curl_timeo >= 0) {
                timeout.tv_sec = curl_timeo / 1000;
                if (timeout.tv_sec > 1)
                    timeout.tv_sec = 1;
                else
                    timeout.tv_usec = (curl_timeo % 1000) * 1000;
            }

            /* Get filedescriptors from the transfers */
            cm_rc = curl_multi_fdset(cm_h, &fdread, &fdwrite, &fdexcep, &maxfd);
            if (cm_rc != CURLM_OK) {
                DPRINTF("%s: curl_multi_fdset() error: %d\n", __func__, cm_rc);
                handle->last_curlm_error = cm_rc;
                ret = LRE_CURLM;
                goto cleanup;
            }

            rc = select(maxfd+1, &fdread, &fdwrite, &fdexcep, &timeout);

            switch (rc) {
            case -1:
                break; /* select() error */

            case 0:
            default:
                /* Timeout or readable/writeable sockets */
                curl_multi_perform(cm_h, &still_running);
                break;
            }
        } while (still_running);

        /* Close opened files */
        /* Closing must be here! In the next code, if download failed
         * we truncate the download file descriptor. Truncation doesn't take
         * the effect if we truncate the file descriptor first and after that
         * we close the FILE* stream (opened from that file descriptor).
         * In short: FILE* stream must be closed first. */
        for (int x = 0; x < not; x++) {
            if (open_files[x]) {
                fclose(open_files[x]);
                open_files[x] = NULL;
            }
        }

        /* Check download statuses */
        while ((msg = curl_multi_info_read(cm_h, &msgs_left))) {
            if (msg->msg == CURLMSG_DONE) {
                int failed = 0;
                int idx, found = 0;
                lr_CurlTarget t = NULL;

                /* Find out which handle this message is about */
                for (idx=0; idx < not; idx++) {
                    found = (msg->easy_handle == curl_easy_interfaces[idx]);
                    t = lr_curltargetlist_get(targets, idx);
                    if(found)
                        break;
                }

                DPRINTF("%s: Download status: %d (%s)\n", __func__, msg->data.result, t->path);

                if (msg->data.result == CURLE_OK) {
                    long code = 0; // HTTP or FTP code
                    curl_easy_getinfo (msg->easy_handle, CURLINFO_RESPONSE_CODE, &code);
                    //if (code == 200 && msg->data.result != CURLE_ABORTED_BY_CALLBACK) {
                    if (code && (code != 200 && code != 226)) {
                        DPRINTF("%s: Bad HTTP/FTP code: %ld\n", __func__, code);
                        failed = 1;
                        last_ret = LRE_BADSTATUS;
                        last_code = code;
                    }
                } else {
                    failed = 1;
                    last_ret = LRE_CURL;
                }

                /* Check checksum */
                if (!failed && handle->checks & LR_CHECK_CHECKSUM
                    && t->checksum && t->checksum_type != LR_CHECKSUM_UNKNOWN)
                {
                    DPRINTF("%s: Checking checksum\n", __func__);
                    lseek(t->fd, 0, SEEK_SET);
                    if (lr_checksum_fd_cmp(t->checksum_type, t->fd, t->checksum)) {
                        DPRINTF("%s: Bad checksum\n", __func__);
                        last_ret = LRE_BADCHECKSUM;
                        failed = 1;
                    }
                }

                if (failed) {
                    failed_downloads++;
                    /* Update total_to_download in  callback data */
                    shared_cb_data.counted[idx] = 0;
                    /* Seek and truncate the file */
                    lseek(t->fd, 0, SEEK_SET);
                    ftruncate(t->fd, 0);
                } else {
                    /* Succeeded */
                    t->downloaded = 1;
                }
            }
        }

        /* Cleanup */
        curl_multi_cleanup(cm_h);
        cm_h = NULL;
        for (int x = 0; x < not; x++) {
            if (curl_easy_interfaces[x]) {
                curl_easy_cleanup(curl_easy_interfaces[x]);
                curl_easy_interfaces[x] = NULL;
            }
       }

        if (failed_downloads == 0)
            break;

    } /* End of iteration over mirrors */

cleanup:
    DPRINTF("%s: Cleanup\n", __func__);

    if (failed_downloads != 0) {
        /* At least one file cannot be downloaded from any mirror */
        if (last_ret != LRE_OK)
            ret = last_ret;
        if (last_code != 0)
            handle->status_code = last_code;
    }

    if (cm_h)
        curl_multi_cleanup(cm_h);
    for (int x = 0; x < not; x++) {
        if (curl_easy_interfaces[x])
            curl_easy_cleanup(curl_easy_interfaces[x]);
        if (open_files[x])
            fclose(open_files[x]);
    }
    lr_free(shared_cb_data.counted);

    return ret;
}

