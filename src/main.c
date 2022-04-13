#include "main.h"
#include "gifdec.h"
#include "unistd.h"
#include "getopt.h"
#include <sys/stat.h>

#define GIF_HEADER_SIZE 13
#define FRAME_HEADER_SIZE   9

uint8_t *file_buf = NULL;

enum error {
    ERR_OK = 0,
    ERR_EMPTY_FILENAME = -1,
    ERR_NOT_FOUND = -2,
    ERR_NOT_GIF = -3,
    ERR_NO_ACCESS = -4,
    ERR_NO_MEM = -5,
};

static void put_key(FILE *fd, int key_size, uint8_t key, uint8_t *shift, uint8_t *byte) {
    key <<= 8 - key_size;
    *byte |= key >> *shift;
    if (*shift + key_size > 7) {
        fwrite(byte, 1, 1, fd);
        *byte = 0;
        *shift = *shift + key_size - 8;
    }
}

static int skip_to_next_frame(uint8_t *buf, long *ptr) {
    char sep;
    int cnt = *ptr;
    sep = buf[cnt++];
    while (sep != ',') {
        if (sep == ';')
            return -1;
        sep = buf[cnt++];
    }
    return cnt;
}

static int decompress_gif(char *input, char *output, bool remove_gct) {
    if (input == NULL)
        return ERR_EMPTY_FILENAME;
    if (output == NULL) {
        strcpy(output, "out_\0");
        strcat(output, input);
    }

    int ret = ERR_OK;
    long file_ptr = 0;

    // Open input file
    FILE *in_f = fopen(input, "rb");
    if (in_f == NULL) {
        ret = ERR_NOT_FOUND;
        goto gif_ret;
    }

    // Get input file size
    struct stat stbuf;
    if (stat(input, &stbuf) == -1) {
        LOGE("Can't access to the file");
        ret = ERR_NO_ACCESS;
        goto gif_ret;
    }

    // Read input file
    file_buf = malloc(stbuf.st_size);
    if (file_buf == NULL) {
        ret = ERR_NO_MEM;
        goto gif_ret;
    }
    fread(file_buf, 1, stbuf.st_size, in_f);
    fclose(in_f);

    // Open input file with GIF
    gd_GIF *gif = gd_open_gif(input);
    if (gif == NULL) {
        ret = ERR_NOT_GIF;
        goto gif_ret;
    }

    // Copy GIF header from input file
    FILE *out_f = fopen(output, "wb");
    fwrite(file_buf, 1, GIF_HEADER_SIZE, out_f);
    file_ptr = GIF_HEADER_SIZE;
    
    // Don't copy global color table if needed
    if (remove_gct) {
        if (gif->gct.size != 0) {
            file_ptr += gif->gct.size * 3;
        }
    }

    uint16_t sub_len;
    uint8_t byte = 0;
    uint8_t shift = 0;
    uint8_t key_size;
    long sub_len_ptr;
    long next_frame;
    while (gd_get_frame(gif)) {
        // Copy until the next frame
        next_frame = skip_to_next_frame(file_buf, file_ptr); 
        if (next_frame == -1)
            goto gif_ret;

        fwrite(&file_buf[file_ptr], 1, next_frame - file_ptr, out_f);
        file_ptr = next_frame;

        fwrite(&file_buf[file_ptr], 1, FRAME_HEADER_SIZE + 1, out_f);
        file_ptr += FRAME_HEADER_SIZE + 1;

        key_size = file_buf[file_ptr - 1];
        sub_len_ptr = lseek(out_f, 0, SEEK_CUR);
        lseek(out_f, 1, SEEK_CUR);
        
        for (int i = 0; i < gif->fw * gif->fh; i++) {
            put_key(out_f, key_size, gif->frame[i], &shift, &byte);
            sub_len++;
            long cursor = lseek(out_f, 0, SEEK_CUR);
            if (sub_len == 256) {
                byte = 0;
                fwrite(&byte, 1, 1, out_f);
                lseek(out_f, sub_len_ptr, SEEK_SET);
                byte = 255;
                fwrite(&byte, 1, 1, out_f);

            }
        }
    }
    byte = ';';
    fwrite(&byte, 1, 1, out_f);

gif_ret:
    free(file_buf);
    fclose(in_f);
    fclose(out_f);
    gd_close_gif(gif);
    return ret;
}

static void print_help() {
    printf("-i\t\t- input gif file name\n");
    printf("-o\t\t- output gif file name\n");
    printf("--rgct\t\t- remove global color table\n");
    printf("-h(--help)\t- get help\n");
}

int main(int argc, char *argv[]) {
    char *input_file = NULL;
    char *output_file = NULL;
    enum opt {
        OPT_END = -1,
        OPT_HELP = 'h',
        OPT_REMOVE_GCT = 'r',
        OPT_INPUT = 'i',
        OPT_OUTPUT = 'o',
        OPT_UNEXPECTED = '?',
    };
    static const struct option longopts[] = {
        {.name = "help", .has_arg = no_argument, .val = OPT_HELP},
        {.name = "rgct", .has_arg = no_argument, .val = OPT_REMOVE_GCT},
        {},
    };
    for (;;) {
        int longindex = -1;
        enum opt opt = getopt_long(argc, argv, "i:o:h", longopts, &longindex);
        switch (opt) {
        case OPT_END:
            goto end_optparse;
        case OPT_HELP:
            print_help();
            break;
        case OPT_REMOVE_GCT:
            LOGI("Remove global color table");
            break;
        case OPT_INPUT:
            LOGI("Input file %s", optarg);
            break;
        case OPT_OUTPUT:
            LOGI("Output file %s", optarg);
            break;
        case OPT_UNEXPECTED:
            print_help();
            return 1;
        }
    }
end_optparse:
    return 0;
}