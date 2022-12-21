/************* mount_umount.c file **************/

#ifndef __MOUNTUMOUNT_C__
#define __MOUNTUMOUNT_C__

#include "mkdir_creat.c"
extern MTABLE mountTable[NMOUNT];

//display current mounted file systems
int print_mount(void)
{
    printf("Currently mounted file systems:\n");
    printf("Dev  Dev Name   Mount Name\n");
    for(int i = 0; i < NMOUNT; i++)
    {
        if(mountTable[i].dev != 0 )
        {
            printf("%d\t%s\t%s\n", mountTable[i].dev, mountTable[i].devName, mountTable[i].mntName);
        }
    }
    return 0; 
}

int my_mount(char *filesys, char *mount_point)
{
  MINODE *mip; 
  int imount = 0;
  // given virtual disk and mount_point a DIR pathname


  //check if file system is already mounted 
  //if already mounted reject
  for (int i = 0; i < NMOUNT; i++ )
  {
      if(mountTable[i].dev != 0)
      {
          if(!strcmp(filesys, mountTable[i].devName))
          {
              printf("rejected already mounted\n");
              return -1; 
          }
      }
  }

  for (int i = 0; i < NMOUNT; i++) {
    if (mountTable[i].dev == 0) {
        imount = i;
        break;
    }
  }

  if(imount == -1) {
        printf("Mount limit reached\n");
        return -1;
    }
  //if not allocate a free MOUNT table entry (dev = 0)
    MTABLE *mtp;
    mtp = &mountTable[imount];
    mtp->dev = 0;
    strcpy(mtp->devName, filesys);
    strcpy(mtp->mntName, mount_point);

    // open filesys virtual disk for RW; use file descriptor as new dev
    int fd = open(filesys, O_RDWR);
    if (fd < 0 || fd > 15) {
        printf("invalid fd, cannot open file: %s\n", filesys);
        return -1;
    }

    char buf[BLKSIZE];
    get_block(fd, 1, buf);
    SUPER *sp=(SUPER*)buf;
    if (sp->s_magic != 0xEF53) {
        return -1;
    }
    // find the ino, then the minode of mount_point
    int ino = getino(mount_point);
    printf("dest ino: %d\n", ino);
    mip = iget(running->cwd->dev, ino);

    // check if it is a directory
    if (!S_ISDIR(mip->INODE.i_mode)) {
        printf("not a directory, cannot be mounted\n");
        return -1;
    }

    // check if not busy
    if (mip->refCount > 2) {
        printf("directory is busy, cannot be mounted\n");
        return -1;
    }

    // record new dev and filesys name in the MOUNT table entry
    // store ninodes, nblocks, bmap, imap, and inodes start block
    mtp->dev = fd;
    mtp->ninodes = sp->s_inodes_count;
    mtp->nblocks = sp->s_blocks_count;
    // mtp->bmap = bmap;
    // mtp->imap = imap;
    // mtp->free_inodes = sp->s_free_inodes_count;
    mip->mounted = 1;
    mip->mptr = mtp;
    mtp->mntDirPtr = mip;
    printf("\n\nhere\n");
  return 1;
}

int my_umount(char *filesys)
{
    // search the MOUNT table to check filesys is indeed mounted
    int mounted = 0;
    int mdev = 0;
    MTABLE *mtp;
    for (int i = 0; i < NMOUNT; i++) {
        if (!strcmp(mountTable[i].devName, filesys) && mountTable[i].dev != 0) {
            // found mounted MTABLE
            mounted = 1;
            mdev = mountTable[i].dev;
            mtp = &mountTable[i];
            break;
        }
    }

    // check whether any file is active in the mounted filesys; if so reject
    for (int i = 0; i < NMINODE; i++) {
        if (minode[i].dev == mdev) {
            printf("cannot unmount, there are active files\n");
            return -1;
        }
    }

    // find the mount_point in-memory inode, should be in memory while mounted on.
    // reset the minode's mounted flag to 0, then input() the minode
    MINODE *mip = mtp->mntDirPtr;
    mip->mounted = 0;
    iput(mip);
    return 0;
}

#endif