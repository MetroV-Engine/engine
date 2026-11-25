#include <boost/ut.hpp>

#include <clocale>
#include <string>

#include "Engine/Serialization/Serialization.hpp"
#include "Serialization/Support/Helpers.hpp"
#include "Serialization/Support/TestTypes.hpp"

using namespace boost::ut;
using ECS::reflect::Ref;
using serial::load;
using serial::Report;
using serial::serialize;
using namespace sertest;

namespace {
    // Switches the C locale for the scope of a test. Where the locale is not
    // installed, active() is false and the test has nothing to check.
    class ScopedLocale {
        public:
            explicit ScopedLocale(const char* name) {
                const char* previous = std::setlocale(LC_ALL, nullptr);
                _previous = previous ? previous : "C";
                _active = std::setlocale(LC_ALL, name) != nullptr;
            }
            ~ScopedLocale() { std::setlocale(LC_ALL, _previous.c_str()); }
            bool active() const { return _active; }

        private:
            std::string _previous;
            bool _active = false;
    };

    // Locales whose decimal separator is a comma.
    const char* const CommaLocales[] = {"fr_FR.utf8", "fr_FR.UTF-8", "de_DE.utf8", "de_DE.UTF-8"};
}

void run_locale_tests() {
    "floats are written with a dot whatever the locale"_test = [] {
        for (const char* name : CommaLocales) {
            ScopedLocale locale(name);
            if (!locale.active())
                continue;
            Actor actor;
            actor.scale = 0.1f;
            actor.speed = 2.5;
            const std::string text = serialize(Ref::make(actor));
            expect(text.find("0.1") != std::string::npos);
            expect(text.find("2.5") != std::string::npos);
            expect(text.find(',') == text.find(",\n"));   // only separators between fields
            expect(parseJson(text)["scale"] == 0.1);
            return;
        }
    };

    "floats read back exactly under a locale with a decimal comma"_test = [] {
        for (const char* name : CommaLocales) {
            ScopedLocale locale(name);
            if (!locale.active())
                continue;
            Actor source;
            source.scale = 0.1f;
            source.speed = 1.0 / 3.0;
            Actor copy;
            Report report;
            expect(load(Ref::make(copy), serialize(Ref::make(source)), &report));
            expect(report.warnings.empty());
            expect(copy.scale == 0.1f);
            expect(copy.speed == 1.0 / 3.0);
            return;
        }
    };

    "text written under the C locale reads the same under a decimal comma"_test = [] {
        for (const char* name : CommaLocales) {
            ScopedLocale locale(name);
            if (!locale.active())
                continue;
            Actor actor;
            Report report;
            expect(load(Ref::make(actor), R"({"scale":0.5,"speed":1.25})", &report));
            expect(report.warnings.empty());
            expect(actor.scale == 0.5f);
            expect(actor.speed == 1.25);
            return;
        }
    };

    "the C locale is restored after a test"_test = [] {
        const std::string before = std::setlocale(LC_ALL, nullptr);
        {
            ScopedLocale locale("fr_FR.utf8");
            (void)locale;
        }
        expect(std::string(std::setlocale(LC_ALL, nullptr)) == before);
    };
}
