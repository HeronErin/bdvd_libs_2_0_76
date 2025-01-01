/*
 *  nandinfo.c
 *
 *  Copyright (C) 2008 Broadcom
 *
 * Overview:
 *   This utility show bad block for MTD type devices using a NAND
 */

#define _GNU_SOURCE
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <getopt.h>
#include <errno.h>
#include <asm/errno.h>
#include <asm/types.h>
#include "mtd/mtd-user.h"

#define PROGRAM "nandinfo"
#define VERSION "$Revision: 1.00 $"


/*
    #define MTD_CLEAR_BITS		1       // Bits can be cleared (flash)
    #define MTD_SET_BITS		2       // Bits can be set
    #define MTD_ERASEABLE		4       // Has an erase function
    #define MTD_WRITEB_WRITEABLE	8       // Direct IO is possible
    #define MTD_VOLATILE		16      // Set for RAMs
    #define MTD_XIP			32	// eXecute-In-Place possible
    #define MTD_OOB			64	// Out-of-band data (NAND flash)
    #define MTD_ECC			128	// Device capable of automatic ECC
    #define MTD_NO_VIRTBLOCKS	256	// Virtual blocks not allowed
    #define MTD_PROGRAM_REGIONS	512	// Configurable Programming Regions
*/

static char* STR_MTD_CLEAR_BITS = "\nBits can be cleared";
static char* STR_MTD_SET_BITS   = "\nBits can be set";
static char* STR_MTD_ERASEABLE  = "\nHas an erase function ";
static char* STR_MTD_WRITEB_WRITEABLE = "\nDirect IO is possible ";
static char* STR_MTD_VOLATILE         = "\nSet for RAMs ";
static char* STR_MTD_XIP              = "\neXecute-In-Place possible ";
static char* STR_MTD_OOB              = "\nOut-of-band data (NAND flash) ";
static char* STR_MTD_ECC              = "\nDevice capable of automatic ECC ";
static char* STR_MTD_NO_VIRTBLOCKS    = "\nVirtual blocks not allowed ";
static char* STR_MTD_PROGRAM_REGIONS  = "\nConfigurable Programming Regions ";

// Global
char  *mtd_device;
int    advanced_info = 0;
char   nandinfo_str[400];

void display_help (void)
{
    printf("Usage: nandinfo -h for help\n"
           "Usage: nandinfo -v for version info.\n"
           "Usage: nandinfo -d for detailed informations\n"
           "Usage: nandinfo:\n"
           "Gives info on MTD devices using NAND.\n" );
    exit(0);
}

void display_version (void)
{
    printf(PROGRAM " " VERSION "\n"
           "\n"
           "Copyright (C) 2008 Broadcom Inc.\n"
           "\n"
           PROGRAM " comes with NO WARRANTY\n"
           "to the extent permitted by law.\n"
           "\n"
           "You may redistribute copies of " PROGRAM "\n"
           "under the terms of the GNU General Public Licence.\n"
           "See the file `COPYING' for more information.\n");
    exit(0);
}


static char* itoa(register int i)
{
	static char a[7]; /* Max 7 ints */
	register char *b = a + sizeof(a) - 1;
	int   sign = (i < 0);

	if (sign)
		i = -i;
	*b = 0;
	do
	{
		*--b = '0' + (i % 10);
		i /= 10;
	}
	while (i);
	if (sign)
		*--b = '-';
	return b;
}

void process_options (int argc, char *argv[])
{
    int error = 0;

    for (;;) {
        int c = getopt(argc, argv, "dhv?");
        if (c == EOF) {
            break;
        }

        switch (c) {
         case '?':
            display_help();
            break;
         case 'h':
            display_help();
            break;
         case 'v':
            display_version();
            break;
         case 'd':
            advanced_info = 1;
            break;
        }
    }

}

static char *meminfo_get_type(uint8_t type)
{
    if(type == MTD_ABSENT) return("No Chip Present");
    if(type == MTD_RAM) return("RAM");
    if(type == MTD_ROM) return("ROM");
    if(type == MTD_NORFLASH) return("NOR FLASH");
    if(type == MTD_NANDFLASH) return("NAND FLASH");
    if(type == MTD_PEROM) return("PEROM");
    if(type == MTD_DATAFLASH) return("DATA FLASH");
    if(type == MTD_BLOCK) return("BLOCK");
    if(type == MTD_OTHER) return("OTHER");
    if(type == MTD_UNKNOWN) return("UNKNOWN");
}

static char *meminfo_get_flags(uint32_t type)
{

    nandinfo_str[0] = 0;

    if(type & MTD_CLEAR_BITS)
    { 
        strcat(nandinfo_str, STR_MTD_SET_BITS);
    }
    if(type & MTD_SET_BITS)
    {
         strcat(nandinfo_str, STR_MTD_SET_BITS);
    }
    if(type & MTD_SET_BITS)
    {
         strcat(nandinfo_str, STR_MTD_SET_BITS);
    }
    if(type & MTD_ERASEABLE)
    {
         strcat(nandinfo_str, STR_MTD_ERASEABLE);
    }
    if(type & MTD_WRITEB_WRITEABLE)
    {
         strcat(nandinfo_str, STR_MTD_WRITEB_WRITEABLE);
    }
    if(type & MTD_VOLATILE)
    {
         strcat(nandinfo_str, STR_MTD_VOLATILE);
    }
    if(type & MTD_OOB)
    { 
         strcat(nandinfo_str, STR_MTD_OOB);
    }
    if(type & MTD_ECC)
    {
         strcat(nandinfo_str, STR_MTD_ECC);
    }
    if(type & MTD_NO_VIRTBLOCKS)
    {
         strcat(nandinfo_str, STR_MTD_NO_VIRTBLOCKS);
    }
    if(type & MTD_PROGRAM_REGIONS)
    {
         strcat(nandinfo_str, STR_MTD_PROGRAM_REGIONS);
    }

    return(nandinfo_str);
}

static char *meminfo_get_ecctype(uint32_t type)
{
    if(type == MTD_NANDECC_OFF) return("ECC Off");
    else if(type == MTD_NANDECC_PLACE) return("YAFFS2 Legacy Mode Placement");
    else if(type == MTD_NANDECC_AUTOPLACE) return("Default Placement Scheme");
    else if(type == MTD_NANDECC_PLACEONLY) return("Use Given Placement Scheme");
    else if(type == MTD_NANDECC_AUTOPL_USR) return("Use Given Autoplacement Scheme");
	else 
		return("Unknown ecctype! Value of type is: %d!", type);
	
}


/*
 * Main program
 */
int main(int argc, char **argv)
{

    FILE  *fdmtd;

    int    fd;
    int    badblockfound = 0;
    int    ret;
    int    i;
    int    mtdNumberOfOccurence = 0;
    int    mtdoffset = 0;
    int    blockalign = 1; 
    int    blockstart = -1;

    struct mtd_info_user meminfo;

    loff_t offs;

    char   str[128];    
    char   mtdAsciiNumber[3];
    char  *mtdAsciiReturnedNumber;

    process_options(argc, argv);

    // See if we have NAND hardware!
    // First find out if a NAND chip is used by opening /proc/hwinfo
    if ((fdmtd = fopen("/proc/hwinfo", "r")) == -1) 
    {
        perror("nandinfo: Unable to open /proc/hwinfo");
        exit(1);
    }

    // Read the file /proc/hwinfo
    // Read the file and look for the "NAND" string
    while(!feof(fdmtd))
    {
        if(fgets(str,126,fdmtd))
        {
            // printf("%s", str);
            if(strstr(str,"Flash Type:") != NULL)
            {
                if(strstr(str,"NAND") == NULL)
                {
                    printf("nandinfo: Flash Type is not NAND!");
                }
            }
        }
                        
    }

    // Second, find out how many partitions we have 
    // Try to open /proc/mtd
    if ((fdmtd = fopen("/proc/mtd", "r")) == -1) 
    {
        perror("nandinfo: Unable to open /proc/mtd");
        exit(1);
    }

    // Read the file /proc/mtd
    while(!feof(fdmtd))
    {
        if(fgets(str,126,fdmtd))
        {
            // printf("%s", str);
            // Count the number of occurence of "mtd" on each line...
            if(strstr(str,"mtd") != NULL)
            {
                 mtdNumberOfOccurence++;
            }
        }
                        
    }

    printf("\nJR+2.6.24-There is %d MTD type partitions\n", mtdNumberOfOccurence);

        
    if(mtdNumberOfOccurence != 0)
    {

        // Now pass each MTD partitions one by one!
        for (i = 0; i < mtdNumberOfOccurence; i++)
        {

            // Initialize some counters
            mtdoffset = 0;
            blockstart = -1;  
            offs = 0; 
            badblockfound = 0; 

             /* Open the MTDx device */
            str[0] = 0;

            strcat(str, "/dev/mtd");

            mtdAsciiReturnedNumber = itoa(i);

            strcat(str, mtdAsciiReturnedNumber);        

            if ((fd = open(str, O_RDONLY)) == -1) 
            {
                printf("trying to open %s\n", str);
                perror("nandinfo: open flash");
                exit(1);
            }

            /* Fill in MTD device capability structure */
            if (ioctl(fd, MEMGETINFO, &meminfo) != 0) 
            {
                perror("nandinfo: MEMGETINFO");
                close(fd);
                exit(1);
            }

            // Let's print out what we know up to now
            if(advanced_info == 1)
            {
                printf("Detailed MEMINFO Structure Values for %s: Size: 0x%08X (%09d)\n", str, meminfo.size, meminfo.size);
                printf("-------------------------------------------------------------------------------\n");
                printf("type:                  0x%08X: %s\n", meminfo.type, meminfo_get_type(meminfo.type));
                printf("flags:                 0x%08X: %s\n", meminfo.flags, meminfo_get_flags(meminfo.flags));
                printf("Erase size:            0x%08X (%9d)\n", meminfo.erasesize, meminfo.erasesize);
                printf("Page Size (writesize): 0x%08X (%9d)\n", meminfo.oobblock, meminfo.oobblock);
                printf("oobsize:               0x%08X (%9d)\n", meminfo.oobsize, meminfo.oobsize);
                //printf("ecc type:              0x%08X: %s\n", meminfo.ecctype, meminfo_get_ecctype(meminfo.ecctype));
                //printf("ecc size:              0x%08X (%9d)\n", meminfo.eccsize, meminfo.eccsize);
            }
            else
            {
                printf("Partition: %s Size: 0x%08X (%09d)\n", str, meminfo.size, meminfo.size);
            }

            //printf("Looking for Bad Block in %s:\n", str);

            /* Make sure device page sizes are valid */
            if (!(meminfo.oobsize == 16 && meminfo.oobblock == 512) &&
                !(meminfo.oobsize == 8 && meminfo.oobblock == 256) &&
                !(meminfo.oobsize == 64 && meminfo.oobblock == 2048)) 
            {
                  fprintf(stderr, "nandinfo: Unknown flash (not normal NAND)\n");
                  close(fd);
                  exit(1);
            }

            // Now find out those bad blocks!
            while (mtdoffset < meminfo.size)
            {

                while (blockstart != (mtdoffset & (~meminfo.erasesize + 1))) 
                {
                    blockstart = mtdoffset & (~meminfo.erasesize + 1);
                    offs = blockstart;

                    //printf("nandinfo address: %x end while condition: %x\n",(int) offs, blockstart + meminfo.erasesize);

                    if ((ret = ioctl(fd, MEMGETBADBLOCK, &offs)) < 0) 
                    {
                        perror("nandinfo: ioctl(MEMGETBADBLOCK)");
                        goto closeall;
                    }

                    if (ret == 1) 
                    {
                        badblockfound = 1;
                        printf("Bad block offset %x\n", (int) offs);
                    }


                }

                //Increment mtdOffset one block:
                mtdoffset = blockstart + meminfo.erasesize;        

            }

closeall:
            close(fd);

             if(badblockfound == 0) printf("No bad Block Found\n");
            //printf("nandinfo END\n");
        }
    }
    else
    {
        printf("No MTD Partitions found!!!\n");
    }
    /* Return happy */
    return 0;
}

