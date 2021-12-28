 #include <stdio.h>
#include <stdlib.h>

#include <sodium.h>

int main(int argc, char *argv[])
{
        printf("Hello World Consti! %d \n",crypto_box_NONCEBYTES);
        printf("Hello World Consti! \n");

        char p[10];
        // call sodium method
        randombytes_buf((void*)p,sizeof(p));

        printf("Done x buffer ! \n");

        return 0;
}
