#include <stdlib.h>

#include <archive.h>
#include <archive_entry.h>

struct archive *
reader_init(void *buf, size_t size)
{
    struct archive *r;
    int ret;

    r = archive_read_new();
    if (r == NULL) {
        printf("init reader failed\n");
        return NULL;
    }

    ret = archive_read_support_format_tar(r);
    if (ret != ARCHIVE_OK) {
        printf("reader cannot support tar format\n");
        return NULL;
    }

    ret = archive_read_open_memory(r, buf, size);
    if (ret != ARCHIVE_OK) {
        printf("reader cannot open file\ncause: %s\n", archive_error_string(r));
        return NULL;
    }

    return r;
}

struct archive *
writer_init()
{
    struct archive *w;
    int ret;

    w = archive_write_disk_new();
    if (w == NULL) {
        printf("init writer failed\n");
        return NULL;
    }

    ret = archive_write_disk_set_options(w, ARCHIVE_EXTRACT_TIME);
    if (ret < 0) {
        printf("error setting writer disk options\n");
        return NULL;
    }

    return w;
}

int
unzip(struct archive *r, struct archive *w)
{
    struct archive_entry *entry;
    const void *buf;
    int64_t offset;
    size_t size;
    int ret;

    while ((ret = archive_read_next_header(r, &entry)) != ARCHIVE_EOF) {
        if (ret != ARCHIVE_OK) {
            printf("error reading archive entry\ncause: %s\n", archive_error_string(r));
            return -1;
        }

        ret = archive_write_header(w, entry);
        if (ret != ARCHIVE_OK) {
            printf("error writing %s entry\ncause: %s\n", archive_entry_pathname(entry), archive_error_string(w));
            return -1;
        }

        while ((ret = archive_read_data_block(r, &buf, &size, &offset)) != ARCHIVE_EOF) {
            if (ret != ARCHIVE_OK) {
                printf("error reading archive data block\ncause: %s\n", archive_error_string(r));
                return -1;
            }

            ret = archive_write_data_block(w, buf, size, offset);
            if (ret != ARCHIVE_OK) {
                printf("error writing archive data block\ncause: %s\n", archive_error_string(w));
                return -1;
            }
        }

        ret = archive_write_finish_entry(w);
        if (ret != ARCHIVE_OK) {
            printf("error finishing %s entry\ncause: %s\n", archive_entry_pathname(entry), archive_error_string(w));
            return -1;
        }
    }

    return 0;
}

void
archive_cleanup(struct archive *r, struct archive *w)
{
    archive_read_close(r);
	archive_read_free(r);
	
	archive_write_close(w);
  	archive_write_free(w);
}

int
file_read(char *name, void **buf_ptr, size_t *size)
{
    FILE *fp;
    size_t sz, amount_read, idx = 0;
    uint8_t *buf;

    fp = fopen(name, "r");
    if (fp == NULL)
        return -1;

    fseek(fp, 0L, SEEK_END);
    sz = ftell(fp);

    rewind(fp);

    buf = (uint8_t *) malloc(sz);
    if (buf == NULL)
        return -1;

    *size = sz;

    while (sz) {
        amount_read = fread(&buf[idx], sizeof(uint8_t), sz, fp);
        if (amount_read <= 0) {
            fclose(fp);
            free((void *) buf);
            return -1;
        }
        sz -= amount_read;
        idx += amount_read;
    }

    *buf_ptr = (void *) buf;

    return 0;
}

int
main(int argc, char *argv[])
{
    struct archive *reader, *writer;
    int ret;
    void *buf;
    size_t size;

    if (argc != 2) {
        printf("usage: %s ARCHIVE_FILE_NAME\n", argv[0]);
        return -1;
    }

    ret = file_read(argv[1], &buf, &size);
    if (ret < 0)
        return -1;

    reader = reader_init(buf, size);
    if (reader == NULL)
        return -1;

    writer = writer_init();
    if (writer == NULL)
        return -1;

    ret = unzip(reader, writer);
    if (ret < 0)
        return -1;

    archive_cleanup(reader, writer);

    return 0;
}
