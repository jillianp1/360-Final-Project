/****************************************************************************
*                   KCW: mount root file system                             *
*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#include <ext2fs/ext2_fs.h>

#include <string.h>
#include <libgen.h>
#include <sys/stat.h>
#include <time.h>

#include "type.h"

extern MINODE *iget();

MINODE minode[NMINODE];
MINODE *root;
PROC   proc[NPROC], *running;

char gpath[128]; // global for tokenized components
char *name[64];  // assume at most 64 components in pathname
int   n;         // number of component strings

OFT oft[64];

int  fd, dev, rdev;
int  nblocks, ninodes, bmap, imap, iblk;
char line[128], cmd[32], pathname[128], old_file[128], new_file[128];
int mode, closefd; 
char string[128];
char src_file[128], dest_file[128];
MTABLE mountTable[NMOUNT];

#include "cd_ls_pwd.c"
#include "rmdir.c"
#include "alloc_dalloc.c"
#include "mkdir_creat.c"
#include "read_cat.c"
#include "link_unlink.c"
#include "symlink.c"
#include "open_close.c"
#include "write_cp.c"
#include "mount_umount.c"

int init()
{
  int i, j;
  MINODE *mip;
  PROC   *p;
  MTABLE *mpt;

  printf("init()\n");

  for (i=0; i<NMINODE; i++){
    mip = &minode[i];
    mip->dev = mip->ino = 0;
    mip->refCount = 0;
    mip->mounted = 0;
    mip->mptr = 0;
  }

  // //initialize ofts as free
  // for( i = 0; i<NOFT; i++)
  // {
  //     oft[i].refCount = 0; 
  // }

  //initialize PROCS
  for (i=0; i<NPROC; i++){
    p = &proc[i];
    p->pid = i;           // pid = 1, 2
    p->uid = p->gid = i;    // uid = 0: SUPER user
    p->cwd = 0;             // CWD of process
    for(int j = 0; j < NFD; j++)
    {
        //fd[] of every proc = 0
        p->fd[j] = 0; //all file descriptors are null 
    }
        
  }

  for (int i = 0; i < NMOUNT; i++) {
    mpt = &mountTable[i];
    mpt->dev = 0;
  }

  proc[0].next = &proc[1];
  proc[1].next = &proc[0];
  root = NULL;
}

// load root INODE and set root pointer to it
int mount_root()
{  
  printf("mount_root()\n");
  root = iget(dev, 2);
}

//switch to disk2
char *disk = "disk2";     // change this to YOUR virtual

int main(int argc, char *argv[ ])
{
  int ino;
  //int mode; 
  char buf[BLKSIZE];
  if (argc > 1) {
    disk = argv[1];
  }
  printf("checking EXT2 FS ....");
  if ((fd = open(disk, O_RDWR)) < 0){
    printf("open %s failed\n", disk);
    exit(1);
  }

  dev = rdev = fd;    // global dev same as this fd   

  /********** read super block  ****************/
  get_block(dev, 1, buf);
  sp = (SUPER *)buf;

  /* verify it's an ext2 file system ***********/
  if (sp->s_magic != 0xEF53){
      printf("magic = %x is not an ext2 filesystem\n", sp->s_magic);
      exit(1);
  }     
  printf("EXT2 FS OK\n");
  ninodes = sp->s_inodes_count;
  nblocks = sp->s_blocks_count;

  get_block(dev, 2, buf); 
  gp = (GD *)buf;

  bmap = gp->bg_block_bitmap;
  imap = gp->bg_inode_bitmap;
  iblk = gp->bg_inode_table;
  printf("bmp=%d imap=%d inode_start = %d\n", bmap, imap, iblk);

  init();  
  mount_root();
  printf("root refCount = %d\n", root->refCount);

  printf("creating P0 as running process\n");
  running = &proc[0];
  running->cwd = iget(dev, 2);
  printf("root refCount = %d\n", root->refCount);
  for (int i = 0; i < NFD; i++) {
    running->fd[i] = NULL;
  }
  // WRTIE code here to create P1 as a USER process
  
  while(1){
    memset(pathname, 0, sizeof(pathname));
    printf("input command : \n[ls|cd|pwd|quit|mkdir|creat|rmdir|link|unlink|symlink|open|close|pfd|cat|read|write] \n");
    fgets(line, 128, stdin);
    line[strlen(line)-1] = 0;

    if (line[0]==0)
       continue;
    pathname[0] = '\0';

    sscanf(line, "%s %s %d", cmd, pathname, &mode);
    printf("cmd=%s pathname=%s param=%d\n", cmd, pathname, mode);
    if (strcmp(cmd, "ls")==0) {
      my_ls(pathname);
    }
    else if (strcmp(cmd, "cd")==0)
       cd(pathname);
    else if (strcmp(cmd, "pwd")==0)
       pwd(running->cwd);
    else if (strcmp(cmd, "quit")==0)
       quit();
    else if (strcmp(cmd, "mkdir")==0)
        make_dir(pathname);
    else if (strcmp(cmd, "creat")== 0)
        creat_file(pathname);
    else if (strcmp(cmd, "link")== 0)
    {
        sscanf(line, "%s %s %s", cmd, old_file, new_file);
        printf("old file %s new file = %s\n", old_file, new_file);
        link_file(old_file, new_file);
    }
       // link_file(pathname);
    else if (strcmp(cmd, "unlink")== 0)
    {
        sscanf(line, "%s %s", cmd, pathname);
        my_unlink(pathname);
    }
        //my_unlink(pathname);
    else if(strcmp(cmd, "rmdir")==0)
    {
        my_rmdir(pathname);
    }
    else if(strcmp(cmd, "symlink")==0)
    {
      sscanf(line, "%s %s %s", cmd, old_file, new_file);
      printf("symlink old_file = %s newfile = %s\n", old_file, new_file);
      symlink_file(old_file, new_file);
    }
    else if(strcmp(cmd, "open")==0)
    {
      //sscanf(line, "%s %s %d", cmd, pathname, mode);
      printf("mode : %d\n", mode);
      open_file(pathname, mode);
    }
    else if(strcmp(cmd, "close")==0)
    {
      //int fd = -1; 
      sscanf(line, "%s %d", cmd, &fd);
      my_close(fd);
    }
    else if(strcmp(cmd, "pfd")==0)
      pfd();
    else if(strcmp(cmd, "write")==0)
    {
        write_file();
    }
    else if(strcmp(cmd, "read")==0)
    {
        read_file();
    }
    else if(strcmp(cmd, "cp")==0)
    {
        sscanf(line, "%s %s %s", cmd, src_file, dest_file);
        my_cp(src_file, dest_file);
    }
    else if(strcmp(cmd, "cat")==0)
    {
        my_cat(pathname);
    }
    else if (strcmp(cmd, "mount")== 0) {
      sscanf(line, "%s %s %s", cmd, src_file, dest_file);
      if (!strcmp(src_file, "")) {
        print_mount();
      }
      else{
        my_mount(src_file, dest_file);
      }
    }
  }
}

int quit()
{
  int i;
  MINODE *mip;
  for (i=0; i<NMINODE; i++){
    mip = &minode[i];
    if (mip->refCount > 0)
      iput(mip);
  }
  printf("see you later, alligator\n");
  exit(0);
}
