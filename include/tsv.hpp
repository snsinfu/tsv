// Copyright snsinfu 2021.
// Distributed under the Boost Software License, Version 1.0.
//
// Permission is hereby granted, free of charge, to any person or organization
// obtaining a copy of the software and accompanying documentation covered by
// this license (the "Software") to use, reproduce, display, distribute,
// execute, and transmit the Software, and to prepare derivative works of the
// Software, and to permit third-parties to whom the Software is furnished to
// do so, all subject to the following:
//
// The copyright notices in the Software and this entire statement, including
// the above license grant, this restriction and the following disclaimer,
// must be included in all copies of the Software, in whole or in part, and
// all derivative works of the Software, unless such copies or derivative
// works are solely in the form of machine-executable object code generated by
// a source language processor.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
// SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
// FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

#ifndef INCLUDED_SNSINFU_TSV_HPP
#define INCLUDED_SNSINFU_TSV_HPP

#include <charconv>
#include <cstddef>
#include <istream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>


namespace tsv
{
    /** Holds options to control how a TSV input is handled. */
    struct options
    {
        /** A character used to split fields. */
        char delimiter = '\t';

        /** True to skip the first non-comment line. */
        bool header = true;

        /** Lines starting with this character are skipped. */
        char comment = 0;
    };

    /**
     * Loads tab-separated values from each line of an input.
     *
     * @param input is a stream containing a tab-separated document.
     * @param opts control how the parser behaves.
     *
     * @returns A vector of loaded records.
     */
    template<typename Record>
    std::vector<Record> load(std::istream& input, tsv::options const& opts = {});

    template<typename Record>
    std::vector<Record> load(std::istream&& input, tsv::options const& opts = {})
    {
        return tsv::load<Record>(input, opts);
    }

    /**
     * Traits class for customizing how values of type T are parsed. Default
     * implementations for string, char and numeric types are defined in this
     * library.
     */
    template<typename T>
    struct conversion;

    /** Base class for reporting an error on parsing an input. */
    class error : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;

        /** Content of the line where the error has occured, if available. */
        std::string line;

        /**
         * 1-based line number in an input where the error has occured. This is
         * set to zero if a line number is not available.
         */
        std::size_t line_number = 0;

        /**
         * Describes the error in detail. This function allocates memory and
         * thus can throw an exception in an out-of-memory case.
         */
        std::string describe() const
        {
            std::string message = what();

            if (line_number) {
                message += " (at line ";
                message += std::to_string(line_number);
                message += ")";
            }

            if (!line.empty()) {
                message += ": \"";
                message += line;
                message += "\"";
            }

            return message;
        }
    };

    /** An exception thrown when an input line has unexpected number of fields. */
    class format_error : public tsv::error
    {
    public:
        using tsv::error::error;

        static inline char const* const missing_header =
            "header is expected but not seen";
        static inline char const* const missing_field =
            "insufficient number of fields";
        static inline char const* const excess_field =
            "excess fields";
    };

    /** An exception thrown when a text is not parseable as a value. */
    class parse_error : public tsv::error
    {
    public:
        using tsv::error::error;

        static inline char const* const unknown =
            "parse error";
        static inline char const* const out_of_range =
            "value out of range";
        static inline char const* const leftover =
            "excess character(s) at the end of a field";
    };

    /** An exception thrown when reading from an input fails. */
    class io_error : public tsv::error
    {
    public:
        using tsv::error::error;

        static inline char const* const unknown =
            "input error";
    };

    /** An exception thrown when validation fails on a record. */
    class validation_error : public tsv::error
    {
    public:
        using tsv::error::error;
    };

    /**
     * Throws `tsv::validation_error` if a predicate is false.
     *
     * Use `tsv::check()` function to validate the values of the fields in your
     * record structure in `validate()` member function.
     *
     * ```
     * struct my_record
     * {
     *     unsigned row;
     *     unsigned column;
     *     double value;
     *
     *     void validate() const
     *     {
     *         tsv::check(
     *             row < column,
     *             "row index must be smaller than column index"
     *         );
     *         tsv::check(value >= 0, "value must be non-negative);
     *     }
     * };
     * ```
     *
     * @param pred  The condition to test.
     * @param message  A message that describes the expected condition.
     */
    inline
    void check(bool pred, std::string const& message)
    {
        if (!pred) {
            throw tsv::validation_error{message};
        }
    }
}

// STRUCTURE REFLECTION ------------------------------------------------------

namespace tsv::detail
{
    /** Dummy convertible-to-anything type used to probe structure fields. */
    struct any
    {
        template<typename T>
        operator T() const;
    };

    /**
     * Traits for detecting the number of fields in an aggregate structure.
     *
     * The second parameter is `std::void_t` probing an aggregate initializer.
     * The third parameter is the type list of the initializers.
     */
    template<typename Record, typename = void, typename... Inits>
    struct record_size
    {
        static constexpr std::size_t value = 0;
    };

    template<typename Record, typename... Inits>
    struct record_size<
        Record,
        std::void_t<decltype(Record{detail::any{}, Inits{}...})>,
        Inits...
    >
    {
        static constexpr std::size_t value =
            record_size<Record, void, detail::any, Inits...>::value + 1;
    };

    /** Detects the number of fields in an aggregate structure. */
    template<typename Record>
    inline constexpr std::size_t record_size_v = detail::record_size<Record>::value;

    /** A dummy type to hold a template type list. */
    template<typename...>
    struct type_list {};

    /** Creates type_list from values. */
    template<typename... Ts>
    detail::type_list<Ts...> type_list_of(Ts const&...);

    template<std::size_t N>
    using size = std::integral_constant<std::size_t, N>;

    /** Returns the type_list of the fields of a structure. */
    template<typename Record>
    auto splat(Record const&, detail::size<0>)
    {
        return detail::type_list<>{};
    }

    // Generate overloads for hard-coded number of fields.

#define TSV_SPLAT(N, ...)                               \
    template<typename Record>                           \
    auto splat(Record const& record, detail::size<N>)   \
    {                                                   \
        auto [__VA_ARGS__] = record;                    \
        return detail::type_list_of(__VA_ARGS__);       \
    }

    TSV_SPLAT(1, a1)
    TSV_SPLAT(2, a1, a2)
    TSV_SPLAT(3, a1, a2, a3)
    TSV_SPLAT(4, a1, a2, a3, a4)
    TSV_SPLAT(5, a1, a2, a3, a4, a5)
    TSV_SPLAT(6, a1, a2, a3, a4, a5, a6)
    TSV_SPLAT(7, a1, a2, a3, a4, a5, a6, a7)
    TSV_SPLAT(8, a1, a2, a3, a4, a5, a6, a7, a8)
    TSV_SPLAT(9, a1, a2, a3, a4, a5, a6, a7, a8, a9)
    TSV_SPLAT(10, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10)
    TSV_SPLAT(11, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11)
    TSV_SPLAT(12, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12)
    TSV_SPLAT(13, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13)
    TSV_SPLAT(14, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14)
    TSV_SPLAT(15, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15)
    TSV_SPLAT(16, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16)
    TSV_SPLAT(17, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17)
    TSV_SPLAT(18, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18)
    TSV_SPLAT(19, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19)
    TSV_SPLAT(20, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20)
    TSV_SPLAT(21, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21)
    TSV_SPLAT(22, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22)
    TSV_SPLAT(23, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23)
    TSV_SPLAT(24, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24)
    TSV_SPLAT(25, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25)
    TSV_SPLAT(26, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26)
    TSV_SPLAT(27, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27)
    TSV_SPLAT(28, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28)
    TSV_SPLAT(29, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29)
    TSV_SPLAT(30, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30)
    TSV_SPLAT(31, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31)
    TSV_SPLAT(32, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32)

#undef TSV_SPLAT

    /** Returns a type_list of the fields of an aggregate structure. */
    template<typename Record>
    using field_type_list = decltype(
        detail::splat(
            std::declval<Record>(),
            detail::size<detail::record_size_v<Record>>{}
        )
    );
}

// CONVERSION ----------------------------------------------------------------

namespace tsv::detail
{
    /**
     * Default implementation for the tsv::conversion traits.
     *
     * This fallback definition uses the stream input operator (>>) to read a
     * value from a stringstream. This is slow.
     */
    template<typename T, typename = void>
    struct default_conversion
    {
        static T parse(std::string_view text)
        {
            using char_traits = std::istringstream::traits_type;

            std::istringstream stream{std::string{text}};
            T value;
            stream >> value;

            if (stream.fail() && !stream.eof()) {
                throw tsv::parse_error{tsv::parse_error::unknown};
            }

            if (stream.get() != char_traits::eof()) {
                throw tsv::parse_error{tsv::parse_error::leftover};
            }

            return value;
        }
    };

    /** An optimized implementation for numeric types using std::from_chars. */
    template<typename T>
    struct default_conversion<
        T,
        std::void_t<
            decltype(std::from_chars(nullptr, nullptr, std::declval<T&>()))
        >
    >
    {
        static T parse(std::string_view text)
        {
            auto const begin = text.data();
            auto const end = text.data() + text.size();

            T value;
            auto const [remain, ec] = std::from_chars(begin, end, value);

            if (ec != std::errc{}) {
                throw tsv::parse_error{
                    ec == std::errc::result_out_of_range
                        ? tsv::parse_error::out_of_range
                        : tsv::parse_error::unknown
                };
            }

            if (remain != end) {
                throw tsv::parse_error{tsv::parse_error::leftover};
            }

            return value;
        }
    };

    /** Single-character token. */
    template<>
    struct default_conversion<char, void>
    {
        static char parse(std::string_view text)
        {
            if (text.size() != 1) {
                throw tsv::parse_error{tsv::parse_error::unknown};
            }
            return text.front();
        }
    };

    /** String token. */
    template<>
    struct default_conversion<std::string, void>
    {
        static std::string parse(std::string_view text)
        {
            return std::string{text};
        }
    };
}

namespace tsv
{
    template<typename T>
    struct conversion : detail::default_conversion<T> {};
}

// PARSER --------------------------------------------------------------------

namespace tsv::detail
{
    /**
     * Parses a value of type T from a string. Actual parsing is done by the
     * tsv::conversion trait.
     */
    template<typename T>
    T parse(std::string_view text)
    {
        return tsv::conversion<T>::parse(text);
    }

    /**
     * Splits a string at a delimiter and consumes the first part.
     *
     * @param text  String to split. The string will be trimmed up to the
     *   first occurrence of the delimiter (or the end).
     * @param delim  Delimiter character.
     *
     * @returns The substring of `text` that precedes the first occurence
     *   of `delim`. Returns the entire input string if delim is not in the
     *   input string.
     */
    inline
    std::string_view split_consume(std::string_view& text, char delim)
    {
        if (text.empty()) {
            throw tsv::format_error{tsv::format_error::missing_field};
        }

        auto const pos = text.find(delim);
        if (pos != std::string_view::npos) {
            auto const token = text.substr(0, pos);
            text = text.substr(pos + 1);
            return token;
        } else {
            auto const token = text;
            text = text.substr(text.size());
            return token;
        }
    }

    /**
     * Parses a structure out of a delimited text string. Record is the type of
     * the structure to return and Ts... is the list of field types.
     */
    template<typename Record, typename... Ts>
    Record parse_record(std::string_view text, char delim, detail::type_list<Ts...>)
    {
        Record record = {
            detail::parse<Ts>(detail::split_consume(text, delim))...
        };
        if (!text.empty()) {
            throw tsv::format_error{tsv::format_error::excess_field};
        }
        return record;
    }

    /** Class for reading lines from a stream with one-line lookahead. */
    class line_reader
    {
    public:
        explicit line_reader(std::istream& input)
            : _input{input}
        {
        }

        /**
         * Reads next line. Returns a view of the internal buffer containing
         * the content of the line, or nothing on reaching EOF.
         */
        std::optional<std::string_view> consume()
        {
            if (!ensure_line()) {
                return std::nullopt;
            }
            _available = false;
            return _line;
        }

        /**
         * Looks next line ahead. Returns a view of the internal buffer
         * containing the content of the line, or nothing on reaching EOF.
         */
        std::optional<std::string_view> peek()
        {
            if (!ensure_line()) {
                return std::nullopt;
            }
            return _line;
        }

        /**
         * Returns the current line number (one-based). Returns zero if any
         * line has not been read yet.
         */
        std::size_t line_number() const
        {
            return _line_number;
        }

    private:
        /**
         * Ensures that `_line` contains the content of the next line. It does
         * not read from the stream if `_line` is already filled by a previous
         * lookahead. Returns true on success, or false on reaching EOF.
         */
        bool ensure_line()
        {
            if (_available) {
                return true;
            }

            if (!std::getline(_input, _line)) {
                if (_input.eof()) {
                    return false;
                }
                throw tsv::io_error{tsv::io_error::unknown};
            }

            _line_number++;
            _available = true;

            return true;
        }

    private:
        std::istream& _input;
        std::string _line;
        std::size_t _line_number = 0;
        bool _available = false;
    };

    /** Class for incrementally reading TSV rows from a stream. */
    class parser
    {
    public:
        /** Constructs a TSV parser with given input and delimiter. */
        explicit parser(std::istream& input, char delim)
            : _source{input}, _delim{delim}
        {
        }

        /** Skips comment and empty lines, if any. */
        void skip_comment(char prefix)
        {
            for (;;) {
                std::string_view line;

                if (auto maybe_line = _source.peek()) {
                    line = *maybe_line;
                } else {
                    break;
                }

                if (line.empty() || line.front() == prefix) {
                    _source.consume();
                } else {
                    break;
                }
            }
        }

        /**
         * Parses the next line as textual fields. The fields are appended to
         * the end of the vector. Returns true on success or false on reaching
         * EOF.
         */
        bool parse_fields(std::vector<std::string>& fields)
        {
            std::string_view line;

            if (auto maybe_line = _source.consume()) {
                line = *maybe_line;
            } else {
                return false;
            }

            for (auto remain = line; !remain.empty(); ) {
                fields.push_back(std::string{detail::split_consume(remain, _delim)});
            }

            return true;
        }

        /**
         * Parses the next line as a structure. The fields are parsed and
         * assigned to the members of the given structure object. Returns true
         * on success or false on reaching EOF.
         */
        template<typename Record>
        bool parse_record(Record& record)
        {
            std::string_view line;

            if (auto maybe_line = _source.consume()) {
                line = *maybe_line;
            } else {
                return false;
            }

            try {
                detail::field_type_list<Record> field_types;
                record = detail::parse_record<Record>(line, _delim, field_types);
            } catch (tsv::error& err) {
                err.line = line;
                err.line_number = _source.line_number();
                throw;
            }

            return true;
        }

    private:
        detail::line_reader _source;
        char const _delim;
    };
}

// VALIDATION ----------------------------------------------------------------

namespace tsv::detail
{
    /** Validates the values assigned to the fields of a record. */
    template<
        typename Record,
        typename = decltype(std::declval<Record const&>().validate())
    >
    void validate(Record const& record)
    {
        record.validate();
    }

    template<typename Record, typename... Dummy>
    void validate(Record const&, Dummy...)
    {
    }
}

namespace tsv
{
    template<typename Record>
    std::vector<Record> load(std::istream& input, tsv::options const& opts)
    {
        std::vector<Record> records;
        detail::parser parser{input, opts.delimiter};

        parser.skip_comment(opts.comment);

        if (opts.header) {
            std::vector<std::string> header;
            if (!parser.parse_fields(header)) {
                throw tsv::format_error{tsv::format_error::missing_header};
            }
        }

        for (;;) {
            parser.skip_comment(opts.comment);

            Record record;
            if (!parser.parse_record<Record>(record)) {
                break;
            }
            detail::validate(record);

            records.push_back(std::move(record));
        }

        return records;
    }
}

#endif
