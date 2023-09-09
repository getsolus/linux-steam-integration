/*
 * This file is part of linux-steam-integration.
 *
 * Copyright © 2016-2019 Solus
 * Copyright © 2018-2019 Intel Corporation
 *
 * linux-steam-integration is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 */

#define _GNU_SOURCE

#include <assert.h>
#include <errno.h>
#include <pwd.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "config.h"
#include "lsi.h"
#include "nica/hashmap.h"
#include "nica/inifile.h"

#include "../common/files.h"

/**
 * Configuration file suffix, i.e. ~/.config/linux-steam-integration.conf
 */
#define LSI_CONFIG_FILE "linux-steam-integration.conf"

/**
 * System config file, i.e. /etc/linux-steam-integration.conf
 */
#define LSI_SYSTEM_CONFIG_FILE SYSTEMCONFDIR "/" LSI_CONFIG_FILE

/**
 * Vendor configuration file, lowest priority.
 * /usr/share/defaults/linux-steam-integration/linux-steam-integration.conf
 */
#define LSI_VENDOR_CONFIG_FILE VENDORDIR "/ " LSI_CONFIG_FILE

/**
 * Build the user config file path
 */
static char *lsi_get_user_config_file(void)
{
        autofree(char) *dir = lsi_get_user_config_dir();
        char *c = NULL;
        if (asprintf(&c, "%s/%s", dir, LSI_CONFIG_FILE) < 0) {
                return NULL;
        }
        return c;
}

/**
 * A subset of values equate to a true boolean, everything else is false.
 */
static inline bool lsi_is_boolean_true(const char *compare)
{
        const char *yesses[] = { "yes", "true", "YES", "TRUE", "ON", "on" };
        if (!compare) {
                return false;
        }
        for (size_t i = 0; i < ARRAY_SIZE(yesses); i++) {
                if (streq(compare, yesses[i])) {
                        return true;
                }
        }
        return false;
}

bool lsi_config_load(LsiConfig *config)
{
        char *paths[] = { NULL, LSI_SYSTEM_CONFIG_FILE, LSI_VENDOR_CONFIG_FILE };

        autofree(NcHashmap) *mconfig = NULL;
        char *map_val = NULL;

        paths[0] = lsi_get_user_config_file();
        for (size_t i = 0; i < ARRAY_SIZE(paths); i++) {
                const char *filepath = (const char *)paths[i];
                if (!filepath) {
                        continue;
                }
                /* Don't bother loading non existant paths */
                if (!lsi_file_exists(filepath)) {
                        continue;
                }
                mconfig = nc_ini_file_parse(filepath);
                if (mconfig) {
                        break;
                }
        }
        free((char *)paths[0]);

        /* Found no valid INI config */
        if (!mconfig) {
                return false;
        }

        /* Populate defaults now */
        lsi_config_load_defaults(config);

        /* Determine if native runtime should be used */
        map_val = nc_hashmap_get(nc_hashmap_get(mconfig, "Steam"), "use-native-runtime");
        if (map_val) {
                config->use_native_runtime = lsi_is_boolean_true(map_val);
                map_val = NULL;
        }

        /* Do we want libintercept? */
        map_val = nc_hashmap_get(nc_hashmap_get(mconfig, "Steam"), "use-libintercept");
        if (map_val) {
                config->use_libintercept = lsi_is_boolean_true(map_val);
                map_val = NULL;
        }

        /* Do we want libredirect? */
        map_val = nc_hashmap_get(nc_hashmap_get(mconfig, "Steam"), "use-libredirect");
        if (map_val) {
                config->use_libredirect = lsi_is_boolean_true(map_val);
                map_val = NULL;
        }

        /* Do we want unity3d hack? */
        map_val = nc_hashmap_get(nc_hashmap_get(mconfig, "Steam"), "use-unity-hack");
        if (map_val) {
                config->use_unity_hack = lsi_is_boolean_true(map_val);
                map_val = NULL;
        }

        /* Check if 32-bit is being forced */
        map_val = nc_hashmap_get(nc_hashmap_get(mconfig, "Steam"), "force-32bit");
        if (map_val) {
                config->force_32 = lsi_is_boolean_true(map_val);
        }
        return true;
}

static inline const char *lsi_bool_to_string(bool b)
{
        if (b) {
                return "true";
        }
        return "false";
}

bool lsi_config_store(LsiConfig *config)
{
        autofree(FILE) *fp = NULL;
        autofree(char) *conf = NULL;
        autofree(char) *dir = NULL;

        if (!config) {
                return false;
        }

        /* Ensure we have config directory, i.e. when as a snap.. */
        dir = lsi_get_user_config_dir();
        if (!dir) {
                return false;
        }
        if (!lsi_file_exists(dir)) {
                if (mkdir(dir, 00755) < 0) {
                        return false;
                }
        }

        conf = lsi_get_user_config_file();
        if (!conf) {
                return false;
        }

        fp = fopen(conf, "w");
        if (!fp) {
                return false;
        }
        if (fprintf(fp,
                    "[Steam]\nuse-native-runtime = %s\nforce-32bit = %s\nuse-libintercept = "
                    "%s\nuse-libredirect = %s\nuse-unity-hack = %s\n",
                    lsi_bool_to_string(config->use_native_runtime),
                    lsi_bool_to_string(config->force_32),
                    lsi_bool_to_string(config->use_libintercept),
                    lsi_bool_to_string(config->use_libredirect),
                    lsi_bool_to_string(config->use_unity_hack)) < 0) {
                return false;
        }
        return true;
}

void lsi_config_load_defaults(LsiConfig *config)
{
        assert(config != NULL);

        /* Very simple right now, but in future we'll expand the options and
         * things that LSI knows about */
        config->force_32 = false;
        config->use_native_runtime = false;
        config->use_libintercept = true;
        config->use_libredirect = true;
        config->use_unity_hack = true;
}

void lsi_report_failure(const char *s, ...)
{
        autofree(char) *report = NULL;
        autofree(char) *emit = NULL;
        va_list va;
        va_start(va, s);
        if (vasprintf(&report, s, va) < 0) {
                fputs("Critical internal error\n", stderr);
                return;
        }
        va_end(va);

        /* Display not set, just go ahead and stderr it */
        if (!getenv("DISPLAY")) {
                goto stderr_log;
        }

        if (asprintf(&emit,
                     "zenity --title \"%s\" --icon-name='steam' --error --text=\"%s\"",
                     PACKAGE_NAME,
                     report) < 0) {
                goto stderr_log;
        }

        if (system(emit) != 0) {
                fprintf(stderr, "%s: Failed to launch Zenity: %s\n", PACKAGE_NAME, strerror(errno));
                goto stderr_log;
        }
        return;

stderr_log:
        fputs(PACKAGE_NAME " failure: \n\t", stderr);
        fprintf(stderr, "%s\n", report);
}

/*
 * Editor modelines  -  https://www.wireshark.org/tools/modelines.html
 *
 * Local variables:
 * c-basic-offset: 8
 * tab-width: 8
 * indent-tabs-mode: nil
 * End:
 *
 * vi: set shiftwidth=8 tabstop=8 expandtab:
 * :indentSize=8:tabSize=8:noTabs=true:
 */
