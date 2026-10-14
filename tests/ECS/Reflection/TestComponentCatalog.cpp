#include <boost/ut.hpp>

#include <iostream>
#include <sstream>
#include <streambuf>
#include <string>
#include <string_view>

#include "ECS/Reflection/Reflection.hpp"

using namespace boost::ut;

// The catalog and the TypeRegistry are process-wide: names are prefixed with
// "Cc" so they never collide with another test file's E_REFLECT.
namespace {
    struct CcFresh { int a = 1; };       // not reflected: added by hand
    struct CcManual { int b = 2; };      // not reflected: registered by hand
    struct CcReflected { int c; };
    struct CcConstant { const int d; };  // reflected, but cannot live in a pool

    namespace first { struct CcDup { int x; }; }
    namespace second { struct CcDup { int y; }; }
}

E_REFLECT(CcReflected)
    E_FIELD(c, int)
E_END

E_REFLECT(CcConstant)
    E_FIELD(d, const int)
E_END

E_REFLECT(first::CcDup)
    E_FIELD(x, int)
E_END

// Hand-written Descriptor reusing first::CcDup's name, without the automatic
// registration of E_REFLECT: the tests trigger the collision themselves.
template<> struct ECS::reflect::Descriptor<second::CcDup> {
    static constexpr std::string_view name = "first::CcDup";
    template<typename Visitor> static void forEach(Visitor&) {}
};

using ECS::reflect::detail::ComponentCatalog;

namespace {
    /** @brief Redirects std::cerr into a string for its lifetime, restored even if a test throws. */
    class CerrCapture {
        public:
            CerrCapture() : _previous(std::cerr.rdbuf(_buffer.rdbuf())) {}
            ~CerrCapture() { std::cerr.rdbuf(_previous); }
            CerrCapture(const CerrCapture&) = delete;
            CerrCapture& operator=(const CerrCapture&) = delete;

            [[nodiscard]] std::string str() const { return _buffer.str(); }

        private:
            std::ostringstream _buffer;
            std::streambuf* _previous;
    };

void test_record_error_reports_on_stderr() {
    "recordError writes one line to stderr and appends the error"_test = [] {
        auto& catalog = ComponentCatalog::instance();
        const auto before = catalog.errors().size();
        std::string output;
        {
            const CerrCapture capture;
            catalog.recordError("CcName", "what happened");
            output = capture.str();
        }

        expect(output == std::string("[reflection] CcName: what happened\n"));
        expect((catalog.errors().size() == before + 1) >> fatal);
        expect(catalog.errors().back() == std::string("CcName: what happened"));
    };
}

void test_successful_registration_reports_nothing() {
    "a successful registration writes nothing and records no error"_test = [] {
        auto& catalog = ComponentCatalog::instance();
        const auto before = catalog.errors().size();
        std::string output;
        {
            const CerrCapture capture;
            catalog.add<CcFresh>();
            ECS::reflect::registerComponentType<CcManual>();
            output = capture.str();
        }

        expect(output.empty()) << output;
        expect(catalog.errors().size() == before);
        expect(catalog.find(ECS::componentId<CcFresh>()) != nullptr);
        expect(catalog.find(ECS::componentId<CcManual>()) != nullptr);
    };

    "registering an already registered reflected type again writes nothing"_test = [] {
        auto& catalog = ComponentCatalog::instance();
        const auto before = catalog.errors().size();
        std::string output;
        {
            const CerrCapture capture;
            ECS::reflect::detail::registerReflectedType<CcReflected>();   // component
            ECS::reflect::detail::registerReflectedType<CcConstant>();    // reflected only
            catalog.add<CcFresh>();                                        // idempotent
            output = capture.str();
        }

        expect(output.empty()) << output;
        expect(catalog.errors().size() == before);
    };
}

void test_name_collision_is_reported() {
    "a name collision in registerReflectedType is written to stderr and recorded"_test = [] {
        auto& catalog = ComponentCatalog::instance();
        const auto before = catalog.errors().size();
        std::string output;
        {
            const CerrCapture capture;
            ECS::reflect::detail::registerReflectedType<second::CcDup>();   // name already taken
            output = capture.str();
        }

        expect(output.rfind("[reflection] first::CcDup: ", 0) == 0_ul) << output;
        expect(output.ends_with('\n'));
        expect(output.find('\n') == output.size() - 1) << "exactly one line";
        expect((catalog.errors().size() == before + 1) >> fatal);
        expect(catalog.errors().back().rfind("first::CcDup: ", 0) == 0_ul);
        expect(output == "[reflection] " + catalog.errors().back() + "\n");
        expect(catalog.find(ECS::componentId<second::CcDup>()) == nullptr);
    };
}

void test_record_error_edge_cases() {
    "an empty type name still produces a well-formed line"_test = [] {
        auto& catalog = ComponentCatalog::instance();
        std::string output;
        {
            const CerrCapture capture;
            catalog.recordError("", "Cc no name");
            output = capture.str();
        }

        expect(output == std::string("[reflection] : Cc no name\n"));
        expect(catalog.errors().back() == std::string(": Cc no name"));
    };

    "an empty message still produces a well-formed line"_test = [] {
        auto& catalog = ComponentCatalog::instance();
        std::string output;
        {
            const CerrCapture capture;
            catalog.recordError("CcEmptyMessage", "");
            output = capture.str();
        }

        expect(output == std::string("[reflection] CcEmptyMessage: \n"));
        expect(catalog.errors().back() == std::string("CcEmptyMessage: "));
    };

    "errors keep the order in which they were recorded"_test = [] {
        auto& catalog = ComponentCatalog::instance();
        const auto before = catalog.errors().size();
        std::string output;
        {
            const CerrCapture capture;
            catalog.recordError("CcFirst", "1");
            catalog.recordError("CcSecond", "2");
            catalog.recordError("CcThird", "3");
            output = capture.str();
        }

        expect(output == std::string("[reflection] CcFirst: 1\n"
                                     "[reflection] CcSecond: 2\n"
                                     "[reflection] CcThird: 3\n"));
        const auto& errors = catalog.errors();
        expect((errors.size() == before + 3) >> fatal);
        expect(errors[before] == std::string("CcFirst: 1"));
        expect(errors[before + 1] == std::string("CcSecond: 2"));
        expect(errors[before + 2] == std::string("CcThird: 3"));
    };
}

}

void run_component_catalog_tests() {
    test_record_error_reports_on_stderr();
    test_successful_registration_reports_nothing();
    test_name_collision_is_reported();
    test_record_error_edge_cases();
}
