#pragma once

#include <algorithm>
#include <cstddef>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "Ref.hpp"

namespace ECS::reflect {
    /** @brief Formatting options of prettyPrint. */
    struct PrettyPrintOptions {
        /** @brief Spaces per nesting level; 0 prints everything on one line. */
        int indent = 2;
        /** @brief Appends the type name to leaf values and prefixes containers with it. */
        bool showTypes = false;
        /** @brief Maximum elements shown per Sequence/Map (struct fields are never cut); 0 shows them all. */
        std::size_t maxElements = 0;
        /** @brief Prints a Sequence of leaves (scalars, enums, opaques) on one line even when indenting. */
        bool compactLeafSequences = true;
    };

    namespace detail {
        // Recursive printer working on Ref/TypeInfo only.
        class PrettyPrinter {
            public:
                PrettyPrinter(std::ostream& os, const PrettyPrintOptions& options)
                    : _os(os), _options(options) {}

                void value(Ref ref, int depth) {
                    if (!ref.valid()) {
                        _os << "<invalid>";
                        return;
                    }
                    const TypeInfo& type = ref.type();
                    switch (type.kind) {
                        case TypeKind::Struct:   structure(ref, depth); break;
                        case TypeKind::Sequence: sequence(ref, depth); break;
                        case TypeKind::Map:      map(ref, depth); break;
                        case TypeKind::Optional: optional(ref, depth); break;
                        case TypeKind::Enum:     leaf(ref, [&] { _os << *ref.enumValue(); }); break;
                        case TypeKind::Scalar:
                        case TypeKind::Opaque:   leaf(ref, [&] { scalar(ref); }); break;
                    }
                }

            private:
                void structure(Ref ref, int depth) {
                    const auto& fields = ref.type().fields;
                    _os << ref.type().name << ' ';
                    block('{', '}', fields.size(), 0, depth, [&](std::size_t i) {
                        _os << fields[i].name << ": ";
                        value(ref.field(fields[i]), depth + 1);
                    });
                }

                void sequence(Ref ref, int depth) {
                    typePrefix(ref);
                    // An inlined sequence only holds leaves, so sequence() is never
                    // re-entered while _inline is set: no need to save/restore it.
                    _inline = _options.compactLeafSequences &&
                              isLeaf(*ref.type().sequenceOps->element);
                    block('[', ']', ref.size(), _options.maxElements, depth, [&](std::size_t i) {
                        value(ref.at(i), depth + 1);
                    });
                    _inline = false;
                }

                static bool isLeaf(const TypeInfo& type) noexcept {
                    return type.kind == TypeKind::Scalar || type.kind == TypeKind::Enum ||
                           type.kind == TypeKind::Opaque;
                }

                void map(Ref ref, int depth) {
                    std::vector<std::pair<Ref, Ref>> entries;
                    entries.reserve(ref.size());
                    ref.forEachEntry([&](Ref key, Ref entry) { entries.emplace_back(key, entry); });

                    typePrefix(ref);
                    block('{', '}', entries.size(), _options.maxElements, depth, [&](std::size_t i) {
                        value(entries[i].first, depth + 1);
                        _os << ": ";
                        value(entries[i].second, depth + 1);
                    });
                }

                void optional(Ref ref, int depth) {
                    if (ref.hasValue()) {
                        value(ref.value(), depth);
                    } else {
                        leaf(ref, [&] { _os << "none"; });
                    }
                }

                void scalar(Ref ref) {
                    if (const std::string* text = ref.tryAs<std::string>()) {
                        quoted(*text, '"');
                    } else if (const char* c = ref.tryAs<char>()) {
                        quoted(std::string_view(c, 1), '\'');
                    } else if (ref.type().print) {
                        ref.type().print(_os, ref.data());
                    } else {
                        _os << '<' << ref.type().name << '>';
                    }
                }

                template<typename Write>
                void leaf(Ref ref, Write&& write) {
                    write();
                    if (_options.showTypes) {
                        _os << " (" << ref.type().name << ')';
                    }
                }

                void typePrefix(Ref ref) {
                    if (_options.showTypes) {
                        _os << ref.type().name << ' ';
                    }
                }

                // Prints "{ a, b }" or, with indentation, one item per line.
                // limit caps the items shown (0 = all); structs never truncate their fields.
                template<typename Item>
                void block(char open, char close, std::size_t count, std::size_t limit, int depth,
                           Item&& item) {
                    if (count == 0) {
                        _os << open << close;
                        return;
                    }
                    const std::size_t shown = limit ? std::min(count, limit) : count;
                    _os << open;
                    for (std::size_t i = 0; i < shown; ++i) {
                        if (i > 0) {
                            _os << ',';
                        }
                        breakLine(depth + 1);
                        item(i);
                    }
                    if (shown < count) {
                        _os << ',';
                        breakLine(depth + 1);
                        _os << "... (" << count - shown << " more)";
                    }
                    breakLine(depth);
                    _os << close;
                }

                void breakLine(int depth) {
                    if (_options.indent > 0 && !_inline) {
                        _os << '\n' << std::string(static_cast<std::size_t>(depth * _options.indent), ' ');
                    } else {
                        _os << ' ';
                    }
                }

                void quoted(std::string_view text, char quote) {
                    _os << quote;
                    for (const char c : text) {
                        switch (c) {
                            case '\n': _os << "\\n"; break;
                            case '\t': _os << "\\t"; break;
                            case '\\': _os << "\\\\"; break;
                            default:
                                if (c == quote) {
                                    _os << '\\';
                                }
                                _os << c;
                        }
                    }
                    _os << quote;
                }

                std::ostream& _os;
                const PrettyPrintOptions& _options;
                bool _inline = false;
        };
    }

    /**
     * @brief Writes a readable, recursive representation of any value.
     *
     * Works on every TypeKind without knowing the type at compile time:
     * structs show their fields, sequences `[...]`, maps `{key: value}`,
     * empty optionals `none`, enums their integer value. Leaves use the
     * TypeInfo print operation (operator<<); a type without one prints as
     * `<TypeName>`. Meant for logs and debugging, not for serialization.
     */
    inline void prettyPrint(std::ostream& os, Ref value, const PrettyPrintOptions& options = {}) {
        detail::PrettyPrinter(os, options).value(value, 0);
    }

    /** @brief Same as prettyPrint, into a string. */
    [[nodiscard]] inline std::string toPrettyString(Ref value, const PrettyPrintOptions& options = {}) {
        std::ostringstream os;
        prettyPrint(os, value, options);
        return os.str();
    }

    /** @brief `std::cout << ref` prints it with the default options. */
    inline std::ostream& operator<<(std::ostream& os, Ref value) {
        prettyPrint(os, value);
        return os;
    }
}
