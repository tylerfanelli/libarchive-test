#include <archive.h>
#include <archive_entry.h>

struct archive *
reader_init(char *filename)
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

    ret = archive_read_open_filename(r, filename, 10240);
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
main(int argc, char *argv[])
{
    struct archive *reader, *writer;
    int ret;

    if (argc != 2) {
        printf("usage: %s ARCHIVE_FILE_NAME\n", argv[0]);
        return -1;
    }

    reader = reader_init(argv[1]);
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
