//
// Created by hao on 9/11/20.
//

#include "asset_converter.hpp"
#include "load_save_png.hpp"
#include "read_write_chunk.hpp"
#include "data_path.hpp"
#include "ssize_t.hpp"
#include <iostream>
#include <glm/glm.hpp>
#include <algorithm>
#include <fstream>



/* Hard code the order of assets we read */
static const std::vector<std::string> asset_names = {
    // order is important! The coressponding png file is assets_names[i] + ".png"
    "char_stand",
    "char_crouch",
    "char_jump",
    "char_dead",
    "fire",
    "fire_2",
    "brick",
    "killer",
    "transparent",
    "spikedball",
    "score_0",
    "score_1",
    "score_2",
    "score_3",
    "score_4",
    "score_5",
    "score_6",
    "score_7",
    "score_8",
    "score_9"
};

// 8*8 tile in pixel
static const int TILE_WIDTH = 8;
static const int TILE_HEIGHT = 8;

// how many colors in a palette
static const int PALETTE_SIZE = 4;

// max number of palettes/tiles for PPU
static const int MAX_TOTAL_PALETTES = 8;
static const int MAX_TOTAL_TILES = 256;


static std::vector<PPU466::Tile> tiles;
static std::vector<PPU466::Palette> palettes;
static std::vector<AssetInfo> asset_infos;


/**
 * Helper function for debug
 */
std::string u8vec4_to_string(const glm::u8vec4& d) {
    return std::to_string((int)d[0]) + " " +
            std::to_string((int)d[1]) + " " +
            std::to_string((int)d[2]) + " " +
            std::to_string((int)d[3]);
}

/**
 * Helper function for debug
 */
void print_vec_u8vec4(const std::vector<glm::u8vec4> & data) {
    for(auto&d: data) {
        std::cout<<u8vec4_to_string(d)<<std::endl;
    }
}

/**
 * Helper function for debug
 */
void print_palette(const PPU466::Palette p) {
    for(int i=0; i<4; i++) {
        std::cout<<u8vec4_to_string(p[i])<<std::endl;
    }
}

/**
 * Helper function for debug
 */
void print_tile(const PPU466::Tile tile) {
    for(int i=TILE_HEIGHT - 1; i >= 0; i--) {// from last row to first row
        for(int j=0; j < TILE_WIDTH; j++) {
            // (j, i)
            int color_idx = ((tile.bit0[i] >> j) & 1) | (((tile.bit1[i] >> j) & 1) << 1);
            std::cout<<color_idx<<" ";
        }
        std::cout<<std::endl;
    }
}

/**
 * Debug purpose, reconstruct a png from its assetInfo struct and save multiple (or one) 8*8 png to
 * save directory
 */
void debug_reconstruct_png(const AssetInfo& info, const std::string& save_dir, const std::string& asset_name) {
    uint32_t cols = info.width / TILE_WIDTH;
    uint32_t rows = info.height / TILE_HEIGHT;

    assert(info.tile_indices.size() == cols * rows);

    //debug reconstruct all asset_name_X.png from the 3 vector
    int cons_idx = 0;
    for(uint32_t i=0; i< rows; i++) {
        for(uint32_t j = 0; j < cols; j++) {
            PPU466::Tile tile = tiles[info.tile_indices[j + i * cols]];
            PPU466::Palette palette = palettes[info.palette_index];
            std::vector<glm::u8vec4> data;

            for(int m=0; m < TILE_HEIGHT; m++) {
                for (int n=0; n < TILE_WIDTH; n++) {
                    // pixel at (n, m)
                    int color_idx = (((tile.bit1[m] >> n) & 1) << 1) | ((tile.bit0[m] >> n) & 1);
                    data.push_back(palette[color_idx]);
                }
            }
            save_png(save_dir + "/" + asset_name + "_" + std::to_string(cons_idx++) + ".png",
                     glm::uvec2(TILE_WIDTH, TILE_HEIGHT), &(data[0]), LowerLeftOrigin);
        }
    }
}

/**
 * Parse a single png's (not necessarily a 8*8 png data) data vector, and get its palette
 */
PPU466::Palette get_palette(const std::vector<glm::u8vec4>& data) {
    std::vector<glm::u8vec4> colors;
    // the first one is always 0,0,0,0
    colors.push_back(glm::u8vec4(0, 0, 0, 0));
    for(auto& color: data) {
        if(std::find(colors.begin(), colors.end(), color) == colors.end()) {
            colors.push_back(color);
            assert(colors.size() <= PALETTE_SIZE); // at most 4 different color color
        }
    }

    // note it is possible that the size of colors < 4, fill with (0,0,0,0)
    PPU466::Palette res;
    for(size_t i=0; i < PALETTE_SIZE; i++) {
        if(i < colors.size()) {
            res[i] = colors[i];
        } else {
            res[i] = glm::u8vec4(0, 0, 0, 0);
        }
    }
    return res;
}

/**
 * Construct a new tile based on the 8*8 png data and its corresponding palette
 *
 * @param data png data of size 8*8
 * @param palette this png's corresponding palette
 * @return tile rep for this png
 */
PPU466::Tile get_tile(const std::vector<glm::u8vec4>& data, const PPU466::Palette palette) {
    assert(data.size() == TILE_WIDTH * TILE_HEIGHT);
    PPU466::Tile tile{};

    for (int i = 0; i < TILE_HEIGHT; i ++) {
        for (int j = 0; j < TILE_WIDTH; j ++) {
            // pixel at (j, i)
            glm::u8vec4 color = data[i * TILE_WIDTH + j];
            auto idx = (size_t)(std::find(palette.begin(), palette.end(), color) - palette.begin());

            assert(idx < palette.size());
            tile.bit0[i] |= ((idx & 1) << j);
            tile.bit1[i] |= (((idx & 2) >> 1) << j);
        }
    }
    return tile;
}

/**
 * Split one large png data into multiple 8*8 small png data. The lower left 8*8 is the first one
 * @param png_data large png data
 * @return a vector of 8*8 png data
 */
std::vector<std::vector<glm::u8vec4>> split_png_data(const std::vector<glm::u8vec4>& png_data, uint32_t width, uint32_t height) {
    assert(width % TILE_WIDTH == 0);
    assert(height % TILE_HEIGHT == 0);

    uint32_t cols = width / TILE_WIDTH;
    uint32_t rows = height / TILE_HEIGHT;
    std::vector<std::vector<glm::u8vec4>> small_datas;

    for (uint32_t i=0; i < rows; i++) {
        for (uint32_t j=0; j < cols; j++) {
            // the small png at (i, j), read this png's data from low left
            std::vector<glm::u8vec4> small_data(TILE_WIDTH * TILE_HEIGHT);
            for(int m=0; m < TILE_HEIGHT; m++) {
                for(int n=0; n < TILE_WIDTH; n ++) {
                    small_data[m * TILE_WIDTH + n] = png_data[(i * TILE_HEIGHT + m) * cols * TILE_WIDTH + (j * TILE_WIDTH) + n];
                }
            }
            small_datas.push_back(small_data);
        }
    }
    return small_datas;
}

/**
 * Search a certain tile from the global tiles vector
 *
 * @return idx if found, -1 otherwise
 */
ssize_t search_tile(const PPU466::Tile& target_tile) {
    for(size_t i = 0; i < tiles.size(); i++) {
        if (target_tile.bit0 == tiles[i].bit0 && target_tile.bit1 == tiles[i].bit1) {
            return i;
        }
    }
    return -1;
}

/**
 * Search a certain palette from the global palettes vector
 *
 * @return idx if found, -1 otherwise
 */
ssize_t search_palette(const PPU466::Palette target_palette) {
    for(size_t i = 0; i < palettes.size(); i++) {
        // check if target_palette is a subset of palettes[i] (or if they are the same)
        bool is_subset = true;
        for (int j=0; j < PALETTE_SIZE; j++) {
            if(std::find(palettes[i].begin(), palettes[i].end(), target_palette[j]) == palettes[i].end()) {
                is_subset = false;
                break;
            }
        }
        if(is_subset) {
            return i;
        }
    }
    return -1;
}

void parse_pngs(const std::string& png_dir_name) {
    for(auto& asset_name: asset_names) {
#if defined(_WIN32)
        std::string png_path = png_dir_name + "\\" + asset_name + ".png";
#else
        std::string png_path = png_dir_name + "/" + asset_name + ".png";
#endif
        std::cout<<"Parsing: "<<png_path<<std::endl;
        glm::uvec2 size;
        std::vector<glm::u8vec4> png_data;
        load_png(png_path, &size, &png_data, LowerLeftOrigin); // use LowerLeftOrigin to be consistent with Tile

        // Construct palette per png (we only allow 4 bit color in a png even if it contains multiple 8*8 tile)
        PPU466::Palette new_palette = get_palette(png_data);
        ssize_t pal_idx = search_palette(new_palette);
        if(pal_idx < 0) {
            //find a new palettes
            palettes.push_back(new_palette);
            assert(palettes.size() <= MAX_TOTAL_PALETTES);
            pal_idx = palettes.size() - 1;
        } else {
            // use existing palette to draw
            new_palette = palettes[pal_idx];
        }

        AssetInfo info;
        info.width = size[0];
        info.height = size[1];
        info.palette_index = (uint8_t) pal_idx;

        std::vector<std::vector<glm::u8vec4>> small_png_datas = split_png_data(png_data, info.width, info.height);

        for (auto& small_data: small_png_datas) {
            // construct tile
            PPU466::Tile new_tile = get_tile(small_data, new_palette);
            ssize_t tile_idx = search_tile(new_tile);
            if(tile_idx < 0) {
                // find a new tile
                tiles.push_back(new_tile);
                assert(tiles.size() <= MAX_TOTAL_TILES);
                tile_idx = (int)(tiles.size() - 1);
            }
            info.tile_indices.push_back((uint8_t)tile_idx);
        }
        asset_infos.push_back(info);
//        debug_reconstruct_png(info, png_dir_name, asset_name);
    }
}

void write_asset_info_chunk(const std::vector<AssetInfo>& infos, std::ostream *to_) {
    assert(to_);
    auto &to = *to_;

    std::vector<uint8_t> tile_indices;
    std::vector<StoredAssetInfo> sinfos;

    for (auto const &info : infos) {
        sinfos.emplace_back();
        sinfos.back().tile_idx_begin = (uint32_t)tile_indices.size();
        tile_indices.insert(tile_indices.end(), info.tile_indices.begin(), info.tile_indices.end());
        sinfos.back().tile_idx_end = (uint32_t)tile_indices.size();
        sinfos.back().palette_index = info.palette_index;
        sinfos.back().width = info.width;
        sinfos.back().height = info.height;
    }

    write_chunk(Converter::TILE_IDX_MAGIC, tile_indices, &to);
    write_chunk(Converter::ASSET_INFO_MAGIC, sinfos, &to);
}

void read_asset_info_chunk(std::istream & from, std::vector<AssetInfo> * infos_p) {
    auto & infos = *infos_p;

    std::vector<uint8_t> tile_indices_sequence;
    std::vector<StoredAssetInfo> sinfos;
    read_chunk(from, Converter::TILE_IDX_MAGIC, &tile_indices_sequence);
    read_chunk(from, Converter::ASSET_INFO_MAGIC, &sinfos);

    // translate back to AssetInfo
    for(auto const & sinfo: sinfos) {
        std::vector<uint8_t> tile_indices(tile_indices_sequence.begin() + sinfo.tile_idx_begin,
                                 tile_indices_sequence.begin() + sinfo.tile_idx_end);

        AssetInfo info;
        info.tile_indices = tile_indices;
        info.palette_index = sinfo.palette_index;
        info.width = sinfo.width;
        info.height = sinfo.height;
        infos.emplace_back(info);
    }
}


void parse(const std::string& png_dir_name) {
    parse_pngs(png_dir_name);

    // write tile chunk
    std::ofstream tile_file(data_path(Converter::TILE_CHUNK_FILE), std::ios::binary);
    write_chunk(Converter::TILE_MAGIC, tiles, &tile_file);
    tile_file.close();
    std::cout<<"Tile data output to "<<data_path(Converter::TILE_CHUNK_FILE)<<std::endl;

    // write palette chunk
    std::ofstream palette_file(data_path(Converter::PALETTE_CHUNK_FILE), std::ios::binary);
    write_chunk(Converter::PALETTE_MAGIC, palettes, &palette_file);
    palette_file.close();
    std::cout<<"Palette data output to "<<data_path(Converter::PALETTE_CHUNK_FILE)<<std::endl;

    // write AssetInfo chunk
    std::ofstream asset_info_file(data_path(Converter::ASSET_INFO_CHUNK_FILE), std::ios::binary);
    write_asset_info_chunk(asset_infos, &asset_info_file);
    asset_info_file.close();
    std::cout<<"AssetInfo data output to "<<data_path(Converter::ASSET_INFO_CHUNK_FILE)<<std::endl;


//    /** sample code of read chunk data
    // read tile
    std::vector<PPU466::Tile> converted_tiles{};
    std::ifstream source_tile_file(data_path(Converter::TILE_CHUNK_FILE), std::ios::binary);
    read_chunk(source_tile_file, Converter::TILE_MAGIC, &converted_tiles);

    // read palette
    std::vector<PPU466::Palette> converted_palettes{};
    std::ifstream source_palette_file(data_path(Converter::PALETTE_CHUNK_FILE), std::ios::binary);
    read_chunk(source_palette_file, Converter::PALETTE_MAGIC, &converted_palettes);

    // read asset info
    std::vector<AssetInfo> converted_asset_infos{};
    std::ifstream source_asset_info_file(data_path(Converter::ASSET_INFO_CHUNK_FILE), std::ios::binary);
    read_asset_info_chunk(source_asset_info_file, &converted_asset_infos);
//    **/

    // debug: check if is the same
    assert(tiles.size() == converted_tiles.size());
    for (size_t i=0; i<tiles.size(); i++) {
        assert(tiles[i].bit0 == converted_tiles[i].bit0);
        assert(tiles[i].bit1 == converted_tiles[i].bit1);
    }
    std::cout<<"Tiles check pass!\n";

    assert(palettes.size() == converted_palettes.size());
    for (size_t i=0; i<palettes.size(); i++) {
        assert(palettes[i] == converted_palettes[i]);
    }
    std::cout<<"Palette check pass!\n";

    assert(converted_asset_infos.size() == asset_infos.size());
    for(size_t i=0; i<asset_infos.size(); i++) {
        assert(asset_infos[i].tile_indices == converted_asset_infos[i].tile_indices);
        assert(asset_infos[i].palette_index == converted_asset_infos[i].palette_index);
        assert(asset_infos[i].width == converted_asset_infos[i].width);
        assert(asset_infos[i].height == converted_asset_infos[i].height);
    }
    std::cout<<"Asset info check pass!\n";
}



