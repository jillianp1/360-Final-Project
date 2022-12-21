#ifndef __CDLSPWD_C__
#define __CDLSPWD_C__
extern int dev;
/************* cd_ls_pwd.c file **************/
int cd(char* pathname)
{
   //printf("cd: under construction READ textbook!!!!\n");
   int ino = getino(pathname);
   //return error if ino = 0
   if(ino == 0)
   {
         printf("error in cd\n");
         return -1;
   }
   MINODE *mip = iget(dev, ino);

   //verify that mip->INODE is a DIR return error if not
   //S_ISDIR 
   //use S_ISDIR to see if dir type 
   //mip = &minode[i];
   if (!S_ISDIR(mip->INODE.i_mode))
   {
         printf("error minode type not DIR\n");
         return -1;
   }

   // release old cwd
   iput(running->cwd);
   //change cwd to mip
   running->cwd = mip;
   return -1;
}

int ls_file(MINODE *mip, char *name)
{
  // READ Chapter 11.7.3 HOW TO ls
  char *t1 = "xwrxwrxwr-------";
  char *t2 = "----------------";
  char ftime[64];
  u16 mode = mip->INODE.i_mode;
  if ((S_ISREG(mode)))
        printf("%c", '-');
    if ((S_ISDIR(mode)))
        printf("%c", 'd');
    if ((S_ISLNK(mode)))
        printf("%c", 'l');
    
    for (int i=8; i >= 0; i--) 
    {
        if (mode & (1 << i))
            printf("%c", t1[i]);
        else
            printf("%c", t2[i]);
    }
    printf("%4d ", mip->INODE.i_links_count);
    printf("%4d ", mip->INODE.i_gid);
    printf("%4d ", mip->INODE.i_uid);
    printf("%8d ", mip->INODE.i_size);

    //strcpy(ftime, ctime(&mip->INODE.i_ctime));
    time_t time = mip->INODE.i_atime;
    strcpy(ftime, ctime(&time));
    ftime[strlen(ftime)-1] = 0;
    printf("%s ", ftime);

    //printf("%s", basename(name));
    printf("%s", name);

    //char linkname[256];
    if ((S_ISLNK(mode))) {
        //printf(" -> %s", mip->INODE.i_mode);
        printf(" -> %s", (char *)mip->INODE.i_block);
    }
    printf("\t%d", mip->ino);
    printf("\n");
    return 0;
}

int ls_dir(MINODE *mip)
{
  printf("IN LS_DIR\n");

  char buf[BLKSIZE], temp[BLKSIZE];
  DIR *dp;
  char *cp;

  MINODE *mip1;

  u16 mode = mip->INODE.i_mode;

  get_block(dev, mip->INODE.i_block[0], buf);
  dp = (DIR *)buf;
  cp = buf;
  
  while (cp < buf + BLKSIZE)
  {
     strncpy(temp, dp->name, dp->name_len);
     temp[dp->name_len] = 0;

     mip1 = iget(dev, dp->inode);

     ls_file(mip1, temp);
     mip1->dirty = 1;

     cp += dp->rec_len;
     dp = (DIR *)cp;
    iput(mip1);
  }
  printf("\n");
  return 0;
}

int my_ls(char *pathname)
{ 
  // if not given pathname then call ls_dir on current working directory
  printf("ls %s\n", pathname);

  if (strcmp(pathname, "") == 0) 
  {
      ls_dir(running->cwd);
  }
  else 
  {
    printf("IN ELSE\n");
    int ino = getino(pathname);
    // if INODE cannot be found
    if (ino == -1) 
    {
      printf("INODE does not exist\n");
      return -1;
    }
    else 
    {
      // check if path is a directory or file
      MINODE *mip = iget(dev, ino);
        printf("CALLING LS_DIR\n");
        ls_dir(mip);
    }
   }
   return 0;
}
  
void *rpwd(MINODE *wd)
{
  if(wd == root)
  {
      return;
  }
  int myino;
  char myname[256];
  char buf[BLKSIZE];
  // from wd->INODE.i_block[0] get my_ino and parent ino
  // use findino()
  //takes block number and loads it into buf
  get_block(dev, wd->INODE.i_block[0], buf);

  //get parent_ino using getino with the myino you got from findino
  int parent_ino = findino(wd, &myino);

  MINODE *pip = iget(dev, parent_ino);

  // get name use find my name()
  findmyname(pip, myino, myname);

  // recursive call to rpwd(pip) with parent minode
  rpwd(pip);
  //release minode
  iput(pip);
  printf("/%s", myname);
  return; 
}

char *pwd(MINODE *wd)
{
   if(wd == root)
   {
      printf("/\n");
   }
   else 
   {
      rpwd(wd);
      printf("\n");
   }
}

#endif


