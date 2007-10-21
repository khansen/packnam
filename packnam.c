#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char program_version[] = "packnam 1.0";

/* Prints usage message and exits. */
static void usage()
{
    printf(
        "Usage: packnam [--width=NUM] [--vram-address=NUM]\n"
        "               [--output=FILE]\n"
        "               [--help] [--usage] [--version]\n"
        "                FILE\n");
    exit(0);
}

/* Prints help message and exits. */
static void help()
{
    printf("Usage: packnam [OPTION...] FILE\n\n"
           "  --width=NUM                     Width of input is NUM tiles (32)\n"
           "  --vram-address=NUM              VRAM start address (0x2000)\n"
           "  --output=FILE                   Store encoded data in FILE\n"
           "  --help                          Give this help list\n"
           "  --usage                         Give a short usage message\n"
           "  --version                       Print program version\n");
    exit(0);
}

/* Prints version and exits. */
static void version()
{
    printf("%s\n", program_version);
    exit(0);
}

/**
 * Program entrypoint.
 */
int main(int argc, char **argv)
{
    char nametable[1024];
    int nametable_sz;
    char *out = 0;
    int out_pos;
    int out_sz;
    int buf_sz;
    int width = 32;
    int vram_address = 0x2000;
    const char *input_filename = 0;
    const char *output_filename = 0;
    /* Process arguments. */
    {
        char *p;
        while ((p = *(++argv))) {
            if (!strncmp("--", p, 2)) {
                const char *opt = &p[2];
                if (!strncmp("width=", opt, 6)) {
                    width = strtol(&opt[6], 0, 0);
                } else if (!strncmp("vram-address=", opt, 13)) {
                    vram_address = strtol(&opt[13], 0, 0);
                } else if (!strncmp("output=", opt, 7)) {
                    output_filename = &opt[7];
                } else if (!strcmp("help", opt)) {
                    help();
                } else if (!strcmp("usage", opt)) {
                    usage();
                } else if (!strcmp("version", opt)) {
                    version();
                } else {
                    fprintf(stderr, "unrecognized option `%s'\n", p);
                    return(-1);
                }
            } else {
                input_filename = p;
            }
        }
    }

    if (!input_filename) {
        fprintf(stderr, "packnam: no filename given\n");
        return(-1);
    }

    {
        FILE *fp = fopen(input_filename, "rb");
        if (!fp) {
            fprintf(stderr, "packnam: failed to open `%s' for reading\n", input_filename);
            return(-1);
        }

        nametable_sz = fread(nametable, 1, 1024, fp);
        fclose(fp);
    }

    {
        int y = 0;
        buf_sz = 0;
        int in_pos = 0;
        out_pos = 0;
        while (y * width < nametable_sz) {
            char rlech;
            int count;
            int count_pos;
            int x = 0;
            int state = 0;
            int start_rle = 0;
            int addr = vram_address + (y * 32);
            while (x < width) {
                const char ch = nametable[in_pos];
                switch (state) {
                    case 0:
                    start_rle = (x + 4 <= width)
                            && (nametable[in_pos+1] == ch)
                            && (nametable[in_pos+2] == ch)
                            && (nametable[in_pos+3] == ch);
                    /* fallthrough */
                    case 1:
                    if (ch == 0) {
                        /* don't encode 0s */
                        start_rle = 0;
                        ++in_pos;
                        ++x;
                        ++addr;
                        break;
                    }
                    if (out_pos + 2 >= buf_sz) {
                        buf_sz += 64;
                        out = (char *)realloc(out, buf_sz);
                    }
                    out[out_pos++] = (char)(addr >> 8);
                    out[out_pos++] = (char)(addr & 255);
                    if (start_rle) {
                        /* start of RLE run */
                        count = 4;
                        rlech = ch;
                        in_pos += 4;
                        x += 4;
                        state = 2;
                    } else {
                        /* start of non-RLE run */
                        count_pos = out_pos;
                        if (out_pos + 2 >= buf_sz) {
                            buf_sz += 64;
                            out = (char *)realloc(out, buf_sz);
                        }
                        out[out_pos++] = 0; /* count backpatched later */
                        count = 1;
                        out[out_pos++] = ch;
                        ++in_pos;
                        ++x;
                        state = 3;
                    }
                    break;

                    case 2:
                    if (ch == rlech) {
                        ++count;
                        ++in_pos;
                        ++x;
                    }
                    if ((ch != rlech) || (count == 0x3F)) {
                        /* end RLE run */
                        if (out_pos + 2 >= buf_sz) {
                            buf_sz += 64;
                            out = (char *)realloc(out, buf_sz);
                        }
                        out[out_pos++] = (char)(0x40 | count);
                        out[out_pos++] = rlech;
                        addr += count;
                        state = 0;
                    }
                    break;

                    case 3:
                    start_rle = ((x + 4 <= width)
                            && (nametable[in_pos+1] == ch)
                            && (nametable[in_pos+2] == ch)
                            && (nametable[in_pos+3] == ch));
                    if (start_rle) {
                        /* end non-RLE run */
                        out[count_pos] = count;
                        addr += count;
                        state = 1;
                    } else {
                        if (out_pos + 1 >= buf_sz) {
                            buf_sz += 64;
                            out = (char *)realloc(out, buf_sz);
                        }
                        out[out_pos++] = ch;
                        ++in_pos;
                        ++x;
                        ++count;
                        if (count == 0x3F) {
                            /* end non-RLE run */
                            out[count_pos] = count;
                            addr += count;
                            state = 1;
                        }
                    }
                    break;
                }
            }
            switch (state) {
                case 0:
                case 1:
                break;

                case 2:
                /* finish RLE run */
                if (out_pos + 2 >= buf_sz) {
                    buf_sz += 64;
                    out = (char *)realloc(out, buf_sz);
                }
                out[out_pos++] = (char)(0x40 | count);
                out[out_pos++] = rlech;
                break;

                case 3:
                /* finish non-RLE run */
                out[count_pos] = count;
                break;
            }
            ++y;
        }
        out_sz = out_pos;
    }

    if (!output_filename)
        output_filename = "packnam.dat";
    {
        FILE *fp = fopen(output_filename, "wb");
        fwrite(out, 1, out_sz, fp);
        fclose(fp);
    }

    return 0;
}
