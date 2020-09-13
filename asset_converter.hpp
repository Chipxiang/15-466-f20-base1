//
// Created by hao on 9/11/20.
//

#include "PPU466.hpp"
#include <vector>
#include <string>

#ifndef INC_15_466_F20_BASE1_ASSET_CONVERTER_H
#define INC_15_466_F20_BASE1_ASSET_CONVERTER_H

/**
 * Layout of asset data:
 * 3 chunk files:
 *    (1) for all tile data
 *    (2) for all palette data
 *    (3) for all character info data (placement of tile, mapping from tile to palette)
 */




/**
 * Asset file name (with .png extension)
 *
 */

/**
 * asset type table
 * 0: main character-stand
 * 1: main character-crouch
 * 2: main character-jump
 * 3: main character-dead
 * 4: fire
 * 5: platform
 * 6: killer
 */

namespace Converter {
    const std::string DATA_DIR = "data/"; //directory name of data
    const std::string CHUNK_POSTFIX = ".chunk";
    const std::string TILE_CHUNK_FILE = DATA_DIR + "tiles" + CHUNK_POSTFIX;
    const std::string PALETTE_CHUNK_FILE = DATA_DIR + "palettes" + CHUNK_POSTFIX;
    const std::string ASSET_INFO_CHUNK_FILE = DATA_DIR + "asset_infos" + CHUNK_POSTFIX;

    /* Magic string for different data chunk */
    const std::string TILE_MAGIC = "tile";
    const std::string PALETTE_MAGIC = "pale";
    const std::string ASSET_INFO_MAGIC = "aset";
    const std::string TILE_IDX_MAGIC = "tidx";
}

struct AssetInfo {
    // a list of indices into tile table, the lower left 8*8 is the first one
    std::vector<uint8_t> tile_indices;
    // each asset(png) will only use one palette
    uint8_t palette_index;
    // (width/8) * (height/8) tiles to form this character,
    // width*height/64 == tile_indices.size()
    uint8_t width;
    uint8_t height;
};

struct StoredAssetInfo {
    uint32_t tile_idx_begin;
    uint32_t tile_idx_end;
    uint8_t palette_index;
    uint32_t width;
    uint32_t height;
};

void read_asset_info_chunk(std::istream & from, std::vector<AssetInfo> * infos_p);


#endif //INC_15_466_F20_BASE1_ASSET_CONVERTER_H
