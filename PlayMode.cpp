#include "PlayMode.hpp"
//for the GL_ERRORS() macro:
#include "gl_errors.hpp"

//for glm::value_ptr() :
#include <glm/gtc/type_ptr.hpp>
#include <random>

PlayMode::PlayMode() {
	// read tiles
	std::ifstream source_tile_file(data_path(Converter::TILE_CHUNK_FILE), std::ios::binary);
	read_chunk(source_tile_file, Converter::TILE_MAGIC, &converted_tiles);
	// read palettes
	std::ifstream source_palette_file(data_path(Converter::PALETTE_CHUNK_FILE), std::ios::binary);
	read_chunk(source_palette_file, Converter::PALETTE_MAGIC, &converted_palettes);
	// read asset infos
	std::ifstream source_asset_info_file(data_path(Converter::ASSET_INFO_CHUNK_FILE), std::ios::binary);
	read_asset_info_chunk(source_asset_info_file, &asset_infos);

	assert(converted_tiles.size() <= ppu.tile_table.size());
	assert(converted_palettes.size() <= ppu.palette_table.size());

	for (uint32_t i = 0; i < converted_tiles.size(); i++) {
		ppu.tile_table[i] = converted_tiles[i];
	}
	for (uint32_t i = 0; i < converted_palettes.size(); i++) {
		ppu.palette_table[i] = converted_palettes[i];
	}

	player.size.x = asset_infos[player.asset_id].width;
	player.size.y = asset_infos[player.asset_id].height;
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const& evt, glm::uvec2 const& window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_LEFT) {
			left.downs += 1;
			left.pressed = true;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_RIGHT) {
			right.downs += 1;
			right.pressed = true;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_UP) {
			up.downs += 1;
			up.pressed = true;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_DOWN) {
			down.downs += 1;
			down.pressed = true;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_SPACE && jump.is_jumping == false) {
			jump.yspeed += UNIT_JUMP_SPEED;
			if (jump.yspeed > MAX_JUMP_SPEED)
				jump.yspeed = MAX_JUMP_SPEED;
			jump.pressed = true;
		}
	}
	else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_LEFT) {
			left.pressed = false;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_RIGHT) {
			right.pressed = false;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_UP) {
			up.pressed = false;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_DOWN) {
			down.pressed = false;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_SPACE && jump.is_jumping == false && jump.yspeed < MIN_JUMP_SPEED) {
			jump.yspeed = 0;
			jump.pressed = false;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_SPACE && jump.is_jumping == false && jump.yspeed >= MIN_JUMP_SPEED) {
			jump.pressed = false;
			jump.ystart = player.pos.y;
			jump.xstart = player.pos.x;
			jump.time = 0;
			jump.is_jumping = true;
			jump.xspeed = jump.yspeed;
			return true;
		}
	}

	return false;
}

void PlayMode::update(float elapsed) {
	static std::mt19937 mt;
	//slowly rotates through [0,1):
	// (will be used to set background color)
	background_fade += elapsed / 10.0f;
	background_fade -= std::floor(background_fade);
    total_elapsed += elapsed;

    if(!dying && !dead) {
        score = total_elapsed;
    }

    constexpr float PlayerSpeed = 30.0f;
	if (left.pressed) player.pos.x -= PlayerSpeed * elapsed;
	if (right.pressed) player.pos.x += PlayerSpeed * elapsed;
	if (down.pressed) player.pos.y -= PlayerSpeed * elapsed;
	if (up.pressed) player.pos.y += PlayerSpeed * elapsed;

	if (!dying &&  player.pos.x > PPU466::ScreenWidth - player.size.x) {
		jump.is_jumping = true;
		jump.xstart = player.pos.x;
		jump.ystart = player.pos.y;
		jump.xspeed = -10.0f;
		jump.yspeed = 50.0f;
		jump.time = 0.0f;
		dying = true;
	}
	else if (!dying && player.pos.x < asset_infos[killer_id].width) {
		jump.is_jumping = true;
		jump.xstart = player.pos.x;
		jump.ystart = player.pos.y;
		jump.xspeed = 10.0f;
		jump.yspeed = 50.0f;
		jump.time = 0.0f;
		dying = true;
	}
	if (jump.is_jumping && !dead) {
		jump.time += elapsed * 10;
		float temp_y = jump.ystart + jump.yspeed * jump.time - GRAVITY_CONSTANT * jump.time * jump.time;
		player.pos.x = jump.xstart + jump.xspeed / 2 * jump.time;
		// platform
		if (!dying) {
			for (auto& platform : platforms) {
				if (player.pos.x > platform.x - player.size.x && player.pos.x < platform.x + platform.width
					&& temp_y < platform.height) {
					// up
					if (temp_y > platform.height - 8) {
						temp_y = platform.height + 0.0f;
						jump.is_jumping = false;
						jump.yspeed = 0.0f;
						jump.xspeed = 0.0f;
					}
					// leftside
					else {
						jump.xspeed = 0.0f;
						jump.xstart = platform.x - player.size.x - 1;
						player.pos.x = jump.xstart;
						jump.yspeed = jump.yspeed - GRAVITY_CONSTANT * jump.time;
						if (jump.yspeed > 0)
							jump.yspeed = 0.0f;
						jump.time = 0.0f;
						jump.ystart = temp_y;
					}
					break;
				}
			}
		}
		// death
		if (temp_y < 0 && !dying) {
			jump.xstart = player.pos.x;
			jump.ystart = temp_y;
			jump.xspeed = 0.0f;
			jump.yspeed = 50.0f;
		 	jump.time = 0.0f;
			dying = true;
		}
		else if (temp_y < 0 && dying) {
			jump.is_jumping = false;
			jump.yspeed = 0.0f;
			jump.xspeed = 0.0f;
			dead = true;
		}
		player.pos.y = temp_y;
	}
	float scroll_distance = scroll_move_speed * elapsed;
	if (dying) {
		scroll_distance = 0.0f;
	}
	player.pos.x -= scroll_distance;
	jump.xstart -= scroll_distance;
	for (auto& platform : platforms) {
		platform.x -= scroll_distance;
	}

	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;

	// background move left at a constant speed
	background_pos_x -= scroll_distance;
	ppu.background_position.x = (int) background_pos_x;
	ppu.background_position.x %= (int)PPU466::ScreenWidth;

	if (platforms.back().x + new_gap * 8 <= PPU466::ScreenWidth + 8) {
		std::uniform_int_distribution<uint32_t> gap_rand(MIN_GAP, MAX_GAP);
		std::uniform_int_distribution<uint32_t> width_rand(MIN_WIDTH, MAX_WIDTH);
		std::uniform_int_distribution<uint32_t> height_rand(MIN_HEIGHT, MAX_HEIGHT);
		platforms.push_back(Platform{ width_rand(mt) * 8, height_rand(mt) * 8, platforms.back().x + platforms.back().width+ new_gap * 8 });
		new_gap = gap_rand(mt);
	}
	if (platforms.front().x + platforms.front().width <= 0) {
		platforms.pop_front();
	}

	// update killer info make it move up down in a sin wave
	if (!dead && !dying) {
		uint32_t central_y = (uint32_t)player.pos.y;
		killer_y_position = sin(total_elapsed) * killer_move_magnitude + central_y;
	}
	killer_y_position = killer_y_position > asset_infos[fire_id].height ? killer_y_position: asset_infos[fire_id].height;
}

void PlayMode::draw(glm::uvec2 const& drawable_size) {
	//--- set ppu state based on game state ---

	//background color will be some hsv-like fade:
	ppu.background_color = glm::u8vec4(
		0, 0, 0,
		0xff
	);


	//some other misc sprites:
	for (uint32_t i = 0; i < 64; ++i) {
		ppu.sprites[i].x = 0;
		ppu.sprites[i].y = 240;
		/*float amt = (i + 2.0f * background_fade) / 62.0f;
		ppu.sprites[i].x = int32_t(0.5f * PPU466::ScreenWidth + std::cos( 2.0f * M_PI * amt * 5.0f + 0.01f * player.pos.x) * 0.4f * PPU466::ScreenWidth);
		ppu.sprites[i].y = int32_t(0.5f * PPU466::ScreenHeight + std::sin( 2.0f * M_PI * amt * 3.0f + 0.01f * player.pos.y) * 0.4f * PPU466::ScreenWidth);*/
		ppu.sprites[i].index = asset_infos[transparent_id].tile_indices[0];
		ppu.sprites[i].attributes = 6;
		//ppu.sprites[i].attributes |= 0x80; //'behind' bit
	}

	//player sprite:
	if (dying) {
		player.asset_id = player_dead_id;
	}
	else if (jump.is_jumping) {
		player.asset_id = player_jump_id;
	}
	else if (jump.pressed) {
		player.asset_id = player_crouch_id;
	}
	else {
		player.asset_id = player_stand_id;
	}

	uint32_t n_player_rows = asset_infos[player.asset_id].height / 8;
	uint32_t n_player_cols = asset_infos[player.asset_id].width / 8;
	uint32_t player_offset = 0;
	if (!dead) {
		for (uint32_t i = 0; i < n_player_rows; i++) {
			for (uint32_t j = 0; j < n_player_cols; j++) {
				ppu.sprites[player_offset].x = int32_t(player.pos.x + j * 8);
				ppu.sprites[player_offset].y = int32_t(player.pos.y + i * 8);
				ppu.sprites[player_offset].index = asset_infos[player.asset_id].tile_indices[player_offset];
				ppu.sprites[player_offset].attributes = asset_infos[player.asset_id].palette_index;
                player_offset++;
			}
		}
	}

	// draw killer
    uint32_t n_killer_rows = asset_infos[killer_id].height / 8;
    uint32_t n_killer_cols = asset_infos[killer_id].width / 8;
    uint32_t killer_offset = player_offset;
    for (uint32_t i = 0; i < n_killer_rows; i++) {
        for (uint32_t j = 0; j < n_killer_cols; j++) {
            ppu.sprites[killer_offset].x = int32_t(0 + j * 8);
            ppu.sprites[killer_offset].y = int32_t(killer_y_position + i * 8);
            ppu.sprites[killer_offset].index = asset_infos[killer_id].tile_indices[killer_offset - player_offset];
            ppu.sprites[killer_offset].attributes = asset_infos[killer_id].palette_index;
            killer_offset++;
        }
    }

    //draw score
    uint32_t display_score = (uint32_t) score > 999 ? 999 : (uint32_t) score;
    uint8_t units = display_score % 10;
    uint8_t tens = ((display_score - units) / 10) % 10;
    uint8_t hundreds = (display_score - units - tens * 10) / 100;

    uint32_t score_offset = killer_offset;
    // display units
    ppu.sprites[score_offset].x = 3 * 8;
    ppu.sprites[score_offset].y = PPU466::ScreenHeight - 2 * 8;
    ppu.sprites[score_offset].index = asset_infos[score_0_id + units].tile_indices[0];
    ppu.sprites[score_offset].attributes = asset_infos[score_0_id + units].palette_index;
    score_offset++;

    ppu.sprites[score_offset].x = ppu.sprites[score_offset - 1].x;
    ppu.sprites[score_offset].y = ppu.sprites[score_offset - 1].y + 8;
    ppu.sprites[score_offset].index = asset_infos[score_0_id + units].tile_indices[1];
    ppu.sprites[score_offset].attributes = asset_infos[score_0_id + units].palette_index;
    score_offset++;

    // tens
    ppu.sprites[score_offset].x = 2 * 8;
    ppu.sprites[score_offset].y = PPU466::ScreenHeight - 2 * 8;
    ppu.sprites[score_offset].index = asset_infos[score_0_id + tens].tile_indices[0];
    ppu.sprites[score_offset].attributes = asset_infos[score_0_id + tens].palette_index;
    score_offset++;

    ppu.sprites[score_offset].x = ppu.sprites[score_offset - 1].x;
    ppu.sprites[score_offset].y = ppu.sprites[score_offset - 1].y + 8;
    ppu.sprites[score_offset].index = asset_infos[score_0_id + tens].tile_indices[1];
    ppu.sprites[score_offset].attributes = asset_infos[score_0_id + tens].palette_index;
    score_offset++;

    // hundreds
    ppu.sprites[score_offset].x = 1 * 8;
    ppu.sprites[score_offset].y = PPU466::ScreenHeight - 2 * 8;
    ppu.sprites[score_offset].index = asset_infos[score_0_id + hundreds].tile_indices[0];
    ppu.sprites[score_offset].attributes = asset_infos[score_0_id + hundreds].palette_index;
    score_offset++;

    ppu.sprites[score_offset].x = ppu.sprites[score_offset - 1].x;
    ppu.sprites[score_offset].y = ppu.sprites[score_offset - 1].y + 8;
    ppu.sprites[score_offset].index = asset_infos[score_0_id + hundreds].tile_indices[1];
    ppu.sprites[score_offset].attributes = asset_infos[score_0_id + hundreds].palette_index;
    score_offset++;

    /* Draw background of ppu */
	// init every background tile to a "transparent" tile
	for (uint32_t i = 0; i < PPU466::BackgroundHeight; i++) {
		for (uint32_t j = 0; j < PPU466::BackgroundWidth; j++) {
			// use the transparent tile with palette 0(not important)
			ppu.background[i * PPU466::BackgroundWidth + j] = asset_infos[transparent_id].tile_indices[0];
		}
	}

	// draw fire
	for (uint32_t i = 0; i < PPU466::BackgroundWidth; i++) {
		ppu.background[i] = asset_infos[fire_id].tile_indices[0] |
			(asset_infos[fire_id].palette_index << 8);
	}

	for (uint32_t i = 0; i < PPU466::BackgroundWidth; i++) {
		ppu.background[PPU466::BackgroundWidth + i] = asset_infos[fire_id].tile_indices[1] |
			(asset_infos[fire_id].palette_index << 8);
	}
	// draw platforms
	for (auto& platform : platforms) {
		uint32_t nrows = platform.height / 8;
		uint32_t ncols = platform.width / 8;
		for (uint32_t i = 0; i < nrows; i++) {
			for (uint32_t j = 0; j < ncols; j++) {
				float idx = ((platform.x + j * 8 - ppu.background_position.x) / 8) + 1 + PPU466::BackgroundWidth * i;
				if (idx < 0 || idx >= ppu.background.size())
					continue;
				ppu.background[(uint32_t) idx] = asset_infos[brick_id].tile_indices[0] | (asset_infos[brick_id].palette_index << 8);
			}
		}
	}

	//--- actually draw ---
	ppu.draw(drawable_size);
}
