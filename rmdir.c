#ifndef __RMDIR_C__
#define __RMDIR_C__


int rm_child(MINODE* pmip, char* name) 
{
  char buf[BLKSIZE], temp[BLKSIZE];
  DIR *dp, *prevdp;
  char *cp;

  u16 mode = pmip->INODE.i_mode;
  INODE *ip = &pmip->INODE;
    // getting parent block to search 
  get_block(dev, pmip->INODE.i_block[0], buf);
  dp = (DIR *)buf;
  cp = buf;
  printf("%s\n", name);

  // search for name in parent 
  while (cp < buf + BLKSIZE)
  {
     strncpy(temp, dp->name, dp->name_len);
     temp[dp->name_len] = 0;
     printf("%s, %s\n", temp, name);
     if (!strncmp(temp, name, strlen(temp))) {
        // if child is first and only entry in data block
        if (cp == buf && cp + dp->rec_len == buf + BLKSIZE) 
        {
            //deallocate block 
            bdalloc(pmip->dev, ip->i_block[0]);
            put_block(pmip->dev, ip->i_block[0], buf);
            //fix the size of array to remove deleted entry space 
            ip->i_block[0] = 0;
            ip->i_size -= BLKSIZE;
            pmip->dirty = 1;
            return 0;
        }
        //child is last entry 
        else if (cp + dp->rec_len >= buf + BLKSIZE) 
        {
            //absorb rec len into the previous directory 
            prevdp->rec_len += dp->rec_len;
            //update and write back to disk 
            put_block(pmip->dev, ip->i_block[0], buf);
            pmip->dirty = 1;
            printf("in remove last\n");
            return 0;
        }
        else {
            // store the last directory information
            char *lastcp = buf + dp->rec_len;
            DIR *lastdp = (DIR*)lastcp;

            // loop until we get to the last directory
            // to update its rec len size because last directory holds info on entire block 
            while (lastcp + lastdp->rec_len < buf + BLKSIZE) {
                lastcp += lastdp->rec_len;
                lastdp = (DIR*)lastcp;
            }
            
            // updating the last directories rec len to absorb the rec len of what were deleting
            lastdp->rec_len += dp->rec_len;

            // start of block that we are removing 
            char *start = cp + dp->rec_len;
            // end of block we are removing 
            char *end = buf + BLKSIZE;

            memmove(cp, start, end - start);
            //write back to disk 
            put_block(pmip->dev, ip->i_block[0], buf);
            pmip->dirty = 1;
            return 0;
        }
     }
     
     cp += dp->rec_len;
     prevdp = dp;
     dp = (DIR *)cp;
  }
  return -1;
}

int my_rmdir(char *pathname) {
    //get inode pf pathname 
    int ino = getino(pathname);
    if (ino == -1) {
        printf("error: ino doesn't exist \n");
        return -1;
    }
    MINODE *mip = iget(dev, ino);

    //check minode is a dir type 
    if (!S_ISDIR(mip->INODE.i_mode)) {
        printf("error minode type not DIR \n");
        return -1;
    }
    // check minode is not busy 
    if (mip->refCount > 1) {
        printf("error current directory is busy \n");
        return -1;
    }
    // check direcotory to remove is empty 
    if (mip->INODE.i_links_count > 2) {
        printf("error current directory not empty \n");
        return -1;
    }

    char buf[BLKSIZE], name[BLKSIZE];
    get_block(dev, mip->INODE.i_block[0], buf);
    DIR *dp = (DIR *) buf;
    char *cp = buf;
    int count = 0;
    while (cp < buf + BLKSIZE)
     {
        //if you add the rec len multiple times then directory is not empty 
        cp += dp->rec_len;
        dp = (DIR *)cp;
        count++;
        if (count > 2) {
            printf("error current directory is not empty \n");
            return -1;
        }
    }

    strcpy(name, pathname);
    char* parent = dirname(name);
    int pino = getino(parent);
    MINODE *pmip = iget(mip->dev, pino); // create MINODE to make changes to INODE
    strcpy(name, pathname);

    //get name from parent block 
    char* child = basename(name);
    //enter into rm child function 
    rm_child(pmip, child);
    //decrease link count
    pmip->INODE.i_links_count--;
    pmip->INODE.i_atime = time(0L);
    pmip->INODE.i_ctime = time(0L);
    pmip->dirty = 1;
    //deallocate data block and inode 
    idalloc(mip->dev, mip->ino);
    iput(mip); // write back changes
    iput(pmip);
}
#endif