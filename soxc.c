//
// soxc.c
// Sox compiler.
//

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#define DEBUG 1

#define TMP_FILE ".sox_tmp.c"

#define N_VARS 1024
#define VAR_NAMELEN 32

#define N_TOKS 8
#define TOK_NAMELEN 32

#define STR_MAXLEN 1024

int parse_line(char* line, char* toks);
int parse_string(char* line, char* str);

int main(int argc, char* argv[])
{
    if (DEBUG == 1)
    {
        printf("//////// soxc DEBUG output ////////\n");
    }

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

    char vars[N_VARS][VAR_NAMELEN];
    for (int i = 0; i < N_VARS; i++)
    {
        memset(vars[i], 0, VAR_NAMELEN);
    }
    int n_vars = 0;
    char bufvar[VAR_NAMELEN];
    memset(bufvar, 0, VAR_NAMELEN);

    char include_stdio = 0;
    char include_stdint = 0;
    char include_string = 0;

    char c = fgetc(fp);
    char buf[1024];
    memset(buf, 0, 1024);
    int bufp = 0;

    char toks[N_TOKS*TOK_NAMELEN];

    char str[STR_MAXLEN];

    int readstate = 0;
    int linenum = 1;

    while (!feof(fp))
    {
        if (c == '\n')
        {
            if (readstate != 1 && bufp > 0)
            {
                // printf("line was: \"%s\"\n", buf);

                if (strncmp(buf, "stdout(", 7) == 0)
                {
                    include_stdio = 1;

                    char* start = strstr(buf, "(");
                    start++;
                    char* end = strstr(start, ")");
                    int len = end - start;
                    maincodep += sprintf(maincode + maincodep, "printf(");
                    memcpy(maincode + maincodep, start, len);
                    maincodep += len;
                    maincodep += sprintf(maincode + maincodep, ");\n");
                }
                else if (strncmp(buf, "stderr(", 7) == 0)
                {
                    include_stdio = 1;

                    char* start = strstr(buf, "(");
                    start++;
                    char* end = strstr(start, ")");
                    int len = end - start;
                    maincodep += sprintf(maincode + maincodep, "fprintf(stderr, \"\\e[01;31m\");\n");
                    maincodep += sprintf(maincode + maincodep, "fprintf(stderr, ");
                    memcpy(maincode + maincodep, start, len);
                    maincodep += len;
                    maincodep += sprintf(maincode + maincodep, ");\n");
                    maincodep += sprintf(maincode + maincodep, "fprintf(stderr, \"\\e[0m\");\n");
                }
                else if (strncmp(buf, "int8 ", 4) == 0)
                {
                    include_stdint = 1;

                    int ntoks = parse_line(buf, toks);

                    maincodep += sprintf(maincode + maincodep, "int8_t %s = %s;\n", toks+(1*TOK_NAMELEN), toks+(3*TOK_NAMELEN));
                }
                else if (strncmp(buf, "string ", 7) == 0)
                {
                    include_string = 1;

                    int ntoks = parse_line(buf, toks);

                    int slen = parse_string(buf, str);

                    maincodep += sprintf(maincode + maincodep, "char %s[%d];\n", toks+(1*TOK_NAMELEN), STR_MAXLEN);
                    maincodep += sprintf(maincode + maincodep, "strcpy(%s, \"%s\");\n", toks+(1*TOK_NAMELEN), str);
                }
                else if (strncmp(buf, "bool ", 5) == 0)
                {
                    int ntoks = parse_line(buf, toks);

                    maincodep += sprintf(maincode + maincodep, "char %s = %s;\n", toks+(1*TOK_NAMELEN), toks+(3*TOK_NAMELEN));
                }
                else
                {
                    printf("ERR. Bad line %d \"%s\".\n", linenum, buf);
                    fclose(fp);
                    fclose(fout);
                    return 102;
                }
            }
            memset(buf, 0, 1024);
            bufp = 0;
            readstate = 0;
            linenum++;
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

    //////// Parse maincode and replace as needed. ////////
    char* bfinder = strstr(maincode, "\%b");
    while (bfinder != NULL)
    {
        // Replace "%b" with "%d".
        bfinder[1] = 'd';
        bfinder = strstr(bfinder+1, "\%b");
    }

    //////// Print to C file. ////////
    if (include_stdio == 1)
    {
        fprintf(fout, "#include <stdio.h>\n");
    }
    if (include_stdint == 1)
    {
        fprintf(fout, "#include <stdint.h>\n");
    }
    if(include_string == 1)
    {
        fprintf(fout, "#include <string.h>\n");
    }

    fprintf(fout, "int main(int argc, char* argv[])\n{\n");
    fprintf(fout, "%s", maincode);
    fprintf(fout, "return 0;\n}\n");
    fclose(fout);

    char soxcomp[256];
    memset(soxcomp, 0, 256);
    sprintf(soxcomp, "/usr/bin/gcc -o %s %s", outfile, TMP_FILE);
    // printf("Running \"%s\"\n", soxcomp);
    int retval = system(soxcomp);

    if (retval == 0 && DEBUG != 1)
    {
        remove(TMP_FILE);
    }
    
    if (DEBUG == 1)
    {
        printf("//////// END soxc DEBUG ////////\n\n");
    }

    return 0;
}

int parse_line(char* line, char* toks)
{
    memset(toks, 0, N_TOKS * TOK_NAMELEN);
    char* tokpa = line;
    char* tokpb = NULL;
    int len = 0;
    int ntoks = 0;
    do
    {
        tokpb = strstr(tokpa+1, " ");
        if (tokpb == NULL)
        {
            tokpb = strstr(tokpa+1, ";");
        }
        if (tokpb == NULL)
        {
            break;
        }

        len = tokpb - tokpa;
        memcpy(toks+(ntoks*TOK_NAMELEN), tokpa, len);
        ntoks++;
        tokpa = strstr(tokpa+1, " ");
        if (tokpa != NULL)
        {
            tokpa++;
        }
    } while (tokpa != NULL);

    return ntoks;
}

int parse_string(char* line, char* str)
{
    memset(str, 0, STR_MAXLEN);

    char* start = strstr(line, "\"");
    start++;
    char* end = strstr(start, "\"");
    int len = end - start;

    memcpy(str, start, len);

    return len;
}
