/*********** util.c file ****************/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>
#include <time.h>

#include "type.h"

/**** globals defined in main.c file ****/
extern MINODE minode[NMINODE];
extern MINODE *root;
extern PROC proc[NPROC], *running;
extern MTABLE mountTable[NMOUNT];
extern char gpath[128];
extern char *name[64];
extern int n;

extern int fd, dev, rdev;
extern int nblocks, ninodes, bmap, imap, iblk;

extern char line[128], cmd[32], pathname[128];

int get_block(int dev, int blk, char *buf)
{
   lseek(dev, (long)blk * BLKSIZE, 0);
   read(dev, buf, BLKSIZE);
}

int put_block(int dev, int blk, char *buf)
{
   lseek(dev, (long)blk * BLKSIZE, 0);
   int n = write(dev, buf, BLKSIZE);
   if (n != BLKSIZE)
   {
      printf("put_block error %d\n", blk);
   }
}

int tokenize(char *pathname)
{
   int i;
   char *s;
   printf("tokenize %s\n", pathname);

   strcpy(gpath, pathname); // tokens are in global gpath[ ]
   n = 0;

   s = strtok(gpath, "/");
   while (s)
   {
      name[n] = s;
      n++;
      s = strtok(0, "/");
   }
   name[n] = 0;

   for (i = 0; i < n; i++)
      printf("%s  ", name[i]);
   printf("\n");
}

MINODE *mialloc() {
   for (int i = 0; i < NMINODE; i++) {
      MINODE *mp = &minode[i];
      if (mp->refCount == 0) {
         mp->refCount = 1;
         return mp;
      }
   }
   return 0;
}

// return minode pointer to loaded INODE
MINODE *iget(int dev, int ino)
{
   int i;
   MINODE *mip;
   char buf[BLKSIZE];
   int blk, offset;
   INODE *ip;

   for (i = 0; i < NMINODE; i++)
   {
      mip = &minode[i];
      if (mip->refCount && mip->dev == dev && mip->ino == ino)
      {
         mip->refCount++;
         printf("found [%d %d] as minode[%d] in core\n", dev, ino, i);
         return mip;
      }
   }
   
   mip = mialloc();
   mip->dev = dev;
   mip->ino = ino;

   // get INODE of ino into buf[ ]
   blk = (ino - 1) / 8 + iblk;
   offset = (ino - 1) % 8;

   // printf("iget: ino=%d blk=%d offset=%d\n", ino, blk, offset);

   get_block(dev, blk, buf);   // buf[ ] contains this INODE
   ip = (INODE *)buf + offset; // this INODE in buf[ ]
   // copy INODE to mp->INODE
   mip->INODE = *ip;

   mip->refCount = 1;
   mip->mounted = 0;
   mip->dirty = 0;
   mip->mptr = 0;
   return mip;
}

void iput(MINODE *mip) // iput(): release a minode
{
   int i, block, offset, iblock;
   char buf[BLKSIZE];
   INODE *ip;

   if (mip == 0)
      return;

   mip->refCount--; // decreases refCount by 1

   if (mip->refCount > 0)
      return; // still has user
   // if (!mip->dirty)       return;
   if (mip->dirty == 0)
      return; // no need to write back

   /* write INODE back to disk */
   /**************** NOTE ******************************
    For mountroot, we never MODIFY any loaded INODE
                  so no need to write it back
   FOR LATER WorK: MUST write INODE back to disk if refCount==0 && DIRTY

   Write YOUR code here to write INODE back to disk
   *****************************************************/
   block = (mip->ino - 1) / 8 + iblock;
   offset = (mip->ino - 1) % 8;

   // get block containing this inode
   get_block(mip->dev, block, buf);
   ip = (INODE *)buf + offset;      // ip points at INODE
   *ip = mip->INODE;                // copy INODE to inode in block
   put_block(mip->dev, block, buf); // write back to disk

   //idalloc(mip->dev, mip->ino); // mip->refCount = 0
}

int search(MINODE *mip, char *name)
{
   int i;
   char *cp, c, sbuf[BLKSIZE], temp[256];
   DIR *dp;
   INODE *ip;

   printf("search for %s in MINODE = [%d, %d]\n", name, mip->dev, mip->ino);
   ip = &(mip->INODE);

   /*** search for name in mip's data blocks: ASSUME i_block[0] ONLY ***/

   get_block(mip->dev, mip->INODE.i_block[0], sbuf);
   dp = (DIR *)sbuf;
   cp = sbuf;
   printf("  ino   rlen  nlen  name\n");

   while (cp < sbuf + BLKSIZE)
   {
      strncpy(temp, dp->name, dp->name_len); // dp->name is NOT a string
      temp[dp->name_len] = 0;                // temp is a STRING
      printf("%4d  %4d  %4d    %s\n",
             dp->inode, dp->rec_len, dp->name_len, temp); // print temp !!!

      if (strcmp(name, temp) == 0)
      { // compare name with temp !!!
         printf("found %s : ino = %d\n", name, dp->inode);
         return dp->inode;
      }

      cp += dp->rec_len;
      dp = (DIR *)cp;
   }
   return 0;
}

int getino(char *pathname) // return ino of pathname
{
   int i, ino, blk, offset;
   char buf[BLKSIZE];
   INODE *ip;
   MINODE *mip;

   printf("getino: pathname=%s\n", pathname);
   if (strcmp(pathname, "/") == 0)
      return 2;

   // starting mip = root OR CWD
   if (pathname[0] == '/') {
      //mip = root;
      dev = root->dev;
      ino = root->ino;
   }
   else {
      //mip = running->cwd;
      dev = running->cwd->dev;
      ino = running->cwd->ino;
   }

   mip = iget(dev, ino);
   tokenize(pathname);

   for (i = 0; i < n; i++)
   {
      printf("===========================================\n");
      printf("getino: i=%d name[%d]=%s\n", i, i, name[i]);

      ino = search(mip, name[i]);

      if (ino == 0)
      {
         iput(mip);
         printf("name %s does not exist\n", name[i]);
         return 0;
      }
      // upwards traversal. locate mount table entry
      // using dev number. 
      else if(ino == 2 && dev != rdev) {
         for(int i = 0; i < NMOUNT; i++) {
            if (mountTable[i].dev == dev) {
               iput(mip);
               mip = mountTable[i].mntDirPtr;
               dev = mip->dev;
               break;
            }
         }
      }
      else {
         mip->dirty = 1;
         iput(mip);
         mip = iget(dev, ino);
         if (mip->mounted) {
            ino = 2;
            MTABLE *mtp = mip->mptr;
            dev = mtp->dev;
            iput(mip);
            mip = iget(dev, ino);
         }
      }

      iput(mip);
      mip = iget(dev, ino);
   }
   
   iput(mip);
   return ino;
}

// These 2 functions are needed for pwd()
int findmyname(MINODE *parent, u32 myino, char myname[])
{
   // WRITE YOUR code here
   // search parent's data block for myino; SAME as search() but by myino
   // copy its name STRING to myname[ ]

   // this should be the same as search but return name not node at the end

   int i;
   char *cp, sbuf[BLKSIZE], temp[256];
   DIR *dp;

   MINODE *mip = parent;

   // rlen = 12
   // search blocks
   for (i = 0; i < 12; i++)
   {
      if (mip->INODE.i_block[i] == 0)
      {
         return -1;
      }
      // takes block number loads it into buf
      get_block(mip->dev, mip->INODE.i_block[0], sbuf);
      dp = (DIR *)sbuf;
      cp = sbuf;
      while (cp < sbuf + BLKSIZE)
      {
         strncpy(temp, dp->name, dp->name_len); // dp->name is NOT a string
         temp[dp->name_len] = 0;                // temp is a STRING
         // check if you found the right node
         if (dp->inode == myino)
         {
            // copy dp->name into myname using length of dp for num characters
            strncpy(myname, dp->name, dp->name_len);
            // copy name size
            myname[dp->name_len] = 0;
            return 0;
         }
         cp += dp->rec_len;
         dp = (DIR *)cp;
      }
   }

   return -1;
}

int findino(MINODE *mip, u32 *myino) // myino = i# of . return i# of ..
{
   // mip points at a DIR minode
   // WRITE your code here: myino = ino of .  return ino of ..
   // all in i_block[0] of this DIR INODE.

   char buf[BLKSIZE];
   char *temp;
   DIR *dp;
   // takes block number loads it into buf, reads block
   get_block(mip->dev, mip->INODE.i_block[0], buf);
   temp = buf;
   // use buf to get inode of .
   dp = (DIR *)buf;
   *myino = dp->inode;
   // iterate by length of directory
   temp += dp->rec_len;
   // gets inode of ..
   dp = (DIR *)temp;
   return dp->inode;
}