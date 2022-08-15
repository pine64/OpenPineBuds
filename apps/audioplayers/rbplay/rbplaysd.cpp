/***************************************************************************
 *
 * Copyright 2015-2019 BES.
 * All rights reserved. All unpublished rights reserved.
 *
 * No part of this work may be used or reproduced in any form or by any
 * means, or stored in a database or retrieval system, without prior written
 * permission of BES.
 *
 * Use of this work is governed by a license granted by BES.
 * This work contains confidential and proprietary information of
 * BES. which is protected by copyright, trade secret,
 * trademark and other intellectual property rights.
 *
 ****************************************************************************/
/* rbplay source */
/* playback control & rockbox codec porting & codec thread */

#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <ctype.h>
#include <unistd.h>

#include "hal_trace.h"
#include "metadata.h"
#include "SDFileSystem.h"
#include "rbplaysd.h"
#include "utils.h"

#define SD_LABEL "sd"
#define PLAYLIST_PATH "/sd/Playlist"

static SDFileSystem *sdfs = NULL;

// TODO: remove ?
static playlist_item sd_curritem;

playlist_struct sd_playlist;

static void playlist_insert(playlist_item* item)
{
    int fd;
    fd = open(PLAYLIST_PATH, O_RDWR | O_CREAT);
    if (fd <= 0) {
        error("Playlist open fail");
        return;
    }

    lseek(fd, item->song_idx * sizeof(playlist_item), SEEK_SET);
    write(fd, item, sizeof(playlist_item));
    close(fd);
}

static bool sdcard_mount(void)
{
    if (sdfs) {
        info("SD card already mount");
        return true;
    }

    sdfs = new SDFileSystem(SD_LABEL);
    if (sdfs == NULL) {
        error("No memory for sd file system");
        return false;
    }

    DIR *d;
    d = opendir("/" SD_LABEL);

    if (!d) {
        warn("SD card mount error");
        return false;
    }

    closedir(d);
    return true;
}

static void app_rbplay_gen_playlist(playlist_struct *list)
{
    struct dirent *p;
    DIR *d;
    uint32_t total;
    int fd = 0;
    struct mp3entry current_id3;
    playlist_item* sd_curritem_p = &sd_curritem;

    memset(list,0x0,sizeof(playlist_struct));

    d = opendir("/" SD_LABEL);
    if (!d) {
        error("SD card open fail");
        return;
    }

    info("---------gen audio list---------");

    total = 0;
    while (p = readdir(d)) {
        if (probe_file_format(p->d_name) == AFMT_UNKNOWN)
            continue;

        memset(&sd_curritem,0x0,sizeof(playlist_item));
        sd_curritem.song_idx = total;

        sprintf(sd_curritem.file_path, "/" SD_LABEL "/%s", p->d_name);
        sprintf(sd_curritem.file_name, "%s", p->d_name);

        info("Adding music: %s", sd_curritem.file_path);

        fd = open(sd_curritem.file_path, O_RDONLY);

        if (fd <= 0) {
            error("File %s open error", p->d_name);
            break;
        }

        get_metadata(&current_id3, fd, sd_curritem.file_path);
        close(fd);

        if(current_id3.bitrate == 0 || current_id3.filesize == 0 || current_id3.length == 0)
            break;

        info("bits:%d, type:%d, freq:%d", current_id3.bitrate,
                current_id3.codectype, current_id3.frequency);

        sd_curritem_p->bitrate    = current_id3.bitrate;
        sd_curritem_p->codectype  = current_id3.codectype;
        sd_curritem_p->filesize   = current_id3.filesize;
        sd_curritem_p->length     = current_id3.length;
        sd_curritem_p->frequency  = current_id3.frequency;

#ifdef PARSER_DETAIL
        char *str;
        str = current_id3.title;
        if(str != NULL) {
            memset(sd_curritem_p->title,0x0,MP3_TITLE_LEN);
            memcpy(sd_curritem_p->title ,str,strlen(str)>MP3_TITLE_LEN?MP3_TITLE_LEN:strlen(str));
        }

        str = current_id3.artist;
        if(str != NULL) {
            memset(sd_curritem_p->artist,0x0,MP3_ARTIST_LEN);
            memcpy(sd_curritem_p->artist ,str,strlen(str)>MP3_ARTIST_LEN?MP3_ARTIST_LEN:strlen(str));
        }

        str = current_id3.album;
        if(str != NULL) {
            memset(sd_curritem_p->album,0x0,MP3_ALBUM_LEN);
            memcpy(sd_curritem_p->album ,str,strlen(str)>MP3_ALBUM_LEN?MP3_ALBUM_LEN:strlen(str));
        }

        str = current_id3.genre_string;
        if(str != NULL) {
            memset(sd_curritem_p->genre,0x0,MP3_GENRE_LEN);
            memcpy(sd_curritem_p->genre ,str,strlen(str)>MP3_GENRE_LEN?MP3_GENRE_LEN:strlen(str));
        }

        str = current_id3.composer;
        if(str != NULL) {
            memset(sd_curritem_p->composer,0x0,MP3_COMPOSER_LEN);
            memcpy(sd_curritem_p->composer ,str,strlen(str)>MP3_COMPOSER_LEN?MP3_COMPOSER_LEN:strlen(str));
        }
#endif
        playlist_insert(sd_curritem_p);
        total++;
    }

    list->total_songs = total ;
    list->current_item = sd_curritem_p;

    closedir(d);
    info("---------%d audio file searched---------" , total);

}

void app_rbplay_load_playlist(playlist_struct *list)
{
    if(sdcard_mount() == false)
        return;

    remove(PLAYLIST_PATH);

    app_rbplay_gen_playlist(list);
}

playlist_item *app_rbplay_get_playitem(const int idx)
{
    int fd;
    if(idx >= sd_playlist.total_songs) {
        warn("Index exceed: %d / %d", idx, sd_playlist.total_songs);
        return NULL;
    }

    fd = open(PLAYLIST_PATH, O_RDONLY);

    if (fd <= 0) {
        warn("SD card playlist can not open");
        return NULL;
    }

    lseek(fd, sizeof(playlist_item) * idx, SEEK_SET);
    read(fd, sd_playlist.current_item, sizeof(playlist_item));
    info("Get playitem: %d: %s", idx, sd_playlist.current_item->file_path);

    close(fd);

    return sd_playlist.current_item;
}

int app_ctl_remove_file(const int idx)
{
    playlist_item *item = app_rbplay_get_playitem(idx);
    if (!item)
        return -1;
    remove(item->file_path);

    return 0;
}


