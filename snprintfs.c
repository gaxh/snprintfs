#include "snprintfs.h"

#include <string.h>
#include <assert.h>

struct SnprintfsBuffer {
    char *str;
    unsigned long offset;
    unsigned long left;
};

enum SnprintfsIntFormatModifier {
    SnprintfsIntFormatModifier_INT = 0,
    SnprintfsIntFormatModifier_LONG,
    SnprintfsIntFormatModifier_LONGLONG,
};

struct SnprintfsConversions {
    int conversion; // 0, 'd', 'u', 'x', 'p', ... // 0 is INVALID conversion
    enum SnprintfsIntFormatModifier long_int; // if 'l' present
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

static void conversion_clear(struct SnprintfsConversions *sc) {
    sc->conversion = 0;
    sc->long_int = SnprintfsIntFormatModifier_INT;
}

static unsigned long conversion_next(struct SnprintfsConversions *sc, const char *format, unsigned long offset) {
    static const char VALID_CONVERSIONS[] = "%%diouxXscp";

    conversion_clear(sc);

    while(format[offset] != 0) {

        if(strchr(VALID_CONVERSIONS, format[offset])) {
            sc->conversion = format[offset];
            break;
        }

        if(format[offset] == 'l') {
            if(sc->long_int == SnprintfsIntFormatModifier_INT) {
                sc->long_int = SnprintfsIntFormatModifier_LONG;
            } else if(sc->long_int == SnprintfsIntFormatModifier_LONG) {
                sc->long_int = SnprintfsIntFormatModifier_LONGLONG;
            }
        }

        ++offset;
    }

    return offset;
}

static void vsnprintfsb(struct SnprintfsBuffer *buffer, const char *format, va_list args) {
    unsigned long format_offset = 0;

    while(buffer->left != 0 && format[format_offset] != 0) {
        // search for next '%'

        unsigned long sharp_offset = format_offset;
        unsigned long copy_size;
        struct SnprintfsConversions sc;

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
        
        format_offset = conversion_next(&sc, format, format_offset);

        if(sc.conversion == 0) {
            break;
        }

#define VSNPRINTFSB_FORMAT_INT(buffer, args, scale, lower) \
        switch(sc.long_int) { \
            case SnprintfsIntFormatModifier_INT: \
                vsnprintfsb_signed(buffer, int, va_arg(args, int), scale, lower); \
                break; \
            case SnprintfsIntFormatModifier_LONG: \
                vsnprintfsb_signed(buffer, long, va_arg(args, long), scale, lower); \
                break; \
            case SnprintfsIntFormatModifier_LONGLONG: \
                vsnprintfsb_signed(buffer, long long, va_arg(args, long long), scale, lower); \
                break; \
        }

#define VSNPRINTFSB_FORMAT_UNSIGNED(buffer, args, scale, lower) \
        switch(sc.long_int) { \
            case SnprintfsIntFormatModifier_INT: \
                vsnprintfsb_unsigned(buffer, unsigned, va_arg(args, unsigned), scale, lower); \
                break; \
            case SnprintfsIntFormatModifier_LONG: \
                vsnprintfsb_unsigned(buffer, unsigned long, va_arg(args, unsigned long), scale, lower); \
                break; \
            case SnprintfsIntFormatModifier_LONGLONG: \
                vsnprintfsb_unsigned(buffer, unsigned long long, va_arg(args, unsigned long long), scale, lower); \
                break; \
        }

        switch(sc.conversion) {
            case '%':
                vsnprintfsb_char(buffer, '%');
                break;
            case 'd':
            case 'i':
                VSNPRINTFSB_FORMAT_INT(buffer, args, 10, 0);
                break;
            case 'o':
                VSNPRINTFSB_FORMAT_UNSIGNED(buffer, args, 8, 0);
                break;
            case 'u':
                VSNPRINTFSB_FORMAT_UNSIGNED(buffer, args, 10, 0);
                break;
            case 'x':
                VSNPRINTFSB_FORMAT_UNSIGNED(buffer, args, 16, 0);
                break;
            case 'X':
                VSNPRINTFSB_FORMAT_UNSIGNED(buffer, args, 16, 1);
                break;
            case 's':
                vsnprintfsb_string(buffer, va_arg(args, const char *));
                break;
            case 'c':
                vsnprintfsb_char(buffer, va_arg(args, int));
                break;
            case 'p':
                vsnprintfsb_char(buffer, '0');
                vsnprintfsb_char(buffer, 'x');
                vsnprintfsb_unsigned(buffer, unsigned long, (unsigned long)va_arg(args, void *), 16, 0);
                break;
            default:
                assert(0);
                break;
        }
        ++format_offset;
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
