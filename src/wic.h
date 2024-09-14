#ifndef WIC_H
#define WIC_H

typedef struct _tagImageBuffer {
    int width;
    int height;
    size_t stride;
    size_t size;
    void* bits;
} ImageBuffer;

ImageBuffer ImageFileReader(LPWSTR szFilePath);
void ImageFileWriter(LPWSTR, ImageBuffer);

#endif  /* WIC_H */
