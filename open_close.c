/************* open_close_lseek.c file **************/

#ifndef __OPENCLOSE_C__
#define __OPENCLOSE_C__

#include "mkdir_creat.c"

int truncate(MINODE *mip)
{
    //printf("in truncate\n");
    char buf[BLKSIZE];
    INODE *ip = &mip->INODE;

    //release mip->INODE data blocks 
    //file may have 12 direct blocks 
    for(int i = 0; i < 12; i++ )
    {
        if(ip->i_block[i] == 0)
        {
            break;
        }
        //deallocating block
        bdalloc(mip->dev, ip->i_block[i]);
        ip->i_block[i] = 0; 
    }

    //256 indirect blocks
    if(ip->i_block[12] != NULL)
    {
        get_block(mip->dev, ip->i_block[12], buf);
        int *indirect = (int *)buf;

        int indirect_count = 0; 
        while(indirect_count < BLKSIZE / sizeof(int))
        {
            if(indirect[indirect_count]==0)
                break;
            //deallocate the indirect blocks
            bdalloc(mip->dev, indirect[indirect_count]);
            indirect[indirect_count] = 0; 
            indirect_count++;
        }
        // deallocate reference to indirect
        bdalloc(mip->dev, ip->i_block[12]);
        ip->i_block[12] = 0; 
    }
    //doubly indirect
    if(ip->i_block[13]!= NULL)
    {
        get_block(mip->dev, ip->i_block[13], buf);
        int *doubly_indirect = (int *)buf;
        int doubly_indirect_count = 0; 

        while(doubly_indirect_count < BLKSIZE / sizeof(int))
        {
            if(doubly_indirect[doubly_indirect_count] == 0)
                break;
            
            //deallocate
            bdalloc(mip->dev, doubly_indirect[doubly_indirect_count]);
            doubly_indirect[doubly_indirect_count] = 0; 
            doubly_indirect_count++;
        }
        bdalloc(mip->dev, ip->i_block[13]);
        ip->i_block[13] = 0; 
    }

    //update inodes time field 
    mip->INODE.i_atime = mip->INODE.i_mtime = time(0L);
    //set INODES size to 0 mark mindode[] dirty 
    mip->INODE.i_size = 0; 
    mip->dirty = 1; 
    iput(mip);

}


int open_file(char *pathname, int mode)
{
    //pfd();
    //0|1|2|3 for R|W|RW|APPEND
    printf("in open file\n");
    printf("mode in open file= %d\n", mode);

    if(pathname[0] == '/')
    {
        dev = root->dev;
    }
    else
    {
        dev = running->cwd->dev;
    }

   // MINODE *minodePtr; 
    //get files minode 
    //pfd();
    int ino = getino(pathname);
    //pfd();

    if(ino <= 0)
    {
        char p[BLKSIZE], buf[BLKSIZE];
        strcpy(buf, pathname);
        strcpy(p, dirname(buf));
        printf("p = %s\n", p);
        int pino = getino(p);
        if(pino == -1)
        {
            printf("error on finding parent\n");
            return -1; 
        }
        MINODE *pmip = iget(dev, pino);

        // changed from creat_file//////////////
        my_creat(pmip, pathname);
        ino = getino(pathname);
    }

    //pfd();
    //name something else than mip
    MINODE *newmip = iget(dev, ino);
   // pfd();

    //check inode imode is regular file
    if(!S_ISREG(newmip->INODE.i_mode))
    {
        printf("error not a regular file\n");
        return -1;
    }
    //check whether file is already opened with incompatible mode:
    printf("mode = %d\n", mode);
    for(int i = 0; i < NFD; i++)
    {
       // pfd();
        printf("newmip ino = %d\n", newmip->ino);
        //if fd is null
        if(running->fd[i] == NULL)
            break; 
        if(running->fd[i]->minodePtr == newmip)
        {
            if(mode != 0)
            {
                printf("error opened with incompatable mode\n");
                return -1; 
            }
        }
    }

    //allocate an openTable entry OFT and initialize OFT entries
    OFT *oftp = (OFT *)malloc(sizeof(OFT)); 
    oftp->mode = mode; 
    oftp->refCount = 1; 
    //malloc size of minode
    oftp->minodePtr = (MINODE *)malloc(sizeof(MINODE));  //point at the files minode[]
    oftp->minodePtr = newmip;
    

    //depending which mode set offset 
    printf("mode : %d\n", mode);
    switch(mode){
        case 0: oftp->offset = 0;   //R: offset = 0
                break;
        case 1: truncate(newmip);      //W: truncate file to 0 size
                oftp->offset = 0; 
                break;
        case 2: oftp->offset = 0;   //RW: do NOT truncate file
                break; 
        case 3: oftp->offset = newmip->INODE.i_size; //append mode 
                break; 
        default: printf("invalid mode\n");
                 return(-1); 
    }
    //find the smallest i in running PROC's fd[ ] so that fd[i] is NULL
    int return_fd = -1; 
    for(int i = 0; i < NFD; i++)
    {
        if(running->fd[i] == NULL)
        {
            //let running fd[i] point at OFT entry 
            running->fd[i] = oftp; 
            return_fd = i; 
            break; 
        }
    }

    //update inodes time field
    // for R: touch atime
    // for W|RW|APPEND mode : touch atime and mtime
    if(mode != 0)   //means it wasnt for read
    {
        newmip->INODE.i_mtime = time(NULL);
    }
    //for W|RW|APPEND mode
    newmip->INODE.i_atime = time(NULL);
    //mark minode[] dirty 
    newmip->dirty = 1; 
    iput(newmip);
    //return i as the file descriptor 
    printf("fd = %d\n", return_fd);
    return return_fd; 

  // Eventually: return file descriptor of opened file
}

int my_close(int fd)
{
    printf("fd = %d\n", fd);
    //verify fd is within range 
    //check fd is a valid opened file descriptor
    if(running->fd[fd] == NULL)
    {
        printf("error not an OFT entry\n");
        return -1; 
    }
    //verify open fd is pointing at a an OFT entry 
    OFT *oftp = running->fd[fd];
    running->fd[fd] = 0; 
    oftp->refCount--; 
    if(oftp->refCount > 0)
    {
        return 0; 
    }
    printf("close: refcount = %d\n", oftp->refCount);
    printf("fd = %d is closed\n", fd);
    MINODE *mip = oftp->minodePtr;
    mip->dirty = 1; 
    iput(mip);
    free(oftp);
    return 0;
}

int close_file(int fd)
{
    if(running->fd[fd] == NULL)
    {
        printf("error\n");
        return -1;
    }

    OFT *oftp = running->fd[fd];
    running->fd[fd] = 0; 
    oftp->refCount--;
    if(oftp->refCount > 0)
    {
        return 0; 
    }
    MINODE *mip = oftp->minodePtr;
    mip->dirty = 1; 
    iput(mip);

    return 0;
}

//not required for level 2
int my_lseek(int fd, int position)
{
    //from fd find the OFT entry 
    if(running->fd[fd] == NULL)
    {
        printf("error not an OFT entry\n");
        return -1; 
    }
    OFT *oftp = running->fd[fd];
    //change OFT entrys offset to position but make sure NOT to over run 
    if(position > oftp->minodePtr->INODE.i_size || position < 0)
    {
        printf("error overrun\n");
    }

    int original_position = oftp->offset;
    oftp->offset = position; 

    return original_position;

   // Eventually: return original position in file
}

int pfd(void)
{
    //display currently opened files
     printf("fd  mode  offset    INODE\n");
     printf("------------------------------------\n");
    for(int i = 0; i < NFD; i++)
    {
        if(running == NULL || running->fd[i] == NULL)
            break;
        printf("%d    %d      %d     [%d, %d]\n", i, running->fd[i]->mode, running->fd[i]->offset, running->fd[i]->minodePtr->dev, running->fd[i]->minodePtr->ino);
    }
    return 0;
}

int dup(int fd)
{
   //verify fd is an opened descriptor

   //duplicates fd[fd] into first empty fd [] slot

   //increment OFT refCount by 1
    return 1;
}

int dup2(int fd, int gd)
{
    return 1;
}

int is_valid_fd(int fd) 
{
    return (fd >= 0 && fd < NFD);
}

#endif