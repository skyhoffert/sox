//
// soxc.c
// Sox compiler.
//

#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char* argv[])
{
    //////// Vars ////////
    char outfile[1024];
    strcpy(outfile, "a.out");
    char infiles[8][1024];
    for (int i = 0; i < 8; i++)
    {
        memset(infiles[i], 0, 1024);
    }
    int n_infiles = 0;

    //////// Parse args ////////
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-o") == 0)
        {
            if (i+1 < argc)
            {
                strcpy(outfile, argv[i+1]);
                i++;
            }
        }
        else
        {
            if (access(argv[i], F_OK) == 0)
            {
                strcpy(infiles[n_infiles], argv[i]);
                n_infiles++;
            }
            else
            {
                printf("ERR. Bad input file \"%s\".\n", argv[i]);
                return 100;
            }
        }
    }
    
    if (n_infiles == 0)
    {
        printf("ERR. No input files given.\n");
        return 101;
    }

    //////// Debug print stuff ////////
    /*
    printf("infiles: %s", infiles[0]);
    for (int i = 1; i < n_infiles; i++)
    {
        printf(",%s ", infiles[i]);
    }
    printf("\noutfile: %s\n", outfile);
    */

    //////// Parse the file ////////
    FILE* fp = fopen(infiles[0], "r");
    FILE* fout = fopen(outfile, "w");

    char c = fgetc(fp);
    char buf[1024];
    memset(buf, 0, 1024);
    char buf2[1024];
    memset(buf2, 0, 1024);
    int bufp = 0;

    int readstate = 0;

    while (!feof(fp))
    {
        if (c == '\n')
        {
            if (readstate != 1 && bufp > 0)
            {
                // printf("line was: \"%s\"\n", buf);

                if (strncmp(buf, "stdout", 6) == 0)
                {
                    char* start = strstr(buf, "\"");
                    start++;
                    char* end = strstr(start, "\"");
                    int len = end - start;
                    memset(buf2, 0, 1024);
                    memcpy(buf2, start, len);

                    char buf2c= buf2[0];
                    int buf2p = 1;
                    int buf2state = 0;
                    while (buf2c != 0)
                    {
                        if (buf2state == 0)
                        {
                            if (buf2c == '\\')
                            {
                                buf2state = 1;
                            }
                            else
                            {
                                printf("%c", buf2c);
                            }
                        }
                        else if (buf2state == 1)
                        {
                            if (buf2c == 'n')
                            {
                                printf("\n");
                            }
                            buf2state = 0;
                        }

                        buf2c = buf2[buf2p];
                        buf2p++;
                    }
                }
            }
            memset(buf, 0, 1024);
            bufp = 0;
            readstate = 0;
        }
        else if (readstate == 1) {}
        else
        {
            if (bufp == 0 && c == ' ') {}
            else
            {
                buf[bufp] = c;
                bufp++;
            }
        }

        if (strcmp(buf, "//") == 0)
        {
            readstate = 1;
            memset(buf, 0, 1024);
            bufp = 0;
        }

        c = fgetc(fp);
    }

    fclose(fp);
    fclose(fout);

    return 0;
}
