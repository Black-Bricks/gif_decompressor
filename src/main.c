#include "main.h"
#include "gifdec.h"
#include <fcntl.h>
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif
#include "getopt.h"
#include <sys/stat.h>

#define OUT_FNAME_DEFAULT_PREFIX    "out_\0"

enum error {
    ERR_OK = 0,
    ERR_EMPTY_FILENAME = -1,
    ERR_NOT_FOUND = -2,
    ERR_NOT_GIF = -3,
    ERR_NO_ACCESS = -4,
    ERR_NO_MEM = -5,
};

static void put_key(
    FILE *fd, 
    int key_size, 
    uint8_t key, 
    uint8_t *shift, 
    uint8_t *byte,
    uint8_t *sub_size
) {
    key <<= 8 - key_size;
    *byte |= key >> *shift;
    if (*shift + key_size > 7) {
        write(fd, byte, 1);
        *byte = 0;
        *byte |= key << (8 - *shift);
        *shift = *shift + key_size - 8;
        *sub_size = *sub_size + 1;
    } else {
        *shift += key_size;
    }
}

static int decompress_gif(
    char *input, 
    char *output, 
    bool remove_gct, 
    bool remove_lct
) {
    if (input == NULL)
        return ERR_EMPTY_FILENAME;
    if (output == NULL) {
        char prefix[] = OUT_FNAME_DEFAULT_PREFIX;
        output = malloc(strlen(input) + sizeof(prefix) + 1);
        memset(output, 0, strlen(input) + sizeof(prefix));
        char *last_slash = strrchr(input, '\\');
        if (last_slash == NULL) {
            last_slash = strrchr(input, '/');
        }
        strncpy(output, input, last_slash - input + 1);
        strcat(output, prefix);
        strcat(output, last_slash + 1);
        LOGI("input: %s", input);
        LOGI("output: %s", output);
    }

    int ret = ERR_OK;

    LOGI("Open file");
    gd_GIF *gif = gd_open_gif_n_cpy(input, output, remove_gct ? 0 : 1);
    if (gif == NULL) {
        return ERR_NOT_GIF;
    }
    LOGI("Decompress frames");
    int n = 1;
    while (gd_get_frame_n_cpy(gif, remove_lct ? 0 : 1)) {
        uint8_t key_size = -1;
        uint16_t p_size = gif->palette->size;
        while (p_size) {
            key_size++;
            p_size >>= 1;
        }
        write(gif->ofd, &key_size, 1);
        LOGI("frame %d, w = %d, h = %d, palette size = %d, key size = %d", 
            n++, gif->fw, gif->fh, gif->palette->size, key_size);
        off_t sub_block_size_ptr = lseek(gif->ofd, 0, SEEK_CUR);
        off_t file_ptr;
        uint8_t sub_block_size = 0;
        uint8_t shift = 0;
        uint8_t byte = 0;
        lseek(gif->ofd, 1, SEEK_CUR);
        for (int i = 0; i < gif->fw * gif->fh; i++) {
            put_key(
                gif->ofd, 
                key_size, 
                gif->frame[i], 
                &shift, 
                &byte, 
                &sub_block_size
            );
            if (sub_block_size != 255)
                continue;
            LOGI("sub block size = %d", sub_block_size);
            file_ptr = lseek(gif->ofd, 0, SEEK_CUR);
            lseek(gif->ofd, sub_block_size_ptr, SEEK_SET);
            write(gif->ofd, &sub_block_size, 1);
            lseek(gif->ofd, file_ptr, SEEK_SET);
            sub_block_size = 0;
            sub_block_size_ptr = lseek(gif->ofd, 0, SEEK_CUR);
            lseek(gif->ofd, 1, SEEK_CUR);
        }
        if (sub_block_size_ptr == lseek(gif->ofd, 0, SEEK_CUR)) {
            continue;
        }
        LOGI("sub block size = %d", sub_block_size);
        file_ptr = lseek(gif->ofd, 0, SEEK_CUR);
        lseek(gif->ofd, sub_block_size_ptr, SEEK_SET);
        write(gif->ofd, &sub_block_size, 1);
        lseek(gif->ofd, file_ptr, SEEK_SET);
        byte = 0;
        write(gif->ofd, &byte, 1);
    }
gif_ret:
    LOGI("Close files");
    gd_close_gif_n_cpy(gif);
    return ret;
}

static void print_help() {
    printf("-i\t\t- input gif file name\n");
    printf("-o\t\t- output gif file name\n");
    printf("--rgct\t\t- remove global color table\n");
    printf("-h(--help)\t- get help\n");
}

static void print_error(int err) {
    switch (err) {
        case ERR_OK:
            printf("Decompression successful.\n");
            break;
        case ERR_EMPTY_FILENAME:
            printf("Input file name is empty.\n");
            break;
        case ERR_NOT_FOUND:
            printf("File not found.\n");
            break;
        case ERR_NOT_GIF:
            printf("Input file is not a gif file.\n");
            break;
        case ERR_NO_ACCESS:
            printf("No permissions to access to the file.\n");
            break;
        case ERR_NO_MEM:
            printf("Not enough memory.\n");
            break;
        default:
            break;
    }
}

int main(int argc, char *argv[]) {
    char *input_file = NULL;
    char *output_file = NULL;
    bool rgct = false;
    bool rlct = false;
    enum opt {
        OPT_END = -1,
        OPT_HELP = 'h',
        OPT_REMOVE_GCT = 'r',
        OPT_REMOVE_LCT = 'l',
        OPT_INPUT = 'i',
        OPT_OUTPUT = 'o',
        OPT_UNEXPECTED = '?',
    };
    static const struct option longopts[] = {
        {.name = "help", .has_arg = no_argument, .val = OPT_HELP},
        {.name = "rgct", .has_arg = no_argument, .val = OPT_REMOVE_GCT},
        {.name = "rlct", .has_arg = no_argument, .val = OPT_REMOVE_LCT},
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
                rgct = true;
                break;
            case OPT_REMOVE_LCT:
                LOGI("Remove local color table");
                rlct = true;
                break;
            case OPT_INPUT:
                LOGI("Input file: %s", optarg);
                input_file = malloc(strlen(optarg) + 1);
                memset(input_file, 0, strlen(optarg) + 1);
                strcpy(input_file, optarg);
                break;
            case OPT_OUTPUT:
                LOGI("Output file %s", optarg);
                output_file = malloc(strlen(optarg) + 1);
                memset(output_file, 0, strlen(optarg) + 1);
                strcpy(output_file, optarg);
                break;
            case OPT_UNEXPECTED:
                print_help();
                return 1;
        }
    }
end_optparse:
    LOGI("Decompress");
    int ret = decompress_gif(input_file, output_file, rgct, rlct);
    print_error(ret);
    return 0;
}