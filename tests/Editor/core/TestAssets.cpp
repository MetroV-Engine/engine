// The assets folder (AssetLibrary): what the Asset Browser shows.

#include <boost/ut.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "Support/FreshWorkingDirectory.hpp"
#include "core/AssetLibrary/AssetLibrary.hpp"

using namespace boost::ut;

namespace {
    void writeFile(const std::filesystem::path &path, const std::string &content)
    {
        if (path.has_parent_path())
            std::filesystem::create_directories(path.parent_path());
        std::ofstream(path, std::ios::binary) << content;
    }

    std::vector<std::string> paths(const AssetLibrary &library)
    {
        std::vector<std::string> result;
        for (const AssetLibrary::Asset &asset : library.assets())
            result.push_back(asset.path);
        return result;
    }

    const AssetLibrary::Asset &assetAt(const AssetLibrary &library, const std::string &path)
    {
        return *std::ranges::find(library.assets(), path, &AssetLibrary::Asset::path);
    }
}

void run_asset_tests()
{
    "the assets folder is listed with its subfolders, sorted and typed"_test = [] {
        FreshWorkingDirectory directory;
        writeFile("assets/sprites/hero.png", "12345");
        writeFile("assets/sounds/Jump.WAV", "");
        writeFile("assets/fonts/ui.ttf", "");
        writeFile("assets/notes.txt", "");
        writeFile("assets/first.level", "");
        writeFile("assets/data.bin", "");
        writeFile("assets/.hidden", "");
        writeFile("assets/.cache/skipped.png", "");
        writeFile("outside.png", "");

        AssetLibrary library;
        library.scan();

        expect(paths(library)
            == std::vector<std::string>{"data.bin", "first.level", "fonts/ui.ttf", "notes.txt", "sounds/Jump.WAV", "sprites/hero.png"})
            << "hidden files and folders skipped, '/' separators";
        expect(assetAt(library, "sprites/hero.png").type == AssetLibrary::Type::Image);
        expect(assetAt(library, "sprites/hero.png").size == 5_ul);
        expect(assetAt(library, "sounds/Jump.WAV").type == AssetLibrary::Type::Audio) << "extensions in any case";
        expect(assetAt(library, "fonts/ui.ttf").type == AssetLibrary::Type::Font);
        expect(assetAt(library, "notes.txt").type == AssetLibrary::Type::Text);
        expect(assetAt(library, "first.level").type == AssetLibrary::Type::Level);
        expect(assetAt(library, "data.bin").type == AssetLibrary::Type::Other);
        expect(!library.truncated());
    };

    "a missing assets folder gives no asset"_test = [] {
        FreshWorkingDirectory directory;
        AssetLibrary library;
        library.scan();
        expect(library.assets().empty());
    };

    "a new scan sees the files added since the last one"_test = [] {
        FreshWorkingDirectory directory;
        writeFile("assets/a.png", "");
        AssetLibrary library;
        library.scan();
        writeFile("assets/b.png", "");
        expect(paths(library) == std::vector<std::string>{"a.png"});
        library.scan();
        expect(paths(library) == std::vector<std::string>{"a.png", "b.png"});
    };

    "each scan starts a new generation of asset indices"_test = [] {
        FreshWorkingDirectory directory;
        AssetLibrary library;
        const std::size_t before = library.generation();
        library.scan();
        expect(library.generation() == before + 1);
        library.scan();
        expect(library.generation() == before + 2);
    };
}
