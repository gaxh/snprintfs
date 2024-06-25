#include "snprintfs.h"

#include <string.h>
#include <assert.h>

struct SnprintfsBuffer {
    char *str;
    unsigned long offset;
    unsigned long left;
};

static void vsnprintfsb_char(struct SnprintfsBuffer *buffer, char v) {
    if(buffer->left != 0) {
        buffer->str[buffer->offset] = v;
        ++buffer->offset;
        --buffer->left;
    }
}

static void vsnprintfsb_string(struct SnprintfsBuffer *buffer, const char *v) {
    unsigned long copy_size = 0;
    while(v[copy_size] != 0) {
        ++copy_size;
    }

    copy_size = copy_size <= buffer->left ? copy_size : buffer->left;

    memcpy(buffer->str + buffer->offset, v, copy_size);

    buffer->offset += copy_size;
    buffer->left -= copy_size;
}

static const char *scale_map[] = {
    "0123456789abcdefghijklmnopqrstuvwxyz",
    "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ",
};

#define vsnprintfsb_unsigned(buffer, UnsignedType, val, scale, lower) \
    do { \
        const UnsignedType v = (UnsignedType)(val); \
        if(v == 0) { \
            vsnprintfsb_char(buffer, '0'); \
        } else { \
            char unsigned_buffer[64]; \
            unsigned unsigned_buffer_offset = 0; \
            UnsignedType value = v; \
            while(value != 0) { \
                UnsignedType modvalue = value % (UnsignedType)(scale); \
                assert(unsigned_buffer_offset < (unsigned)sizeof(unsigned_buffer)); \
                unsigned_buffer[unsigned_buffer_offset++] = scale_map[lower][modvalue];\
                value = value / (UnsignedType)(scale); \
            } \
            for(unsigned i = unsigned_buffer_offset - 1; i < (unsigned)sizeof(unsigned_buffer) && buffer->left != 0; --i) { \
                buffer->str[buffer->offset] = unsigned_buffer[i]; \
                ++buffer->offset; \
                --buffer->left; \
            } \
        } \
    } while(0)

#define vsnprintfsb_signed(buffer, SignedType, val, scale, lower) \
    do { \
        const SignedType v = (SignedType)(val); \
        if(v == 0) { \
            vsnprintfsb_char(buffer, '0'); \
        } else { \
            const SignedType hb_mask = (SignedType)1 << (sizeof(SignedType) * 8 - 1); \
            int negative = v < 0; \
            int min = (v & hb_mask) != 0 && (v & ~hb_mask) == 0; \
            char signed_buffer[64]; \
            unsigned signed_buffer_offset = 0; \
            SignedType value = v; \
            if(min) { \
                value += 1; \
            } \
            if(negative) { \
                value = -value; \
            } \
            while(value != 0) { \
                SignedType modvalue = value % (SignedType)(scale); \
                assert(signed_buffer_offset < (unsigned)sizeof(signed_buffer)); \
                signed_buffer[signed_buffer_offset++] = scale_map[lower][modvalue]; \
                value = value / (SignedType)(scale); \
            } \
            if(min) { \
                ++signed_buffer[0]; \
            } \
            if(negative) { \
                vsnprintfsb_char(buffer, '-'); \
            } \
            for(unsigned i = signed_buffer_offset - 1; i < (unsigned)sizeof(signed_buffer) && buffer->left != 0; --i) { \
                buffer->str[buffer->offset] = signed_buffer[i]; \
                ++buffer->offset; \
                --buffer->left; \
            } \
        } \
    } while(0)

static void vsnprintfsb(struct SnprintfsBuffer *buffer, const char *format, va_list args) {
    unsigned long format_offset = 0;

    while(buffer->left != 0 && format[format_offset] != 0) {
        // search for next '%'

        unsigned long sharp_offset = format_offset;
        unsigned long copy_size;

        while(format[sharp_offset] != '%' && format[sharp_offset] != 0) {
            ++sharp_offset;
        }

        copy_size = sharp_offset - format_offset;
        copy_size = copy_size <= buffer->left ? copy_size : buffer->left;

        memcpy(buffer->str + buffer->offset, format + format_offset, copy_size);

        buffer->offset += copy_size;
        buffer->left -= copy_size;
        format_offset = sharp_offset;

        if(buffer->left == 0 || format[format_offset] == 0) {
            break;
        }

        assert(format[format_offset] == '%');
        ++format_offset;

        switch(format[format_offset]) {
            case '%':
                vsnprintfsb_char(buffer, '%');
                ++format_offset;
                break;
            case 'd':
            case 'i':
                vsnprintfsb_signed(buffer, int, va_arg(args, int), 10, 0);
                ++format_offset;
                break;
            case 'o':
                vsnprintfsb_unsigned(buffer, unsigned, va_arg(args, unsigned), 8, 0);
                ++format_offset;
                break;
            case 'u':
                vsnprintfsb_unsigned(buffer, unsigned, va_arg(args, unsigned), 10, 0);
                ++format_offset;
                break;
            case 'x':
                vsnprintfsb_unsigned(buffer, unsigned, va_arg(args, unsigned), 16, 0);
                ++format_offset;
                break;
            case 'X':
                vsnprintfsb_unsigned(buffer, unsigned, va_arg(args, unsigned), 16, 1);
                ++format_offset;
                break;
            case 's':
                vsnprintfsb_string(buffer, va_arg(args, const char *));
                ++format_offset;
                break;
            case 'c':
                vsnprintfsb_char(buffer, va_arg(args, int));
                ++format_offset;
                break;
            case 'p':
                vsnprintfsb_char(buffer, '0');
                vsnprintfsb_char(buffer, 'x');
                vsnprintfsb_unsigned(buffer, unsigned long, (unsigned long)va_arg(args, void *), 16, 0);
                ++format_offset;
                break;
            case 'l':
                ++format_offset;
                {
                    switch(format[format_offset]) {
                        case 'd':
                        case 'i':
                            vsnprintfsb_signed(buffer, long, va_arg(args, long), 10, 0);
                            ++format_offset;
                            break;
                        case 'o':
                            vsnprintfsb_unsigned(buffer, unsigned long, va_arg(args, unsigned long), 8, 0);
                            ++format_offset;
                            break;
                        case 'u':
                            vsnprintfsb_unsigned(buffer, unsigned long, va_arg(args, unsigned long), 10, 0);
                            ++format_offset;
                            break;
                        case 'x':
                            vsnprintfsb_unsigned(buffer, unsigned long, va_arg(args, unsigned long), 16, 0);
                            ++format_offset;
                            break;
                        case 'X':
                            vsnprintfsb_unsigned(buffer, unsigned long, va_arg(args, unsigned long), 16, 1);
                            ++format_offset;
                            break;
                        default:
                            assert(0);
                            ++format_offset;
                            break;
                    }
                }
                break;
            default:
                assert(0);
                ++format_offset;
                break;
        }
    }
}

unsigned long vsnprintfs(char *str, unsigned long size, const char *format, va_list args) {
    if(size != 0) {
        struct SnprintfsBuffer buffer;
        buffer.str = str;
        buffer.offset = 0;
        buffer.left = size - 1u;
        vsnprintfsb(&buffer, format, args);

        buffer.str[buffer.offset] = 0;

        return buffer.offset;
    } else {
        return 0;
    }
    
}

unsigned long snprintfs(char *str, unsigned long size, const char *format, ...) {
    va_list args;
    unsigned long ret;
    va_start(args, format);
    ret = vsnprintfs(str, size, format, args);
    va_end(args);
    return ret;
}
