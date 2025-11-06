#include <iostream>
#include <string>
#include <cstdint>
#include <exception>
#include <string>
#include <filesystem>
#include <functional>
#include <fstream>

#include <landstalker_tools.h>
#define TCLAP_SETBASE_ZERO 1
#include <tclap/CmdLine.h>

#include <landstalker/main/GameData.h>
#include <landstalker/main/Rom.h>
#include <landstalker/main/ImageBuffer.h>
#include <landstalker/behaviours/BehaviourYamlConverter.h>

std::string format_filename(const std::string& name)
{
    std::string out;
    for(char c : name)
    {
        if(std::isalnum(c) || c == '-' || c == '_' || c == '.')
        {
            out += c;
        }
        else if(c == '\'' || c == '"')
        {
            // skip
        }
        else
        {
            out += '_';
        }
    }
    return out;
}

std::string format_path(const std::string& name)
{
    std::string out;
    for(char c : name)
    {
        if(std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '/')
        {
            out += c;
        }
        else if(c == '\'' || c == '"')
        {
            // skip
        }
        else
        {
            out += '_';
        }
    }
    return out;
}

void extract_maps(const std::shared_ptr<Landstalker::GameData>& gd, const std::filesystem::path& outdir)
{
    auto rd = gd->GetRoomData();
    if(!rd)
    {
        throw std::runtime_error("No room data available to extract maps.");
    }
    auto maps_dir = outdir / "graphics" / "room_maps";
    std::filesystem::create_directories(maps_dir);
    for(const auto& [name, map] : rd->GetMaps())
    {
        std::cout << "Extracting map: " << name << std::endl;
        std::string fg, bg, hm;
        if(!map->GetData()->ToCsv(fg, bg, hm))
        {
            std::cerr << "  Failed to convert map to CSV format." << std::endl;
            continue;
        }
        std::ofstream ffg(maps_dir / format_filename(name + "_fg.csv"));
        std::ofstream fbg(maps_dir / format_filename(name + "_bg.csv"));
        std::ofstream fhm(maps_dir / format_filename(name + "_hm.csv"));
        ffg << fg;
        fbg << bg;
        fhm << hm;
    }
}

void extract_tiles(std::shared_ptr<Landstalker::TilesetEntry> tileset, const std::shared_ptr<Landstalker::GameData>& gd, const std::filesystem::path& outdir)
{
    auto buf = Landstalker::ImageBuffer();
    std::filesystem::create_directories(outdir);
    std::cout << "Extracting tileset: " << tileset->GetName() << std::endl;
    std::size_t width = std::min<std::size_t>(32, tileset->GetData()->GetTileCount());
    std::size_t height = (tileset->GetData()->GetTileCount() + width - 1) / width;
    auto palette = gd->GetPalette(tileset->GetDefaultPalette());
    if (!palette)
    {
        palette = std::make_shared<Landstalker::PaletteEntry>(gd.get(), Landstalker::ByteVector(), "default_debug_palette", "", Landstalker::Palette::Type::FULL);
    }
    auto indices = Landstalker::SplitStr<uint8_t>(tileset->GetPaletteIndicies());
    if (!indices.empty())
    {
        indices[0] = 0; // Ensure transparency index is mapped to itself
        tileset->GetData()->SetColourIndicies(indices);
    }
    buf.Resize(width * tileset->GetData()->GetTileWidth(), height * tileset->GetData()->GetTileHeight());
    buf.Clear(0);
    int tile_index = 0;
    for(std::size_t y = 0; y < height; ++y)
    {
        for(std::size_t x = 0; x < width; ++x)
        {
            if(tile_index >= tileset->GetData()->GetTileCount())
            {
                break;
            }
            buf.InsertTile(x * tileset->GetData()->GetTileWidth(),
                            y * tileset->GetData()->GetTileHeight(),
                            0, 
                            {static_cast<uint16_t>(tile_index++)},
                            *tileset->GetData(),
                            true);
        }
    }
    buf.WritePNG((outdir / format_filename(tileset->GetName() + ".png")).string(), {palette->GetData()});
}

void extract_tiles(std::shared_ptr<Landstalker::AnimatedTilesetEntry> tileset, const std::shared_ptr<Landstalker::GameData>& gd, const std::filesystem::path& outdir)
{
    auto buf = Landstalker::ImageBuffer();
    std::filesystem::create_directories(outdir);
    std::cout << "Extracting animated tileset: " << tileset->GetName() << std::endl;
    std::ofstream meta_out(outdir / format_filename(tileset->GetName() + ".yaml"));
    meta_out << tileset->GetData()->ExtractMetadataYaml();
    std::size_t width = std::min<std::size_t>(tileset->GetData()->GetFrameSizeTiles(), tileset->GetData()->GetTileCount());
    std::size_t height = (tileset->GetData()->GetTileCount() + width - 1) / width;
    auto palette = gd->GetPalette(tileset->GetDefaultPalette());
    buf.Resize(width * tileset->GetData()->GetTileWidth(), height * tileset->GetData()->GetTileHeight());
    buf.Clear(0);
    int tile_index = 0;
    for(std::size_t y = 0; y < height; ++y)
    {
        for(std::size_t x = 0; x < width; ++x)
        {
            if(tile_index >= tileset->GetData()->GetTileCount())
            {
                break;
            }
            buf.InsertTile(x * tileset->GetData()->GetTileWidth(),
                            y * tileset->GetData()->GetTileHeight(),
                            0, 
                            {static_cast<uint16_t>(tile_index++)},
                            *tileset->GetData(),
                            true);
        }
    }
    buf.WritePNG((outdir / format_filename(tileset->GetName() + ".png")).string(), {palette->GetData()});
}

template <template <typename, typename, typename, typename> class C, typename Key, typename Comp, typename Alloc>
void extract_tiles(const C<Key, std::shared_ptr<Landstalker::TilesetEntry>, Comp, Alloc>& tiles, const std::shared_ptr<Landstalker::GameData>& gd, const std::filesystem::path& outdir)
{
    std::filesystem::create_directories(outdir);
    for(const auto& [name, tileset] : tiles)
    {
        extract_tiles(tileset, gd, outdir);
    }
}

template <template <typename, typename> class C,typename Alloc>
void extract_tiles(const C<std::shared_ptr<Landstalker::TilesetEntry>, Alloc>& tiles, const std::shared_ptr<Landstalker::GameData>& gd, const std::filesystem::path& outdir)
{
    std::filesystem::create_directories(outdir);
    for(const auto& tileset : tiles)
    {
        extract_tiles(tileset, gd, outdir);
    }
}

void extract_animated_tilesets(const std::shared_ptr<Landstalker::GameData>& gd, const std::filesystem::path& outdir)
{
    auto rd = gd->GetRoomData();
    if(!rd)
    {
        throw std::runtime_error("No room data available to extract animated tilesets.");
    }
    auto tilesets_dir = outdir / "graphics" / "tilesets";
    std::filesystem::create_directories(tilesets_dir);
    for(const auto& tileset : rd->GetTilesets())
    {
        auto tileset_dir = tilesets_dir / format_filename(tileset->GetName());
        for(const auto& anim_tileset : rd->GetAnimatedTilesets(tileset->GetName()))
        {
            extract_tiles(anim_tileset, gd, tileset_dir);
        }
    }
}

void extract_graphics(const std::shared_ptr<Landstalker::GameData>& gd, const std::filesystem::path& outdir)
{
    auto graphics_dir = outdir / "graphics";
    auto misc_tiles_dir = graphics_dir / "misc_tiles";
    std::filesystem::create_directories(graphics_dir);
    std::filesystem::create_directories(misc_tiles_dir);
    for(const auto& tileset : gd->GetRoomData()->GetTilesets())
    {
        extract_tiles(tileset, gd, graphics_dir / "tilesets" / format_filename(tileset->GetName()));
    }
    extract_tiles(gd->GetRoomData()->GetIntroFont(), gd, misc_tiles_dir / "fonts");
    extract_tiles(gd->GetStringData()->GetFonts(), gd, misc_tiles_dir / "fonts");
    extract_tiles(gd->GetGraphicsData()->GetFonts(), gd, misc_tiles_dir / "fonts");
    extract_tiles(gd->GetGraphicsData()->GetUIGraphics(), gd, misc_tiles_dir / "ui");
    extract_tiles(gd->GetGraphicsData()->GetStatusEffects(), gd, misc_tiles_dir / "status_effects");
    extract_tiles(gd->GetGraphicsData()->GetSwordEffects(), gd, misc_tiles_dir / "sword_effects");
    extract_tiles(gd->GetGraphicsData()->GetEndCreditLogosTiles(), gd, misc_tiles_dir / "end_credit_logos");
    extract_tiles(gd->GetGraphicsData()->GetLithographTiles(), gd, misc_tiles_dir / "lithograph");
    extract_tiles(gd->GetGraphicsData()->GetTitleScreenTiles(), gd, misc_tiles_dir / "title_screen");
    extract_tiles(gd->GetGraphicsData()->GetSegaLogoTiles(), gd, misc_tiles_dir / "title_screen");
    extract_tiles(gd->GetGraphicsData()->GetGameLoadScreenTiles(), gd, misc_tiles_dir / "load_game");
}

void extract_palettes(std::shared_ptr<Landstalker::PaletteEntry> palette, const std::filesystem::path& outdir)
{
    std::filesystem::create_directories(outdir);
    std::cout << "Extracting palette: " << palette->GetName() << std::endl;
    std::ofstream f(outdir / format_filename(palette->GetName() + ".gpl"));
    f << palette->GetData()->ToGpl();
}

void extract_palettes(const std::map<std::string, std::shared_ptr<Landstalker::PaletteEntry>>& palettes, const std::filesystem::path& outdir)
{
    for(const auto& [name, palette] : palettes)
    {
        extract_palettes(palette, outdir);
    }
}

void extract_palettes(const std::vector<std::shared_ptr<Landstalker::PaletteEntry>>& palettes, const std::filesystem::path& outdir)
{
    for(const auto& palette : palettes)
    {
        extract_palettes(palette, outdir);
    }
}

void extract_palettes(const std::shared_ptr<Landstalker::GameData>& gd, const std::filesystem::path& outdir)
{
    auto grd = gd->GetGraphicsData();
    if(!grd)
    {
        throw std::runtime_error("No graphics data available to extract palettes.");
    }
    auto palettes_dir = outdir / "graphics" / "palettes";
    std::filesystem::create_directories(palettes_dir);
    extract_palettes(gd->GetGraphicsData()->GetPlayerPalette(), palettes_dir / "misc");
    extract_palettes(gd->GetGraphicsData()->GetHudPalette(), palettes_dir / "misc");
    extract_palettes(gd->GetGraphicsData()->GetSwordPalettes(), palettes_dir / "equipment");
    extract_palettes(gd->GetGraphicsData()->GetArmourPalettes(), palettes_dir / "equipment");
    extract_palettes(gd->GetGraphicsData()->GetOtherPalettes(), palettes_dir / "misc");
    extract_palettes(gd->GetRoomData()->GetRoomPalettes(), palettes_dir / "room");
    extract_palettes(gd->GetRoomData()->GetMiscPalette(Landstalker::RoomData::MiscPaletteType::LANTERN), palettes_dir / "room_misc");
    extract_palettes(gd->GetRoomData()->GetMiscPalette(Landstalker::RoomData::MiscPaletteType::LAVA), palettes_dir / "room_misc");
    extract_palettes(gd->GetRoomData()->GetMiscPalette(Landstalker::RoomData::MiscPaletteType::WARP), palettes_dir / "room_misc");
    extract_palettes(gd->GetSpriteData()->GetAllPalettes(), palettes_dir / "sprites");
}

void extract_2d_map(std::shared_ptr<Landstalker::Tilemap2DEntry> map, const std::shared_ptr<Landstalker::GameData>& gd, const std::filesystem::path& outdir)
{
    std::cout << "Extracting 2D map: " << map->GetName() << std::endl;
    try
    {
        std::string csv = map->GetData()->ToCsv();
        std::ofstream ofs(outdir / format_filename(map->GetName() + ".csv"));
        ofs << csv;

        auto tileset = gd->GetTileset(map->GetTileset());
        if(!tileset)
        {
            std::cerr << "  No tileset found for map preview." << std::endl;
            return;
        }
        auto palette = gd->GetPalette(tileset->GetDefaultPalette());
        if(!palette)
        {
            std::cerr << "  No palette found for map preview." << std::endl;
            return;
        }
        Landstalker::ImageBuffer buf(map->GetData()->GetWidth() * tileset->GetData()->GetTileWidth(), map->GetData()->GetHeight() * tileset->GetData()->GetTileHeight());
        for(std::size_t y = 0; y < map->GetData()->GetHeight(); ++y)
        {
            for(std::size_t x = 0; x < map->GetData()->GetWidth(); ++x)
            {
                buf.InsertTile(x * tileset->GetData()->GetTileWidth(),
                                y * tileset->GetData()->GetTileHeight(),
                                0,
                                map->GetData()->GetTile(x, y),
                                *tileset->GetData(),
                                true);
            }
        }
        buf.WritePNG((outdir / format_filename(map->GetName() + "_preview.png")).string(), {palette->GetData()});
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
}

void extract_2d_maps(const std::shared_ptr<Landstalker::GameData>& gd, const std::filesystem::path& outdir)
{
    auto grd = gd->GetGraphicsData();
    if(!grd)
    {
        throw std::runtime_error("No graphics data available to extract maps.");
    }
    auto maps_dir = outdir / "graphics" / "2d_maps";
    std::filesystem::create_directories(maps_dir);
    for(const auto& [name, map] : grd->GetAllMaps())
    {
        extract_2d_map(map, gd, maps_dir);
    }
    for(const auto& map : gd->GetStringData()->GetAllMaps())
    {
        extract_2d_map(map.second, gd, maps_dir);
    }
}

void extract_blocksets(const std::shared_ptr<Landstalker::GameData>& gd, const std::filesystem::path& outdir)
{
    auto rd = gd->GetRoomData();
    if(!rd)
    {
        throw std::runtime_error("No room data available to extract blocksets.");
    }
    auto blocksets_dir = outdir / "graphics" / "blocksets";
    std::filesystem::create_directories(blocksets_dir);
    for(const auto& [name, blockset] : rd->GetAllBlocksets())
    {
        auto tileset = rd->GetTilesets().at(blockset->GetTileset());
        auto blockset_dir = blocksets_dir / format_filename(tileset ? tileset->GetName() : "unknown");
        std::filesystem::create_directories(blockset_dir);
        std::cout << "Extracting blockset: " << name << std::endl;
        std::string csv;
        if(!blockset->GetData())
        {
            std::cerr << "  Failed to convert blockset to CSV format." << std::endl;
            continue;
        }
        std::ofstream f(blockset_dir / format_filename(name + ".csv"));
        f << Landstalker::BlocksetCmp::ToCsv(*blockset->GetData());
        if(blockset->GetData()->empty())
        {
            continue;
        }
        Landstalker::ImageBuffer buf;
        unsigned int width = std::min<unsigned int>(32, blockset->GetData()->size());
        unsigned int height = (blockset->GetData()->size() + width - 1) / width;
        auto palette = gd->GetPalette(tileset->GetDefaultPalette());
        unsigned int blockwidth = Landstalker::MapBlock::GetBlockWidth() * tileset->GetData()->GetTileWidth();
        unsigned int blockheight = Landstalker::MapBlock::GetBlockHeight() * tileset->GetData()->GetTileHeight();
        buf.Resize(width * blockwidth, height * blockheight);
        for(unsigned int by = 0; by < height; ++by)
        {
            for(unsigned int bx = 0; bx < width; ++bx)
            {
                unsigned int block_index = by * width + bx;
                if(block_index >= blockset->GetData()->size())
                {
                    break;
                }
                const auto& block = (*blockset->GetData())[block_index];
                buf.InsertBlock(bx * blockwidth,
                                by * blockheight,
                                0, block, *tileset->GetData());
            }
        }
        buf.WritePNG((blockset_dir / format_filename(name + "_preview.png")).string(), {palette->GetData()});
    }
}

void extract_sprites(const std::shared_ptr<Landstalker::GameData>& gd, const std::filesystem::path& outdir)
{
    auto sd = gd->GetSpriteData();
    if(!sd)
    {
        throw std::runtime_error("No sprite data available to extract sprites.");
    }
    auto sprites_dir = outdir / "graphics" / "sprites";
    auto entities_dir = outdir / "data" / "entities";
    std::filesystem::create_directories(sprites_dir);
    std::filesystem::create_directories(entities_dir);
    for(int i = 0; i < 256; ++i)
    {
        if(!sd->IsSprite(i))
        {
            continue;
        }
        auto name = sd->GetSpriteName(i);
        std::cout << "Extracting sprite: " << name << std::endl;
        {
            std::ofstream meta_out(sprites_dir / format_filename(name + "_metadata.yaml"));
            meta_out << sd->GetSpriteMetadataYaml(i);
        }
        // Extract frames for each entity
        auto metadata = sd->GetSpriteMetadata(i);
        std::cout << "  " << metadata.frame_count << " frames found." << std::endl;
        int width = std::min(8U, metadata.frame_count);
        int height = (metadata.frame_count + width - 1) / width;
        Landstalker::ImageBuffer buf(width * metadata.frame_width, height * metadata.frame_height);
        for(int j = 0; j < metadata.frame_count; ++j)
        {
            const auto& frame = sd->GetSpriteFrame(i, j);
            buf.InsertSprite((j % width) * metadata.frame_width + metadata.origin.x,
                             (j / width) * metadata.frame_height + metadata.origin.y,
                             0,
                             *frame->GetData(),
                             false);
        }
        std::set<std::pair<int, int>> palettes;
        for(const auto& entity : sd->GetEntitiesFromSprite(i))
        {
            const std::string& entity_name = Landstalker::wstr_to_utf8(sd->GetEntityDisplayName(entity));
            auto outfile = entities_dir / format_filename(Landstalker::StrPrintf("%03d_%s.yaml", entity, entity_name.c_str()));
            palettes.insert(sd->GetEntityPaletteIdxs(entity));
            // Extract entity metadata
            std::ofstream entity_meta_out(outfile);
            entity_meta_out << sd->GetEntityMetadataYaml(entity, gd->GetStringData());
        }
        for(const auto& idxs : palettes)
        {
            auto palette = sd->GetSpritePalette(idxs.first, idxs.second);
            if(!palette)
            {
                std::cerr << "  No palette found for sprite preview: " << name << std::endl;
                continue;
            }
            buf.WritePNG((sprites_dir / format_filename(Landstalker::StrPrintf("%s_l%02d_h%02d_frames.png", name.c_str(), idxs.first, idxs.second))).string(), {palette});
        }
    }
}

void extract_strings(const std::shared_ptr<Landstalker::GameData>& gd, const std::filesystem::path& outdir)
{
    auto strd = gd->GetStringData();
    if(!strd)
    {
        throw std::runtime_error("No string data available to extract strings.");
    }
    auto strings_dir = outdir / "data" / "strings";
    auto script_dir = outdir / "data" / "script";
    std::filesystem::create_directories(strings_dir);
    std::filesystem::create_directories(script_dir);
    auto extract_strings = [&strings_dir, &strd](const std::string& filename, std::size_t count, auto get_string_func)
    {
        if(count == 0)
        {
            return;
        }
        std::ofstream f(strings_dir / filename);
        std::cout << "Extracting strings to: " << filename << std::endl;
        for(std::size_t i = 0; i < count; ++i)
        {
            f << Landstalker::wstr_to_utf8(get_string_func(i));
            if(i + 1 < count)
            {
                f << '\n';
            }
        }
    };
    extract_strings("main_strings.txt", strd->GetMainStringCount(), std::bind(&Landstalker::StringData::GetMainString, strd.get(), std::placeholders::_1));
    extract_strings("item_names.txt", strd->GetItemNameCount(), std::bind(&Landstalker::StringData::GetItemName, strd.get(), std::placeholders::_1));
    extract_strings("char_names.txt", strd->GetCharNameCount(), std::bind(&Landstalker::StringData::GetCharName, strd.get(), std::placeholders::_1));
    extract_strings("special_char_names.txt", strd->GetSpecialCharNameCount(), std::bind(&Landstalker::StringData::GetSpecialCharName, strd.get(), std::placeholders::_1));
    extract_strings("default_char_name.txt", 1, [&strd](std::size_t) { return strd->GetDefaultCharName(); });
    extract_strings("menu_strings.txt", strd->GetMenuStrCount(), std::bind(&Landstalker::StringData::GetMenuStr, strd.get(), std::placeholders::_1));
    extract_strings("system_strings.txt", strd->GetSystemStringCount(), std::bind(&Landstalker::StringData::GetSystemString, strd.get(), std::placeholders::_1));
    extract_strings("end_credit_strings.tsv", strd->GetEndCreditStringCount(), [&strd](std::size_t i) { return strd->GetEndCreditString(i).Serialise(); });
    extract_strings("intro_strings.tsv", strd->GetIntroStringCount(), [&strd](std::size_t i) { return strd->GetIntroString(i).Serialise(); });
    std::cout << "Exporting Special Character SFX Mapping" << std::endl;
    std::ofstream ofs(script_dir / "special_char_sfx.yaml");
    ofs << strd->ExportSpecialCharTalkSfxYaml();
}

void extract_script(const std::shared_ptr<Landstalker::GameData>& gd, const std::filesystem::path& outdir)
{
    auto sd = gd->GetScriptData();
    if(!sd)
    {
        throw std::runtime_error("No script data available to extract script.");
    }
    auto script_dir = outdir / "data" / "script";
    std::filesystem::create_directories(script_dir);
    std::cout << "Extracting script to YAML." << std::endl;
    std::ofstream ofs(script_dir / "game_script.yaml");
    ofs << Landstalker::wstr_to_utf8(sd->GetScript()->ToYaml(gd));
}

void extract_script_funcs(const std::shared_ptr<Landstalker::GameData>& gd, const std::filesystem::path& outdir)
{
    auto sd = gd->GetScriptData();
    if(!sd)
    {
        throw std::runtime_error("No script data available to extract script functions.");
    }
    auto script_funcs_dir = outdir / "data" / "script_functions";
    std::filesystem::create_directories(script_funcs_dir);
    auto save_yaml = [&](const std::string& filename, const std::string& name, std::shared_ptr<Landstalker::ScriptFunctionTable> scriptfunc)
    {
        std::cout << "Extracting " << name << " functions to: " << filename << std::endl;
        std::ofstream ofs(script_funcs_dir / filename);
        ofs << scriptfunc->ToYaml(name);
    };
    save_yaml("character_functions.yaml", "CharacterScript", sd->GetCharFuncs());
    save_yaml("cutscene_functions.yaml", "CutsceneScript", sd->GetCutsceneFuncs());
    save_yaml("shop_functions.yaml", "ShopScript", sd->GetShopFuncs());
    save_yaml("item_functions.yaml", "ItemHandleScript", sd->GetItemFuncs());
    save_yaml("progres_flags.yaml", "ProgressFlags", sd->GetProgressFlagsFuncs());
}

void extract_behaviours(const std::shared_ptr<Landstalker::GameData>& gd, const std::filesystem::path& outdir)
{
    auto sd = gd->GetSpriteData();
    if(!sd)
    {
        throw std::runtime_error("No sprite data available to extract behaviours.");
    }
    auto behaviours_dir = outdir / "data" / "behaviours";
    std::filesystem::create_directories(behaviours_dir);
    for(const auto& [id, name] : sd->GetScriptNames())
    {
        std::cout << "Extracting behaviour: " << name << " (" << id << ")" << std::endl;
        std::ofstream ofs(behaviours_dir / format_filename(name + ".yaml"));
        ofs << Landstalker::BehaviourYamlConverter::ToYaml(id, name, sd->GetScript(id).second);
    }
}

void extract_room_preview(const std::shared_ptr<Landstalker::Room>& room, const std::shared_ptr<Landstalker::GameData>& gd, const std::filesystem::path& outdir)
{
    auto buf = Landstalker::ImageBuffer();
    auto rd = gd->GetRoomData();
    static std::set<std::string> extracted_rooms;
    if(!rd)
    {
        throw std::runtime_error("No room data available to extract room preview.");
    }
    auto map = rd->GetMap(room->map)->GetData();
    auto tileset = rd->GetTileset(room->tileset);
    auto palette = rd->GetRoomPalette(room->room_palette);
    auto blockset = rd->GetCombinedBlocksetForRoom(room->name);
    const auto ISO_BLOCK_WIDTH = tileset->GetData()->GetTileWidth() * Landstalker::MapBlock::GetBlockWidth();
    const auto ISO_BLOCK_HEIGHT = tileset->GetData()->GetTileHeight() * Landstalker::MapBlock::GetBlockHeight() / 2;
    std::string fileprefix = format_filename(Landstalker::StrPrintf("%s_%s_%s_b%d%d", room->map.c_str(), palette->GetName().c_str(),
                                                                    tileset->GetName().c_str(), room->pri_blockset, room->sec_blockset));

    if(extracted_rooms.find(fileprefix) != extracted_rooms.end())
    {
        // Already extracted
        return;
    }
    extracted_rooms.insert(fileprefix);
    buf.Resize(map->GetPixelWidth(), map->GetPixelHeight());
    buf.Insert3DMapLayer(0, 0, 0, Landstalker::Tilemap3D::Layer::BG, map, tileset->GetData(), blockset, false);
    buf.Insert3DMapLayer(0, 0, 0, Landstalker::Tilemap3D::Layer::FG, map, tileset->GetData(), blockset, false, std::nullopt, std::nullopt, Landstalker::ImageBuffer::BlockMode::NO_PRIORITY_ONLY);
    buf.WritePNG((outdir / format_filename(fileprefix + "_preview_layer1.png")).string(), {palette->GetData()});
    buf.Insert3DMapLayer(0, 0, 0, Landstalker::Tilemap3D::Layer::FG, map, tileset->GetData(), blockset, false, std::nullopt, std::nullopt, Landstalker::ImageBuffer::BlockMode::PRIORITY_ONLY);
    buf.WritePNG((outdir / format_filename(fileprefix + "_preview_full.png")).string(), {palette->GetData()});
    buf.Clear(0);
    buf.Insert3DMapLayer(0, 0, 0, Landstalker::Tilemap3D::Layer::FG, map, tileset->GetData(), blockset, false, std::nullopt, std::nullopt, Landstalker::ImageBuffer::BlockMode::PRIORITY_ONLY);
    buf.WritePNG((outdir / format_filename(fileprefix + "_preview_layer2.png")).string(), {palette->GetData()});
}

void extract_rooms(const std::shared_ptr<Landstalker::GameData>& gd, const std::filesystem::path& outdir)
{
    auto rd = gd->GetRoomData();
    if(!rd)
    {
        throw std::runtime_error("No room data available to extract rooms.");
    }
    auto rooms_dir = outdir / "data" / "rooms";
    auto rooms_preview_dir = outdir / "graphics" / "room_maps";
    std::filesystem::create_directories(rooms_dir);
    std::filesystem::create_directories(rooms_preview_dir);
    for(std::size_t i = 0; i < rd->GetRoomCount(); ++i)
    {
        const auto& room = rd->GetRoom(i);
        const auto& name = Landstalker::wstr_to_utf8(room->GetDisplayName());
        std::cout << "Extracting room: " << name << std::endl;
        std::filesystem::path room_file = rooms_dir / format_path(name + ".yaml");
        std::filesystem::create_directories(room_file.parent_path());
        {
            std::ofstream ofs(room_file);
            ofs << room->ToYaml(gd);
        }
        extract_room_preview(room, gd, rooms_preview_dir);
    }
}

int main(int argc, char** argv)
{
	try
	{
		TCLAP::CmdLine cmd("Utility for extracting resources from the Landstalker ROM or ASM.\n"
		                   "Part of the landstalker_tools set: github.com/lordmir/landstalker_tools\n"
						   " - Written by LordMir, November 2025",
						   ' ', XSTR(VERSION_MAJOR) "." XSTR(VERSION_MINOR) "." XSTR(VERSION_PATCH));

		TCLAP::ValueArg<std::string> asmfile("a", "assembly", "Input ASM file to process", true, "", "assembly");
		TCLAP::ValueArg<std::string> romfile("r", "rom", "Input ROM file to process", true, "", "rom");
		TCLAP::ValueArg<std::string> labelsfile("l", "labels", "Labels YAML file", false, "", "rom");
		TCLAP::ValueArg<std::string> outpath("o", "output", "Output path for extracted files", false, "extracted", "output");
		cmd.xorAdd(asmfile, romfile);
        cmd.add(labelsfile);
        cmd.add(outpath);
		cmd.parse(argc, argv);
        std::shared_ptr<Landstalker::GameData> gd = nullptr;
        bool asmload = asmfile.isSet();
        if(labelsfile.isSet())
        {
            Landstalker::Labels::LoadData(labelsfile.getValue());
        }
        else
        {
            Landstalker::Labels::InitDefaults();
        }
        if(romfile.isSet())
        {
            Landstalker::Rom rom(romfile.getValue());
            std::cout << "Reading data from ROM: " << rom.get_description() << std::endl;
            gd = std::make_shared<Landstalker::GameData>(rom);
        }
        else if(asmfile.isSet())
        {
            std::cout << "Reading data from ASM: " << asmfile.getValue() << std::endl;
            gd = std::make_shared<Landstalker::GameData>(asmfile.getValue());
        }
        else
        {
            throw std::runtime_error("No input file specified.");
        }
        std::cout << "Loaded " << gd->GetContentDescription() << std::endl;
        std::filesystem::path outdir(outpath.getValue());
        if(!std::filesystem::exists(outdir))
        {
            std::filesystem::create_directories(outdir);
        }
        extract_maps(gd, outdir);
        extract_graphics(gd, outdir);
        extract_animated_tilesets(gd, outdir);
        extract_palettes(gd, outdir);
        extract_2d_maps(gd, outdir);
        extract_blocksets(gd, outdir);
        extract_sprites(gd, outdir);
        extract_strings(gd, outdir);
        extract_script(gd, outdir);
        if(asmload) extract_script_funcs(gd, outdir);
        extract_behaviours(gd, outdir);
        extract_rooms(gd, outdir);
	}            
	catch (const std::exception& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}
