/***************************************************************************
 *     Copyright (c) 2009, Broadcom Corporation
 *     All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed with the hope that it will be useful, but without
 * any warranty; without even the implied warranty of merchantability or
 * fitness for a particular purpose.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 ***************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mount.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <linux/fs.h>

#define APP_NOTIFY_PATH "/usr/local/bin/app_player_notify"

const char pgm[] = "/sbin/hotplug";

#ifndef MNT_FORCE
#  define MNT_FORCE	0x00000001	/* Attempt to forcibily umount */
#endif

#ifndef MNT_DETACH
#  define MNT_DETACH	0x00000002	/* Just detach from the tree */
#endif



/*
** This is an example of the relevant env vars.
**
** MINOR=1
** PHYSDEVBUS=scsi
** MAJOR=8
** ACTION=add
** SUBSYSTEM=block
** PHYSDEVDRIVER=sd
*/



/* So we can compare the output of this func using strcmp(...) */
const char *
getenv_safe(const char *var) {
    const char *p = getenv(var);
    if (!p) p = "";
    return p;
}


/* Convert a minor/major pair to a BRCM device path. */
int
majmin_to_paths(int major, int minor, int maxpathchars, char *devpath, char *mountpath)
{

    const char letter = "abcdefghijklmnop"[ minor / 16 ];
    const int number  = minor % 16;

    if (0 == number) {
        snprintf(devpath, maxpathchars, "/dev/sd%c", letter);
        snprintf(mountpath, maxpathchars, "/mnt/removable/usb%c", letter);
    } else {
        snprintf(devpath, maxpathchars, "/dev/sd%c%d", letter, number);
        snprintf(mountpath, maxpathchars, "/mnt/removable/usb%c", letter);
    }
    return 0;
}




int 
main(int argc, const char *argv[])
{
#   define DEVICE_PATH_SIZE 64
    int major, minor;
    const char *p0, *p1;
    char devpath[DEVICE_PATH_SIZE], mountpath[DEVICE_PATH_SIZE];
    struct stat st;

    /* Allow error output to be seen on the console */
    fclose(stderr);
    stderr  = fopen("/dev/console", "w");

   
    /* Right now, we only care about new scsi disk devices. */
/*
    mfi: 
    In 2624, "sd" doesn't come up with "block" and "scsi".
    What we're really looking for is "block" and "scsi", so 
    we have put the "sd" comparison under #if 0
*/    
#if 0   
    if (strcmp("sd",    getenv_safe("PHYSDEVDRIVER"))) return 0;
#endif

    if (strcmp("block", getenv_safe("SUBSYSTEM")))     return 0;
    if (strcmp("scsi",  getenv_safe("PHYSDEVBUS")))    return 0;



    /* Grok major and minor numbers of device */
    if ((p0 = getenv("MINOR")) && (p1 = getenv("MAJOR"))) {
        minor = atoi(p0);
        major = atoi(p1);
    } else {
        fprintf(stderr,"Cannot get env minor or major\n");
        return 0;
    }

    if (8 != major) { 
        fprintf(stderr,"8 != major , major=%d\n", major);
        return 0; 
    }

    if (majmin_to_paths(major, minor, DEVICE_PATH_SIZE, devpath, mountpath)) {
        fprintf(stderr,"majmin_to_paths returns !=0\n", major);
        return 0;
    }

    /* Hopefully, mountpath should already exist.  If not, try to create it */
    if (stat(mountpath, &st)) {
        if (mkdir(mountpath, 0744)) {
            return 1;
        }
    }


    if (0 == strcmp("add", getenv_safe("ACTION"))) {
        int i, mounted=0, rdonly_mounted=0;
        const char *fstype;
        const char *option;
        static const char *fstypes[] = { "vfat", "msdos", "ext3", "ext2" };
        static const char *options[] = { "shortname=winnt", 0, 0, 0 };

        for (i=0; i < sizeof(fstypes)/sizeof(fstypes[0]); i++) {
            fstype = fstypes[i];
            option = options[i];
            
            if (0 == mount(devpath, mountpath, fstype, (MS_NOATIME | MS_NODIRATIME | MS_DIRSYNC) , (const void *)option)) {
                mounted = 1;
                break;
            } else if (errno==EACCES || errno==EROFS) {
                if (0 == mount(devpath, mountpath, fstype, (MS_NOATIME | MS_NODIRATIME | MS_DIRSYNC | MS_RDONLY) , (const void *)option)) {
                    rdonly_mounted = 1;
                    break;
                }
            }
        }
        if (mounted || rdonly_mounted) {
            fprintf(stderr, "%s: mounted %s%s on %s as %s.\n", pgm, (rdonly_mounted?"read-only ":""), devpath, mountpath, fstype);
            if (0 == stat(APP_NOTIFY_PATH, &st)) {
                setenv("LD_LIBRARY_PATH", "/usr/local/lib:/lib:/usr/lib", 1);
                execl(APP_NOTIFY_PATH, APP_NOTIFY_PATH, "1", "mount", devpath, mountpath, (const char*)0);
            }
        } else {
            fprintf(stderr, "%s: failed to mount %s on %s.\n", pgm, devpath, mountpath);
        } 

    } else if (0 == strcmp("remove", getenv_safe("ACTION"))) {
        if (-1 == umount2(mountpath, MNT_DETACH|MNT_FORCE)) { 
            fprintf(stderr, "%s: failed to unmount %s from %s (errno=%d).\n", pgm, devpath, mountpath, errno);
        } else {
            fprintf(stderr, "%s: unmounted %s from %s.\n", pgm, devpath, mountpath);
            if (0 == stat(APP_NOTIFY_PATH, &st)) {
                setenv("LD_LIBRARY_PATH", "/usr/local/lib:/lib:/usr/lib", 1);
                execl(APP_NOTIFY_PATH, APP_NOTIFY_PATH, "2", "umount", "0", mountpath, (const char*)0);
            }
        }
  
    } else {
        return 0;
    }

    fclose(stderr);
    return 0;
}
