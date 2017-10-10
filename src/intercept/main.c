/*
 * This file is part of linux-steam-integration.
 *
 * Copyright © 2016-2017 Ikey Doherty <ikey@solus-project.com>
 *
 * linux-steam-integration is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 */

#define _GNU_SOURCE

#include <link.h>
#include <stdio.h>
#include <stdlib.h>

#include "../common.h"
#include "nica/nica.h"

/**
 * Is this a process we're actually interested in?
 */
static bool process_supported = false;

/**
 * Determine the basename'd process
 */
static inline char *get_process_name(void)
{
        autofree(char) *realp = NULL;
        char *basep = NULL;

        realp = realpath("/proc/self/exe", NULL);
        if (!realp) {
                return false;
        }

        basep = basename(realp);
        return strdup(basep);
}

/**
 * Find out if we're being executed by a process we actually need to override,
 * otherwise we'd not be loaded by rtld-audit
 */
static bool is_intercept_candidate(void)
{
        static const char *wanted_processes[] = {
                "html5app_steam",
                "opengl-program",
                "steam",
                "steamwebhelper",
        };
        autofree(char) *nom = NULL;

        nom = get_process_name();
        if (!nom) {
                return false;
        }

        for (size_t i = 0; i < ARRAY_SIZE(wanted_processes); i++) {
                if (streq(wanted_processes[i], nom)) {
                        fprintf(stderr,
                                "debug: loading Linux Steam Integration libintercept for %s\n",
                                wanted_processes[i]);
                        return true;
                }
        }
        return false;
}

/**
 * la_version is our main entry point and will only continue if our
 * process is explicitly Steam
 */
_nica_public_ unsigned int la_version(unsigned int supported_version)
{
        /* Unfortunately glibc will die if we tell it to skip us .. */
        process_supported = is_intercept_candidate();
        return supported_version;
}

/**
 * la_objsearch will allow us to blacklist certain LD_LIBRARY_PATH duplicate
 * libraries being loaded by the Steam client, such as the broken libSDL shipped
 * as a private vendored lib
 */
_nica_public_ char *la_objsearch(const char *name, __lsi_unused__ uintptr_t *cookie,
                                 __lsi_unused__ unsigned int flag)
{
        /* We don't know about this process, so have glibc do its thing as normal */
        if (!process_supported) {
                return (char *)name;
        }

        // Patterns we'll permit Steam to privately load
        static const char *steam_allowed[] = {
                /* general */
                "libicui18n.so",
                "libicuuc.so",
                "libavcodec.so.",
                "libavformat.so.",
                "libavresample.so.",
                "libavutil.so.",
                "libswscale.so.",

                /* core plugins */
                "chromehtml.so",
                "crashhandler.so",
                "filesystem_stdio.so",
                "friendsui.so",
                "gameoverlayrenderer.so",
                "gameoverlayui.so",
                "libaudio.so",
                "libicui18n.so",
                "libicuuc.so",
                "libmiles.so",
                "libopenvr_api.so",
                "liboverride.so",
                "libsteam.so",
                "libtier0_s.so",
                "libv8.so",
                "libvideo.so",
                "libvstdlib_s.so",
                "serverbrowser.so",
                "steamclient.so",
                "steamoverlayvulkanlayer.so",
                "steamservice.so",
                "steamui.so",
                "vgui2_s.so",

                /* big picture mode */
                "panorama",
                "libpangoft2-1.0.so",
                "libpango-1.0.so",
        };

        /* Find out if its a Steam private lib.. */
        if (name && strstr(name, "Steam/ubuntu12_")) {
                for (size_t i = 0; i < ARRAY_SIZE(steam_allowed); i++) {
                        if (strstr(name, steam_allowed[i])) {
                                return (char *)name;
                        }
                }
                fprintf(stderr, "\n\ndebug: override steam: %s\n\n", name);
                return NULL;
        }

        return (char *)name;
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
