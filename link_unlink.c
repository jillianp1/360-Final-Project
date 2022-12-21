/************* link_unlink.c file **************/

#ifndef __LINKUNLINK_C__
#define __LINKUNLINK_C__

#include "mkdir_creat.c"
#include "rmdir.c"

int my_link(MINODE *filePip, char *fileName, int fileIno, MINODE *linkPip, char *linkName)
{
    return 1;
}

int link_file(char *old_file, char *new_file)
{
    // char *old_file;
    // char *new_file; 

    if(old_file[0] == '/')
    {
        dev = root->dev;
    }
    else
    {
        dev = running->cwd->dev;
    }

    //verify old file exists and is not a dir
    int oino = getino(old_file);
    MINODE *omip = iget(dev, oino);

    //check omip->INODE file type is a dir
    if(S_ISDIR(omip->INODE.i_mode))
    {
        printf("error type is DIR\n");
        return -1; 
    }

    //set for new
    if(new_file[0] == '/')
    {
        dev = root->dev;
    }
    else
    {
        dev = running->cwd->dev;
    }

    int fino = getino(new_file);
    //new file must not exist yet getino(newfile) must return 0
    if(fino != 0)
    {
        printf("error file already exists\n");
        return -1;
    }

    //creat new_file with same inode number of old_file

   //divide pathname into dirname an dbasename
    strcpy(temp, new_file);
    strcpy (base, basename(temp));
    printf("basename = %s\n", base);
    strcpy(temp2, new_file);
    strcpy(dirName, dirname(temp2));
    printf("dirname = %s\n", dirName);

    int nino = getino(dirName);

    MINODE *nmip = iget(dev, nino);
    //creat entry in new parent DIR with same inode number of old file
    enter_name(nmip, omip->ino, base);

    //increase link count by 1
    omip->INODE.i_links_count ++; 
    omip->dirty = 1; 
    nmip->dirty = 1; 

    iput(omip);
    iput(nmip);

}

int my_unlink(char *pathname)
{
    if(pathname[0] == '/')
    {
        dev = root->dev;
    }
    else
    {
        dev = running->cwd->dev;
    }

    //divide pathname into dirname an dbasename
    strcpy(temp, pathname);
    strcpy (base, basename(temp));
    printf("basename = %s\n", base);
    strcpy(temp2, pathname);
    strcpy(dirName, dirname(temp2));
    printf("dirname = %s\n", dirName);

    //get filenames minode
    int ino = getino(pathname);
    MINODE *mip = iget(dev, ino); 

    //check its REG or symbolic LNK file can not be a dir
    if(S_ISDIR(mip->INODE.i_mode))
    {
        printf("type is dir not allowed\n");
        return -1; 
    }

    // //rm_child(pmip, ino, base);
    // pmip->dirty = 1;
    // iput(pmip);

    //decrement INODE link count by 1
    mip->INODE.i_links_count--;
    if(mip->INODE.i_links_count == 0)
    {
        //deallocate all data blocks inINODE
        //deallocate INODE; 
        if(!S_ISLNK(mip->INODE.i_mode))
        {
            truncate(mip);
        }
    }
    mip->dirty =1;
    //release mip
    iput(mip); 

    //remove name entry from parent DIR data block
    int pino = getino(dirName);
    MINODE *pmip = iget(mip->dev, pino);
    rm_child(pmip, base);
    return 0; 
}

int my_rm(MINODE *mip, char *pathname)
{
    return 1;
}

#endif