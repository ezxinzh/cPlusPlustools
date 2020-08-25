#include <stdio.h>
#include <stdlib.h>
 
int compareFile(FILE *old, FILE *new);
 
int main()
{
    FILE *new;
    FILE *old;
    int lRet;
    static first = 0;
    
    system("touch file_new");
    system("touch file_old");
 
    new = fopen("file_new", "rw");
    old = fopen("file_old", "rw");
 
    while(1)
    {   
        sleep(1);
        system("dmesg > file_new");
    
        if (first == 0)
        {   
            system("cp file_new file_old");
            first++;
            continue;
        }   
 
        new = fopen("file_new", "rw");
        old = fopen("file_old", "rw");
    
        if (compareFile(new, old) == 0)
        {   
            fclose(new);
            fclose(old);
            continue;
        }   
        else
        {   
            fclose(new);
            fclose(old);
    
            system("diff file_new file_old");
            system("cp file_new file_old");
        }  
    }
    return 0;
}
 
 
int compareFile(FILE *old, FILE *new)
{
    char c1;
    char c2;
 
    while(!feof(old) && !feof(new))
    {
        c1 = fgetc(old);
        c2 = fgetc(new);
 
        if (c1 != c2)
        {
            return -1;
        }
    }
 
    if ((c1 == EOF)&&(c2 == EOF))
    {
        return 0;
    }
    return -1;
}