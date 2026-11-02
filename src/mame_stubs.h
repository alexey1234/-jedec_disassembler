#pragma once

#include <iostream>
#include <cstdlib>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <stdarg.h>
#include <strings.h>
#include <stdint.h>
#include <cstdint>
#include <stdio.h>
#include <stdarg.h>
#include <cstddef>
#include <tuple> // Для std::tie в jedparse.cpp
#include <system_error> // Для std::error_condition

// Базовые типы MAME
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

namespace util {

    // Базовый интерфейс чтения с методами навигации
    class random_read {
    public:
        virtual ~random_read() = default;
        virtual std::size_t read(void *buffer, std::size_t length) = 0;
        virtual std::size_t read(void *buffer, uint64_t offset, std::size_t length) = 0;
        
        // Новые методы, которые запрашивает jedparse.cpp
        virtual std::error_condition seek(int64_t offset, int whence) = 0;
        virtual std::error_condition tell(uint64_t &position) = 0;
    };

    class read_stream : public random_read {
    public:
        virtual ~read_stream() = default;
    };

    // Обновленная обертка
    class stdio_read_wrapper : public read_stream {
    private:
        FILE* m_file;
    public:
        stdio_read_wrapper(FILE* f) : m_file(f) {}
        
        virtual ~stdio_read_wrapper() {
            reset();
        }

        virtual std::size_t read(void *buffer, std::size_t length) override {
            if (!m_file) return 0;
            return std::fread(buffer, 1, length, m_file);
        }

        virtual std::size_t read(void *buffer, uint64_t offset, std::size_t length) override {
            if (!m_file) return 0;
            std::fseek(m_file, offset, SEEK_SET);
            return std::fread(buffer, 1, length, m_file);
        }

        // Реализация seek
        virtual std::error_condition seek(int64_t offset, int whence) override {
            if (!m_file || std::fseek(m_file, offset, whence) != 0) {
                return std::make_error_condition(std::errc::bad_file_descriptor);
            }
            return std::error_condition(); // успех
        }

        // Реализация tell
        virtual std::error_condition tell(uint64_t &position) override {
            if (!m_file) return std::make_error_condition(std::errc::bad_file_descriptor);
            long pos = std::ftell(m_file);
            if (pos < 0) return std::make_error_condition(std::errc::bad_file_descriptor);
            position = static_cast<uint64_t>(pos);
            return std::error_condition(); // успех
        }
        
        void reset() {
            if (m_file) {
                std::fclose(m_file);
                m_file = nullptr;
            }
        }

        read_stream& operator*() { return *this; }
        operator bool() const { return m_file != nullptr; }
    };

    inline stdio_read_wrapper stdio_read(FILE* f) {
        return stdio_read_wrapper(f);
    }

    // Глобальный хелпер, который вызывает jedparse.cpp (строки 210, 229, 265...)
    template <typename T>
    inline std::pair<std::error_condition, std::size_t> read(T &stream, void *buffer, std::size_t length) {
        std::size_t actual = stream.read(buffer, length);
        if (actual == 0 && length > 0) {
            return std::make_pair(std::make_error_condition(std::errc::io_error), actual);
        }
        return std::make_pair(std::error_condition(), actual);
    }
}


// Заглушки для функций логирования MAME
inline void osd_printf_error(const char *format, ...) {
    va_list ap; va_start(ap, format); vfprintf(stderr, format, ap); va_end(ap);
}
inline void osd_printf_warning(const char *format, ...) {
    va_list ap; va_start(ap, format); vfprintf(stderr, format, ap); va_end(ap);
}
inline void osd_printf_info(const char *format, ...) {
    va_list ap; va_start(ap, format); vprintf(format, ap); va_end(ap);
}
inline void osd_printf_verbose(const char *format, ...) {
    va_list ap; va_start(ap, format); vprintf(format, ap); va_end(ap);
}
#include <strings.h> // Для strcasecmp

// Эмуляция функций из corestr.h, используемых в MAME
inline int core_stricmp(const char *s1, const char *s2) {
    return strcasecmp(s1, s2);
}

inline int core_strnicmp(const char *s1, const char *s2, size_t n) {
    return strncasecmp(s1, s2, n);
}

