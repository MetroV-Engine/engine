#include <boost/ut.hpp>

#include <cstdint>
#include <map>
#include <optional>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

#include "ECS/Reflection/PrettyPrint.hpp"

using namespace boost::ut;

// The registry is a process-wide singleton: names are prefixed with "Pp" so
// they never collide with another test file's E_REFLECT.
namespace {
    enum class PpMode { A, B };

    struct PpVec { float x; float y; };

    struct PpEmpty {};

    struct PpAll {
        std::string name;
        bool flag;
        std::int8_t small;
        char letter;
        PpMode mode;
        PpVec pos;
        std::vector<int> list;
        std::map<std::string, int> dict;
        std::optional<int> maybe;
        std::vector<int> empty;
    };

    struct PpBag { std::vector<PpVec> points; };

    struct PpOpaque { int a; };

    struct PpStreamable { int v; };
    std::ostream& operator<<(std::ostream& os, const PpStreamable& s) {
        return os << "S(" << s.v << ")";
    }
}

E_REFLECT(PpVec)
    E_FIELD(x, float)
    E_FIELD(y, float)
E_END

E_REFLECT(PpEmpty)
E_END

E_REFLECT(PpAll)
    E_FIELD(name, std::string)
    E_FIELD(flag, bool)
    E_FIELD(small, std::int8_t)
    E_FIELD(letter, char)
    E_FIELD(mode, PpMode)
    E_FIELD(pos, PpVec)
    E_FIELD(list, std::vector<int>)
    E_FIELD(dict, std::map<std::string, int>)
    E_FIELD(maybe, std::optional<int>)
    E_FIELD(empty, std::vector<int>)
E_END

E_REFLECT(PpBag)
    E_FIELD(points, std::vector<PpVec>)
E_END

using ECS::reflect::PrettyPrintOptions;
using ECS::reflect::prettyPrint;
using ECS::reflect::Ref;
using ECS::reflect::toPrettyString;

namespace {
    const PrettyPrintOptions oneLine{.indent = 0};

void test_single_line_struct() {
    "a struct prints on one line with indent 0"_test = [] {
        PpVec vec{1.5f, -2.0f};

        expect(toPrettyString(Ref::make(vec), oneLine) == std::string("PpVec { x: 1.5, y: -2 }"));
    };
}

void test_multi_line_struct() {
    "a struct prints one field per line by default"_test = [] {
        PpVec vec{1.5f, -2.0f};

        expect(toPrettyString(Ref::make(vec)) == std::string("PpVec {\n  x: 1.5,\n  y: -2\n}"));
    };
}

void test_every_kind() {
    "every kind of value prints in its own format"_test = [] {
        PpAll all{"a\"b", true, -5, 'c', PpMode::B, {1.0f, 2.0f}, {1, 2, 3}, {{"k", 1}},
                  std::nullopt, {}};

        expect(toPrettyString(Ref::make(all), oneLine) == std::string(
            "PpAll { name: \"a\\\"b\", flag: true, small: -5, letter: 'c', mode: 1, "
            "pos: PpVec { x: 1, y: 2 }, list: [ 1, 2, 3 ], dict: { \"k\": 1 }, "
            "maybe: none, empty: [] }"));
    };
}

void test_nested_indentation() {
    "nested values are indented one level deeper"_test = [] {
        PpBag bag{{{1.0f, 2.0f}}};

        expect(toPrettyString(Ref::make(bag)) == std::string(
            "PpBag {\n"
            "  points: [\n"
            "    PpVec {\n"
            "      x: 1,\n"
            "      y: 2\n"
            "    }\n"
            "  ]\n"
            "}"));
    };
}

void test_compact_leaf_sequences() {
    "a sequence of leaves stays on one line when indenting"_test = [] {
        PpAll all{"n", false, 0, 'x', PpMode::A, {0.0f, 0.0f}, {1, 2, 3}, {}, 4, {}};
        const std::string text = toPrettyString(Ref::make(all));

        expect(text.find("\n  list: [ 1, 2, 3 ],\n") != std::string::npos);
        expect(text.find("\n  empty: []\n") != std::string::npos);
        expect(text.find("\n  maybe: 4,\n") != std::string::npos);
    };
}

void test_compact_leaf_sequences_can_be_disabled() {
    "compactLeafSequences = false prints one element per line"_test = [] {
        std::vector<int> list{1, 2};

        expect(toPrettyString(Ref::make(list)) == std::string("[ 1, 2 ]"));
        expect(toPrettyString(Ref::make(list), {.compactLeafSequences = false}) ==
               std::string("[\n  1,\n  2\n]"));
    };
}

void test_compact_leaf_sequences_with_max_elements() {
    "a compact sequence still honours maxElements"_test = [] {
        std::vector<int> list{1, 2, 3};

        expect(toPrettyString(Ref::make(list), {.maxElements = 1}) ==
               std::string("[ 1, ... (2 more) ]"));
    };
}

void test_custom_indent_width() {
    "the indent option sets the spaces per level"_test = [] {
        PpVec vec{1.0f, 2.0f};

        expect(toPrettyString(Ref::make(vec), {.indent = 4}) ==
               std::string("PpVec {\n    x: 1,\n    y: 2\n}"));
    };
}

void test_show_types() {
    "showTypes annotates leaves and prefixes containers"_test = [] {
        PpVec vec{1.5f, -2.0f};
        std::vector<int> list{1};
        const PrettyPrintOptions typed{.indent = 0, .showTypes = true};

        expect(toPrettyString(Ref::make(vec), typed) ==
               std::string("PpVec { x: 1.5 (float), y: -2 (float) }"));
        expect(toPrettyString(Ref::make(list), typed) == std::string("vector<int32> [ 1 (int32) ]"));
    };
}

void test_max_elements() {
    "maxElements truncates long containers"_test = [] {
        std::vector<int> list{1, 2, 3, 4, 5};
        std::map<std::string, int> dict{{"a", 1}, {"b", 2}};

        expect(toPrettyString(Ref::make(list), {.indent = 0, .maxElements = 2}) ==
               std::string("[ 1, 2, ... (3 more) ]"));
        expect(toPrettyString(Ref::make(dict), {.indent = 0, .maxElements = 1}) ==
               std::string("{ \"a\": 1, ... (1 more) }"));
        expect(toPrettyString(Ref::make(list), {.indent = 0, .maxElements = 9}) ==
               std::string("[ 1, 2, 3, 4, 5 ]"));
    };
}

void test_max_elements_keeps_struct_fields() {
    "maxElements never truncates struct fields"_test = [] {
        PpBag bag{{{1.0f, 2.0f}, {3.0f, 4.0f}}};

        expect(toPrettyString(Ref::make(bag), {.indent = 0, .maxElements = 1}) ==
               std::string("PpBag { points: [ PpVec { x: 1, y: 2 }, ... (1 more) ] }"));
    };
}

void test_engaged_optional() {
    "an engaged optional prints its value"_test = [] {
        std::optional<PpVec> maybe = PpVec{3.0f, 4.0f};

        expect(toPrettyString(Ref::make(maybe), oneLine) == std::string("PpVec { x: 3, y: 4 }"));
    };
}

void test_opaque_values() {
    "an opaque value uses operator<< when available, its type name otherwise"_test = [] {
        PpOpaque opaque{1};
        PpStreamable streamable{3};

        expect(toPrettyString(Ref::make(streamable)) == std::string("S(3)"));
        expect(toPrettyString(Ref::make(opaque)) == "<" + Ref::make(opaque).type().name + ">");
    };
}

void test_empty_struct_and_invalid_ref() {
    "an empty struct prints {} and an invalid Ref prints <invalid>"_test = [] {
        PpEmpty empty;

        expect(toPrettyString(Ref::make(empty)) == std::string("PpEmpty {}"));
        expect(toPrettyString(Ref{}) == std::string("<invalid>"));
    };
}

void test_string_escaping() {
    "strings are quoted and escaped"_test = [] {
        std::string text = "a\nb\\c";

        expect(toPrettyString(Ref::make(text)) == std::string("\"a\\nb\\\\c\""));
    };
}

void test_escaping_tabs_and_char_quotes() {
    "tabs and the quote character of a char are escaped"_test = [] {
        std::string text = "a\tb";
        char quote = '\'';

        expect(toPrettyString(Ref::make(text)) == std::string("\"a\\tb\""));
        expect(toPrettyString(Ref::make(quote)) == std::string("'\\''"));
    };
}

void test_escaping_carriage_return() {
    "a carriage return is escaped as \\r"_test = [] {
        std::string text = "a\rb";

        expect(toPrettyString(Ref::make(text)) == std::string("\"a\\rb\""));
    };
}

void test_escaping_other_control_characters() {
    "other control characters, embedded NUL and DEL are escaped as \\xHH"_test = [] {
        std::string text("\0|\x01|\x1B|\x7F|\x1f", 9);

        expect(toPrettyString(Ref::make(text)) ==
               std::string("\"\\x00|\\x01|\\x1B|\\x7F|\\x1F\""));
    };
}

void test_escaping_named_escapes_unchanged() {
    "\\n, \\t, backslash and the quote character keep their escapes"_test = [] {
        std::string text = "\n\t\\\"'";
        char quote = '\'';
        char doubleQuote = '"';

        expect(toPrettyString(Ref::make(text)) == std::string("\"\\n\\t\\\\\\\"'\""));
        expect(toPrettyString(Ref::make(quote)) == std::string("'\\''"));
        expect(toPrettyString(Ref::make(doubleQuote)) == std::string("'\"'"));
    };
}

void test_escaping_keeps_utf8() {
    "UTF-8 bytes are printed unchanged"_test = [] {
        std::string text = "\xC3\xA9 \xE6\x97\xA5\xE6\x9C\xAC";

        expect(toPrettyString(Ref::make(text)) ==
               std::string("\"\xC3\xA9 \xE6\x97\xA5\xE6\x9C\xAC\""));
    };
}

void test_escaping_empty_string() {
    "an empty string prints as two quotes"_test = [] {
        std::string text;

        expect(toPrettyString(Ref::make(text)) == std::string("\"\""));
    };
}

void test_escaping_control_chars() {
    "control characters in a char are escaped"_test = [] {
        char cr = '\r';
        char soh = '\x01';
        char nul = '\0';
        char plain = 'a';

        expect(toPrettyString(Ref::make(cr)) == std::string("'\\r'"));
        expect(toPrettyString(Ref::make(soh)) == std::string("'\\x01'"));
        expect(toPrettyString(Ref::make(nul)) == std::string("'\\x00'"));
        expect(toPrettyString(Ref::make(plain)) == std::string("'a'"));
    };
}

void test_escaping_hex_digit_after_escape() {
    "a hex digit right after a \\xHH escape is kept apart"_test = [] {
        std::string text = "\x01" "AF";

        expect(toPrettyString(Ref::make(text)) == std::string("\"\\x01AF\""));
    };
}

void test_escaping_keeps_stream_flags() {
    "escaping leaves the stream's formatting flags untouched"_test = [] {
        std::string text = "\x1B";
        std::ostringstream os;
        const auto flags = os.flags();
        prettyPrint(os, Ref::make(text));
        os << ' ' << 255;

        expect(os.str() == std::string("\"\\x1B\" 255"));
        expect(os.flags() == flags);
        expect(os.fill() == ' ');
    };
}

void test_compact_sequences_of_enums_and_opaques() {
    "sequences of enums and of opaque values are leaves, printed on one line"_test = [] {
        std::vector<PpMode> modes{PpMode::A, PpMode::B};
        std::vector<PpStreamable> values{{1}, {2}};

        expect(toPrettyString(Ref::make(modes)) == std::string("[ 0, 1 ]"));
        expect(toPrettyString(Ref::make(values)) == std::string("[ S(1), S(2) ]"));
    };
}

void test_stream_operator() {
    "operator<< on a Ref prints with the default options"_test = [] {
        PpVec vec{1.0f, 2.0f};
        std::ostringstream os;
        os << Ref::make(vec);

        expect(os.str() == toPrettyString(Ref::make(vec)));
    };
}

}

void run_pretty_print_tests() {
    test_single_line_struct();
    test_multi_line_struct();
    test_every_kind();
    test_nested_indentation();
    test_compact_leaf_sequences();
    test_compact_leaf_sequences_can_be_disabled();
    test_compact_leaf_sequences_with_max_elements();
    test_custom_indent_width();
    test_show_types();
    test_max_elements();
    test_max_elements_keeps_struct_fields();
    test_engaged_optional();
    test_opaque_values();
    test_empty_struct_and_invalid_ref();
    test_string_escaping();
    test_escaping_tabs_and_char_quotes();
    test_escaping_carriage_return();
    test_escaping_other_control_characters();
    test_escaping_named_escapes_unchanged();
    test_escaping_keeps_utf8();
    test_escaping_empty_string();
    test_escaping_control_chars();
    test_escaping_hex_digit_after_escape();
    test_escaping_keeps_stream_flags();
    test_compact_sequences_of_enums_and_opaques();
    test_stream_operator();
}
