#define _LARGEFILE64_SOURCE
#include <string.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/sendfile.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
	long long wanted_lba = 0, lba = 0, sw = argc - 4, su =
	 128 * 1024, lstripe = 0;
	int disk[sw + 2], diskeofs = 0, diskeof[99] = { 0 }, d = 0;
	char zero[4096] = { 0 };
	char buf[sw + 2][su];
	if(argc<2) {
		fprintf(stderr,"usage: %s <location of keyfile> [ disk disk disk\nwhere the location of the keyfile is the according sector number from .key.bmap file which should exist
				oh fuck ops stopped making that file.. well good luck to them, then.. i cant help if they dont listen.. A) dont buy cards that randomly corrupt or lose your data B) if you do, then at least keep a key file there to reassemble them with ..   
				
	of course if there was a dis rebuildin, i might find two matches.. its tricky and so case by case reallly
				
				is a file you know is on the target disk somewhere that is at least two stripes long..without that it might be possible by looking at the filesystem structure, but that is not automated\n");
		exit(-1); }
	while (d < (sw + 2)) {
		disk[d] = open(argv[d + 1], O_RDONLY);
		//fprintf(stderr,"%d %d\n",d,disk[d]);
		d++;
	}
#define WANT atol(argv[argc-1])
  wanted_lba = WANT / 2 / 128; // might want info from .key.bmap here
  lba = WANT / 2 / 128 / sw / (sw + 2) * sw * (sw + 2);    // this is starting where the stripes start at 0 because if not starting at 0, you must read before you want so you get the whole stripe in case you need the parity
	lstripe = lba / sw;
	while (diskeofs < sw + 2) {
		int ret, alld;
		char bad[sw + 2];

		long long stripe;
		//long long vdoff=0x877c; // sdc
		off64_t o;
		stripe = lba / sw;
		if (stripe > lstripe) {
			unsigned long long i;
			unsigned o, pd, qd, td,bads=0;
			pd = ((sw + 2) - ((lstripe + 2) % (sw + 2))) % (sw + 2);	// lsi, it'd be 2* stripe for acrea
			qd = ((sw + 2) - ((lstripe + 1) % (sw + 2))) % (sw + 2);	// lsi, it'd be 2* stripe for acrea
			for (td = 0; td < sw + 2; td++) 
				bads+=((td!=qd)&&bad[td]);
			if(!bads) {
				unsigned long t;
				for (td = 0; td < sw + 2; td++) { // this would probably be faster if this loop was inside out, unlike the other, and we were just loading.. the other one, i'm not sure which way would be faster
					if (td == qd) continue;
					if (td == pd) continue;
					unsigned long o;
					t=0;
					for (o = 0; o < su / sizeof(unsigned long); o++)
						t+=(	((unsigned long *)(buf[pd]))[o] ^= ((unsigned long *)(buf[td]))[o]);
				}
				if(t)
					fprintf(stderr,"consistency check fail at stripe %llu\n",stripe);
			}
			for (i = lba - sw; i < lba; i++) {
//      d = ((lba % sw) + ((sw + 2) - ((2 * stripe) % (sw + 2)))) % (sw + 2); // areca
				d = ((i % sw) + ((sw + 2) - ((1 * lstripe) % (sw + 2)))) % (sw + 2);	// lsi
				if (bad[d]) {
					bzero(buf[d], su);
					for (td = 0; td < sw + 2; td++) {
						if (td == qd) continue;
						if (td == d) continue;
						if (bad[td])  {
							fprintf(stderr, "n-2 not supported, zeroing su (%lld bytes) at %lld from starting offset\n", su, i * su);
							bzero(buf[d], su); 
							break; }
						unsigned long o;
						for (o = 0; o < su / sizeof(unsigned long); o++)
							((unsigned long *)(buf[d]))[o] ^= ((unsigned long *)(buf[td]))[o];
					}
				}
				if (i < wanted_lba)
					continue;

				write(1, buf[d], su);
			}
			lstripe = stripe;

		}
//      fprintf(stderr,"%12lld %3lld %3d %4s\n",lba++,stripe,d,argv[d+1]); continue;
//      lseek64(disk[d],(stripe+19074+4)*128*1024,SEEK_SET);
		//awk '{lba=($1*8388607);stripe=int((lba/22));ld=lba%22;so=24-((2*stripe)%24);disk=(ld+so)%24;printf "Physical-0x877c %4s %09x Logical %09x disk %2d\n",$2,stripe,lba, disk}'
		o = stripe * su +
		 //0x877c*0x20000 // sdc on areca
//      0 // lsi sdd/e/f
		 //0x41000 // areca
//      0x80000 // areca
		 (51200000 * 2) / sw * 1024	// LSI sdc
		 ;
		for (d = 0; d < sw + 2; d++) {
			bad[d] = 0;
			ret = pread(disk[d], buf[d], su, o);
			if (ret == su) {
				if (diskeof[d]) {
					diskeof[d] = 0;
					diskeofs--;
				}
			} else {
				long long n = 0;
				if (!diskeof[d]) {
					diskeofs++;
					diskeof[d] = 1;
				}
				bad[d] = 1;
			}
		}
		lba += sw;
	}

}

/*

 Sun Jun 03, 2012 8:37 pm

for areca 1680 raid-6:

. the logical to physical order of the disks (which might be the same, IDK, my disks were shuffled anyway).

. the offset into the logical disks where your stripes start

. the logical virtual disk address to logical disk and address

stripe=lba/sw+offset;
disk=((lba%sw)+((sw+2)-((2*stripe)%(sw+2))))%(sw+2); 

where your LBA is in stripe units (ala, 128k, 32k..whatever you made the array with).. and sw is, say 10, if you have a 12 disk array.

the first VD, for me, was offset 512k into the physical disks.

I don't know how the parity was calculated, I just ignored that data.
*/
