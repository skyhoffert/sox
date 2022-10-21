//
// soxc.c
// Sox compiler.
//

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#define TMP_FILE ".sox_tmp.c"

int main(int argc, char* argv[])
{
    //////// Vars ////////
    char outfile[128];
    strcpy(outfile, "a.out");

    char infiles[8][128];
    for (int i = 0; i < 8; i++)
    {
        memset(infiles[i], 0, 128);
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
    FILE* fout = fopen(TMP_FILE, "w");

    char maincode[10240];
    memset(maincode, 0, 10240);
    int maincodep = 0;

    char include_stdio = 0;

    char c = fgetc(fp);
    char buf[1024];
    memset(buf, 0, 1024);
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
                    include_stdio = 1;

                    char* start = strstr(buf, "\"");
                    start++;
                    char* end = strstr(start, "\"");
                    int len = end - start;
                    maincodep += sprintf(maincode + maincodep, "printf(\"");
                    memcpy(maincode + maincodep, start, len);
                    maincodep += len;
                    maincodep += sprintf(maincode + maincodep, "\");\n");
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

    if (include_stdio == 1)
    {
        fprintf(fout, "#include <stdio.h>\n");
    }

    fprintf(fout, "int main(int argc, char* argv[])\n{\n");

    fprintf(fout, "%s", maincode);

    fprintf(fout, "return 0;\n}\n");

    fclose(fout);

    char soxcomp[256];
    memset(soxcomp, 0, 256);
    sprintf(soxcomp, "/usr/bin/gcc -o %s %s", outfile, TMP_FILE);
    // printf("Running \"%s\"\n", soxcomp);
    system(soxcomp);

    return 0;
}
