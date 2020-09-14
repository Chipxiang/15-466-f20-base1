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
	// let the last tile be a transparent tile
    ppu.tile_table[255].bit0 = {
            0b00000000,
            0b00000000,
            0b00000000,
            0b00000000,
            0b00000000,
            0b00000000,
            0b00000000,
            0b00000000,
    };
    ppu.tile_table[255].bit1 = {
            0b00000000,
            0b00000000,
            0b00000000,
            0b00000000,
            0b00000000,
            0b00000000,
            0b00000000,
            0b00000000,
    };

	player.size.x = asset_infos[jump.asset_id].width;
	player.size.y = asset_infos[jump.asset_id].height;

    /* Draw background of ppu */
    // init every background tile to a "transparent" tile
    for(uint32_t i=0; i<PPU466::BackgroundHeight; i++) {
        for(uint32_t j=0; j<PPU466::BackgroundWidth; j++) {
            // use the transparent tile with palette 0(not important)
            ppu.background[i * PPU466::BackgroundWidth + j] = 255;
        }
    }

	// draw fire
	for(uint32_t i=0; i < PPU466::BackgroundWidth; i++) {
	    ppu.background[i] = asset_infos[fire_id].tile_indices[0] |
                (asset_infos[fire_id].palette_index << 8);
	}

    for(uint32_t i=0; i < PPU466::BackgroundWidth; i++) {
        ppu.background[PPU466::BackgroundWidth + i] = asset_infos[fire_id].tile_indices[1] |
                            (asset_infos[fire_id].palette_index << 8);
    }

	//TODO:
	// you *must* use an asset pipeline of some sort to generate tiles.
	// don't hardcode them like this!
	// or, at least, if you do hardcode them like this,
	//  make yourself a script that spits out the code that you paste in here
	//   and check that script into your repository.

	//Also, *don't* use these tiles in your game:

	/*{ //use tiles 0-16 as some weird dot pattern thing:
		std::array< uint8_t, 8*8 > distance;
		for (uint32_t y = 0; y < 8; ++y) {
			for (uint32_t x = 0; x < 8; ++x) {
				float d = glm::length(glm::vec2((x + 0.5f) - 4.0f, (y + 0.5f) - 4.0f));
				d /= glm::length(glm::vec2(4.0f, 4.0f));
				distance[x+8*y] = std::max(0,std::min(255,int32_t( 255.0f * d )));
			}
		}
		for (uint32_t index = 0; index < 16; ++index) {
			PPU466::Tile tile;
			uint8_t t = (255 * index) / 16;
			for (uint32_t y = 0; y < 8; ++y) {
				uint8_t bit0 = 0;
				uint8_t bit1 = 0;
				for (uint32_t x = 0; x < 8; ++x) {
					uint8_t d = distance[x+8*y];
					if (d > t) {
						bit0 |= (1 << x);
					} else {
						bit1 |= (1 << x);
					}
				}
				tile.bit0[y] = bit0;
				tile.bit1[y] = bit1;
			}
			ppu.tile_table[index] = tile;
		}
	}

	//use sprite 32 as a "player":
	ppu.tile_table[32].bit0 = {
		0b01111110,
		0b11111111,
		0b11111111,
		0b11111111,
		0b11111111,
		0b11111111,
		0b11111111,
		0b01111110,
	};
	ppu.tile_table[32].bit1 = {
		0b00000000,
		0b00000000,
		0b00011000,
		0b00100100,
		0b00000000,
		0b00100100,
		0b00000000,
		0b00000000,
	};

	//makes the outside of tiles 0-16 solid:
	ppu.palette_table[0] = {
		glm::u8vec4(0x00, 0x00, 0x00, 0x00),
		glm::u8vec4(0x00, 0x00, 0x00, 0xff),
		glm::u8vec4(0x00, 0x00, 0x00, 0x00),
		glm::u8vec4(0x00, 0x00, 0x00, 0xff),
	};

	//makes the center of tiles 0-16 solid:
	ppu.palette_table[1] = {
		glm::u8vec4(0x00, 0x00, 0x00, 0x00),
		glm::u8vec4(0x00, 0x00, 0x00, 0x00),
		glm::u8vec4(0x00, 0x00, 0x00, 0xff),
		glm::u8vec4(0x00, 0x00, 0x00, 0xff),
	};

	//used for the player:
	ppu.palette_table[7] = {
		glm::u8vec4(0x00, 0x00, 0x00, 0x00),
		glm::u8vec4(0xff, 0xff, 0x00, 0xff),
		glm::u8vec4(0x00, 0x00, 0x00, 0xff),
		glm::u8vec4(0x00, 0x00, 0x00, 0xff),
	};

	//used for the misc other sprites:
	ppu.palette_table[6] = {
		glm::u8vec4(0x00, 0x00, 0x00, 0x00),
		glm::u8vec4(0x88, 0x88, 0xff, 0xff),
		glm::u8vec4(0x00, 0x00, 0x00, 0xff),
		glm::u8vec4(0x00, 0x00, 0x00, 0x00),
	};
	*/
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

	//slowly rotates through [0,1):
	// (will be used to set background color)
	background_fade += elapsed / 10.0f;
	background_fade -= std::floor(background_fade);

	constexpr float PlayerSpeed = 10.0f;
	if (left.pressed) player.pos.x -= PlayerSpeed * elapsed;
	if (right.pressed) player.pos.x += PlayerSpeed * elapsed;
	if (down.pressed) player.pos.y -= PlayerSpeed * elapsed;
	if (up.pressed) player.pos.y += PlayerSpeed * elapsed;

	if (jump.yspeed > 0 && !jump.pressed) {
		jump.time += elapsed * 10;
		float temp_y = jump.ystart + jump.yspeed * jump.time - GRAVITY_CONSTANT * jump.time * jump.time;
		player.pos.x = jump.xstart + jump.xspeed / 2 * jump.time;
		// death
		if (temp_y < 0) {
			temp_y = 0;
			jump.is_jumping = false;
			jump.yspeed = 0.0f;
			jump.xspeed = 0.0f;
			std::cout << player.pos.x << "," << temp_y << std::endl;
		}
		// platform
		for (auto& platform : platforms) {
			if (player.pos.x > platform.x - player.size.x && player.pos.x < platform.x + platform.width 
				&& temp_y < platform.y + platform.height) {
				if (temp_y > platform.y + platform.height - 3) {
					temp_y = platform.y + platform.height + 0.0f;
					jump.is_jumping = false;
					jump.yspeed = 0.0f;
					jump.xspeed = 0.0f;
					break;
				}
				else {
					jump.is_jumping = false;
					jump.xspeed = 0.0f;
					jump.xstart = platform.x - player.size.x;
					jump.yspeed = 0.1f;
					jump.time = 0.0f;
					jump.ystart = temp_y;
					break;
				}
			}
				
		}

		/*for (uint32_t i = (asset_infos[jump.asset_id].width / 8) * (asset_infos[jump.asset_id].height / 8); i < 63; i++)
		{
			if (ppu.sprites[i].y < 240 &&
				player.pos.x > ppu.sprites[i].x - player.size.x && player.pos.x < ppu.sprites[i].x + 8 &&
				temp_y < ppu.sprites[i].y + 8) {
				std::cout << i << "," << (int)ppu.sprites[i].y << std::endl;
				temp_y = ppu.sprites[i].y + 8.0f;
				jump.is_jumping = false;
				jump.speed = 0.0f;
				std::cout << player.pos.x << "," << temp_y << std::endl;
				break;
			}
		}*/
		player.pos.y = temp_y;
	}
	if (player.pos.x > 256)
		player.pos.x -= 256;
	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;

	// background move left at a constant speed
	background_pos_x -= scroll_move_speed * elapsed;
	ppu.background_position.x = (uint32_t) background_pos_x;
}

void PlayMode::draw(glm::uvec2 const& drawable_size) {
	//--- set ppu state based on game state ---

	//background color will be some hsv-like fade:
	ppu.background_color = glm::u8vec4(
		0, 0, 0,
		0xff
	);

	//tilemap gets recomputed every frame as some weird plasma thing:
	//NOTE: don't do this in your game! actually make a map or something :-)
//	for (uint32_t y = 0; y < PPU466::BackgroundHeight; ++y) {
//		for (uint32_t x = 0; x < PPU466::BackgroundWidth; ++x) {
//			//TODO: make weird plasma thing
//			ppu.background[x + PPU466::BackgroundWidth * y] = 100;
//		}
//	}

	//background scroll:
//	ppu.background_position.x = int32_t(-0.5f * player.pos.x);
//	ppu.background_position.y = int32_t(-0.5f * player.pos.y);


	//some other misc sprites:
	for (uint32_t i = 0; i < 64; ++i) {
		ppu.sprites[i].x = 0;
		ppu.sprites[i].y = 240;
		/*float amt = (i + 2.0f * background_fade) / 62.0f;
		ppu.sprites[i].x = int32_t(0.5f * PPU466::ScreenWidth + std::cos( 2.0f * M_PI * amt * 5.0f + 0.01f * player.pos.x) * 0.4f * PPU466::ScreenWidth);
		ppu.sprites[i].y = int32_t(0.5f * PPU466::ScreenHeight + std::sin( 2.0f * M_PI * amt * 3.0f + 0.01f * player.pos.y) * 0.4f * PPU466::ScreenWidth);*/
		ppu.sprites[i].index = 32;
		ppu.sprites[i].attributes = 6;
		//ppu.sprites[i].attributes |= 0x80; //'behind' bit
	}

	//player sprite:
	if (jump.is_jumping) {
		jump.asset_id = player_jump_id;
	}
	else if (jump.pressed) {
		jump.asset_id = player_crouch_id;
	}
	else {
		jump.asset_id = player_stand_id;
	}
	uint32_t n_player_rows = asset_infos[jump.asset_id].height / 8;
	uint32_t n_player_cols = asset_infos[jump.asset_id].width / 8;
	uint32_t count = 0;
	for (uint32_t i = 0; i < n_player_rows; i++) {
		for (uint32_t j = 0; j < n_player_cols; j++) {
			ppu.sprites[count].x = int32_t(player.pos.x + j * 8);
			ppu.sprites[count].y = int32_t(player.pos.y + i * 8);
			ppu.sprites[count].index = asset_infos[jump.asset_id].tile_indices[count];
			ppu.sprites[count].attributes = asset_infos[jump.asset_id].palette_index;
			count++;
		}
	}
	for (auto& platform : platforms) {
		uint32_t nrows = platform.height / 8;
		uint32_t ncols = platform.width / 8;
		for (uint32_t i = 0; i < nrows; i++) {
			for (uint32_t j = 0; j < ncols; j++) {
				ppu.sprites[count].x = int32_t(platform.x + j * 8);
				ppu.sprites[count].y = int32_t(platform.y + i * 8);
				ppu.sprites[count].index = asset_infos[brick_id].tile_indices[0];
				ppu.sprites[count].attributes = asset_infos[brick_id].palette_index;
				count++;
			}
		}
	}
	//--- actually draw ---
	ppu.draw(drawable_size);
}
