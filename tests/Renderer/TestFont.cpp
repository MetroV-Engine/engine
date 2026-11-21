#include <boost/ut.hpp>

#include "Support/Expect.hpp"
#include "Renderer/Font/Font.hpp"
#include "Renderer/NullRenderer/NullRenderer.hpp"

#include <cmath>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

using namespace boost::ut;

namespace {

    constexpr float TestPixelHeight = 32.0f;

    constexpr std::string_view EAcute = "\xC3\xA9";
    constexpr std::string_view OeLigature = "\xC5\x93";
    constexpr std::string_view Euro = "\xE2\x82\xAC";
    constexpr std::string_view Emoji = "\xF0\x9F\x98\x80";

    std::vector<uint8_t> loadFontBytes()
    {
        std::ifstream file(RENDER_TEST_ASSETS_DIR "/DejaVuSans.ttf", std::ios::binary);
        return std::vector<uint8_t>(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>{});
    }

    void expectSameUvs(const gfx::TriangleMesh& a, const gfx::TriangleMesh& b)
    {
        expect(a.vertices.size() == b.vertices.size()) << fatal;
        for (size_t i = 0; i < a.vertices.size(); ++i) {
            expect(a.vertices[i].u == b.vertices[i].u);
            expect(a.vertices[i].v == b.vertices[i].v);
        }
    }

    // What each test starts from: the test font, loaded into an atlas on a NullRenderer.
    struct FontFixture {
            FontFixture()
            {
                bytes = loadFontBytes();
                if (!bytes.empty())
                    font = gfx::Font::create(renderer, bytes, TestPixelHeight);
            }

            bool rejected(const std::span<const uint8_t> ttf, const float pixelHeight)
            {
                return gfx::Font::create(renderer, ttf, pixelHeight) == nullptr;
            }

            gfx::TriangleMesh meshOf(const std::string_view text) const
            {
                gfx::TextDraw draw;
                draw.font = font.get();
                draw.text = text;

                gfx::TriangleMesh mesh;
                font->appendText(mesh, draw);
                return mesh;
            }

            void expectDifferentUvsFromQuestionMark(const std::string_view text) const
            {
                const gfx::TriangleMesh glyph = meshOf(text);
                const gfx::TriangleMesh question = meshOf("?");
                expect(glyph.vertices.size() == 4u) << fatal;
                expect(question.vertices.size() == 4u) << fatal;
                expect(glyph.vertices[0].u != question.vertices[0].u
                         || glyph.vertices[0].v != question.vertices[0].v);
            }

            gfx::NullRenderer renderer{ 800, 600 };
            std::vector<uint8_t> bytes;
            std::unique_ptr<gfx::Font> font;
    };

}

void run_font_tests()
{
    "font: the test font is readable"_test = [] {
        const auto bytes = loadFontBytes();
        expect(!bytes.empty()) << "could not read " << RENDER_TEST_ASSETS_DIR "/DejaVuSans.ttf" << fatal;
    };

    "font: creates and owns an atlas"_test = [] {
        FontFixture f;
        expect(!f.bytes.empty()) << "could not read " << RENDER_TEST_ASSETS_DIR "/DejaVuSans.ttf" << fatal;
        expect(f.font != nullptr) << fatal;
        auto& renderer = f.renderer;
        auto& font = f.font;

        const gfx::TextureHandle texture = font->texture();
        expect(texture != gfx::TextureHandle::Invalid);
        expect(renderer.textureSize(texture).x > 0.0f);

        font.reset();

        expect(renderer.textureSize(texture).x == 0.0f);
        expect(renderer.textureSize(texture).y == 0.0f);
    };

    "font: rejects bad input"_test = [] {
        FontFixture f;
        expect(!f.bytes.empty()) << "could not read " << RENDER_TEST_ASSETS_DIR "/DejaVuSans.ttf" << fatal;
        expect(f.font != nullptr) << fatal;
        auto& bytes = f.bytes;

        const std::vector<uint8_t> fourBytes(4, 0);
        const std::vector<uint8_t> zeros(64, 0);

        expect(f.rejected({}, TestPixelHeight));
        expect(f.rejected(fourBytes, TestPixelHeight));
        expect(f.rejected(zeros, TestPixelHeight));
        expect(f.rejected(bytes, 0.0f));
        expect(f.rejected(bytes, -1.0f));
    };

    "font: fails when atlas cannot fit"_test = [] {
        FontFixture f;
        expect(!f.bytes.empty()) << "could not read " << RENDER_TEST_ASSETS_DIR "/DejaVuSans.ttf" << fatal;
        expect(f.font != nullptr) << fatal;
        auto& bytes = f.bytes;

        expect(f.rejected(bytes, 2000.0f));
    };

    "font: draws four vertices per glyph"_test = [] {
        FontFixture f;
        expect(!f.bytes.empty()) << "could not read " << RENDER_TEST_ASSETS_DIR "/DejaVuSans.ttf" << fatal;
        expect(f.font != nullptr) << fatal;

        const gfx::TriangleMesh mesh = f.meshOf("ab");

        expect(mesh.vertices.size() == 8u);
        const std::vector<uint16_t> expected{ 0, 1, 2, 0, 2, 3, 4, 5, 6, 4, 6, 7 };
        expect(mesh.indices == expected);
    };

    "font: spaces advance without geometry"_test = [] {
        FontFixture f;
        expect(!f.bytes.empty()) << "could not read " << RENDER_TEST_ASSETS_DIR "/DejaVuSans.ttf" << fatal;
        expect(f.font != nullptr) << fatal;
        auto& font = f.font;

        expect((f.meshOf("a b").vertices.size()) == 8u);
        expect((font->measure("a b").x) > (font->measure("ab").x));
    };

    "font: covers french"_test = [] {
        FontFixture f;
        expect(!f.bytes.empty()) << "could not read " << RENDER_TEST_ASSETS_DIR "/DejaVuSans.ttf" << fatal;
        expect(f.font != nullptr) << fatal;

        f.expectDifferentUvsFromQuestionMark(EAcute);
        f.expectDifferentUvsFromQuestionMark(OeLigature);
        f.expectDifferentUvsFromQuestionMark(Euro);
    };

    "font: missing glyph draws question mark"_test = [] {
        FontFixture f;
        expect(!f.bytes.empty()) << "could not read " << RENDER_TEST_ASSETS_DIR "/DejaVuSans.ttf" << fatal;
        expect(f.font != nullptr) << fatal;

        const gfx::TriangleMesh emoji = f.meshOf(Emoji);

        expect(emoji.vertices.size() == 4u) << fatal;
        expectSameUvs(emoji, f.meshOf("?"));
    };

    "font: invalid utf8 is skipped"_test = [] {
        FontFixture f;
        expect(!f.bytes.empty()) << "could not read " << RENDER_TEST_ASSETS_DIR "/DejaVuSans.ttf" << fatal;
        expect(f.font != nullptr) << fatal;

        expect((f.meshOf("a\xFFz").vertices.size()) == 8u);
        expect((f.meshOf("a\xC3").vertices.size()) == 4u);
    };

    "font: newline starts aline"_test = [] {
        FontFixture f;
        expect(!f.bytes.empty()) << "could not read " << RENDER_TEST_ASSETS_DIR "/DejaVuSans.ttf" << fatal;
        expect(f.font != nullptr) << fatal;
        auto& font = f.font;

        const gfx::TriangleMesh mesh = f.meshOf("a\na");

        expect(mesh.vertices.size() == 8u) << fatal;
        expect(test::near(mesh.vertices[4].y - mesh.vertices[0].y, font->lineHeight(), 1.0f));
        expect(test::near(mesh.vertices[4].x, mesh.vertices[0].x, 1.0f));
        expect(test::floatEq(font->measure("a\na").y, 2.0f * font->lineHeight()));
    };

    "font: measure of empty is zero"_test = [] {
        FontFixture f;
        expect(!f.bytes.empty()) << "could not read " << RENDER_TEST_ASSETS_DIR "/DejaVuSans.ttf" << fatal;
        expect(f.font != nullptr) << fatal;
        auto& font = f.font;

        const gfx::Vec2 size = font->measure("");

        expect(size.x == 0.0f);
        expect(size.y == 0.0f);
    };

    "font: top left anchors the first line"_test = [] {
        FontFixture f;
        expect(!f.bytes.empty()) << "could not read " << RENDER_TEST_ASSETS_DIR "/DejaVuSans.ttf" << fatal;
        expect(f.font != nullptr) << fatal;
        auto& font = f.font;

        gfx::TextDraw draw;
        draw.font = font.get();
        draw.text = "A";
        draw.position = gfx::Vec2{ 10.0f, 20.0f };

        gfx::TriangleMesh mesh;
        font->appendText(mesh, draw);

        expect(mesh.vertices.size() == 4u) << fatal;
        for (const gfx::Vertex& vertex : mesh.vertices) {
            expect(vertex.x >= 10.0f);
            expect(vertex.y >= 20.0f);
        }
    };

    "font: screen text lands on whole pixels"_test = [] {
        FontFixture f;
        expect(!f.bytes.empty()) << "could not read " << RENDER_TEST_ASSETS_DIR "/DejaVuSans.ttf" << fatal;
        expect(f.font != nullptr) << fatal;
        auto& font = f.font;

        gfx::TextDraw draw;
        draw.font = font.get();
        draw.text = "Whole pixels";
        draw.position = gfx::Vec2{ 10.0f, 20.0f };
        draw.space = gfx::Space::Screen;

        gfx::TriangleMesh mesh;
        font->appendText(mesh, draw);

        expect(!mesh.vertices.empty()) << fatal;
        for (const gfx::Vertex& vertex : mesh.vertices) {
            expect(test::near(vertex.x, std::round(vertex.x), 1e-4));
            expect(test::near(vertex.y, std::round(vertex.y), 1e-4));
        }
    };

    "font: colour reaches every vertex"_test = [] {
        FontFixture f;
        expect(!f.bytes.empty()) << "could not read " << RENDER_TEST_ASSETS_DIR "/DejaVuSans.ttf" << fatal;
        expect(f.font != nullptr) << fatal;
        auto& font = f.font;

        gfx::TextDraw draw;
        draw.font = font.get();
        draw.text = "ab";
        draw.color = gfx::Color{ 10, 20, 30, 40 };

        gfx::TriangleMesh mesh;
        font->appendText(mesh, draw);

        expect(mesh.vertices.size() == 8u) << fatal;
        for (const gfx::Vertex& vertex : mesh.vertices) {
            expect(vertex.r == 10);
            expect(vertex.g == 20);
            expect(vertex.b == 30);
            expect(vertex.a == 40);
        }
    };

    "font: long text stops at the vertex limit"_test = [] {
        FontFixture f;
        expect(!f.bytes.empty()) << "could not read " << RENDER_TEST_ASSETS_DIR "/DejaVuSans.ttf" << fatal;
        expect(f.font != nullptr) << fatal;

        const std::string text(20000, 'a');
        const gfx::TriangleMesh mesh = f.meshOf(text);

        expect(mesh.vertices.size() == gfx::MaxMeshVertices);
        for (const uint16_t index : mesh.indices) {
            expect((static_cast<size_t>(index)) < mesh.vertices.size()) << fatal;
        }
    };

    "font: draw text is recorded with its font"_test = [] {
        FontFixture f;
        expect(!f.bytes.empty()) << "could not read " << RENDER_TEST_ASSETS_DIR "/DejaVuSans.ttf" << fatal;
        expect(f.font != nullptr) << fatal;
        auto& renderer = f.renderer;
        auto& font = f.font;

        renderer.beginFrame(gfx::Color{});

        gfx::TextDraw draw;
        draw.font = font.get();
        draw.text = "ab";
        draw.layer = gfx::Layer::Foreground;
        draw.space = gfx::Space::Screen;
        renderer.drawText(draw);

        expect(renderer.recorded().size() == 1u) << fatal;
        const gfx::NullRenderer::Recorded& entry = renderer.recorded()[0];
        expect(entry.kind == gfx::NullRenderer::Recorded::Kind::Text);
        expect(entry.text.font == font.get());
        expect(entry.textContent == ("ab"));
        expect(entry.text.layer == gfx::Layer::Foreground);
        expect(entry.text.space == gfx::Space::Screen);
    };

    "font: empty text appends nothing"_test = [] {
        FontFixture f;
        expect(!f.bytes.empty()) << "could not read " << RENDER_TEST_ASSETS_DIR "/DejaVuSans.ttf" << fatal;
        expect(f.font != nullptr) << fatal;
        auto& font = f.font;

        gfx::TriangleMesh mesh;
        gfx::TextDraw draw;
        draw.font = font.get();
        draw.text = "";
        font->appendText(mesh, draw);

        expect(mesh.vertices.size() == 0u);
    };
}
