#include "PlayMode.hpp"

//for the GL_ERRORS() macro:
#include "gl_errors.hpp"

//for glm::value_ptr() :
#include <glm/gtc/type_ptr.hpp>

#include <random>

PlayMode::PlayMode() {
	// read tiles
	std::ifstream source_tile_file(data_path(Converter::TILE_CHUNK_FILE));
	read_chunk(source_tile_file, Converter::TILE_MAGIC, &converted_tiles);
	// read palettes
	std::ifstream source_palette_file(data_path(Converter::PALETTE_CHUNK_FILE));
	read_chunk(source_palette_file, Converter::PALETTE_MAGIC, &converted_palettes);
	// read asset infos
	std::ifstream source_asset_info_file(data_path(Converter::ASSET_INFO_CHUNK_FILE));
	read_asset_info_chunk(source_asset_info_file, &asset_infos);
	
	assert(converted_tiles.size() <= ppu.tile_table.size());
	assert(converted_palettes.size() <= ppu.palette_table.size());

	for (uint32_t i = 0; i < converted_tiles.size(); i++) {
		ppu.tile_table[i] = converted_tiles[i];
	}
	for (uint32_t i = 0; i < converted_palettes.size(); i++) {
		ppu.palette_table[i] = converted_palettes[i];
	}
	//TODO:
	// you *must* use an asset pipeline of some sort to generate tiles.
	// don't hardcode them like this!
	// or, at least, if you do hardcode them like this,
	//  make yourself a script that spits out the code that you paste in here
	//   and check that script into your repository.

	//Also, *don't* use these tiles in your game:

	{ //use tiles 0-16 as some weird dot pattern thing:
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

}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_LEFT) {
			left.downs += 1;
			left.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_RIGHT) {
			right.downs += 1;
			right.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_UP) {
			up.downs += 1;
			up.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_DOWN) {
			down.downs += 1;
			down.pressed = true;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_SPACE && jump.is_jumping == false) {
			jump.speed += unit_jump_speed;
			if (jump.speed > max_jump_speed)
				jump.speed = max_jump_speed;
			jump.pressed = true;
		}
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_LEFT) {
			left.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_RIGHT) {
			right.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_UP) {
			up.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_DOWN) {
			down.pressed = false;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_SPACE && jump.is_jumping == false && jump.speed < min_jump_speed) {
			jump.speed = 0;
			jump.pressed = false;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_SPACE && jump.is_jumping == false && jump.speed >= min_jump_speed) {
			jump.pressed = false;
			jump.ystart = player_at.y;
			jump.xstart = player_at.x;
			jump.time = 0;
			jump.is_jumping = true;
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
	if (left.pressed) player_at.x -= PlayerSpeed * elapsed;
	if (right.pressed) player_at.x += PlayerSpeed * elapsed;
	if (down.pressed) player_at.y -= PlayerSpeed * elapsed;
	if (up.pressed) player_at.y += PlayerSpeed * elapsed;

	if (jump.speed > 0 && !jump.pressed) {
		jump.time += elapsed * 10;
		float temp_y = jump.ystart + jump.speed * jump.time - gravity_constant * jump.time * jump.time;
		player_at.x = jump.xstart + jump.speed / 2 * jump.time;
		// death
		if (temp_y < 0) {
			temp_y = 0;
			jump.is_jumping = false;
			jump.speed = 0.0f;
			std::cout << player_at.x << "," << temp_y << std::endl;
		}
		// platform
		for (uint32_t i = 1; i < 64; i++)
		{
			if (ppu.sprites[i].y < 240 && player_at.x > ppu.sprites[i].x - 7 && player_at.x < ppu.sprites[i].x + 7 &&
				temp_y < ppu.sprites[i].y + 8) {
				temp_y = ppu.sprites[i].y + 8.0f;
				jump.is_jumping = false;
				jump.speed = 0.0f;
				std::cout << player_at.x << "," << temp_y << std::endl;
				break;
			}
		}
		player_at.y = temp_y;
	}
	if (player_at.x > 256)
		player_at.x -= 256;
	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//--- set ppu state based on game state ---

	//background color will be some hsv-like fade:
	ppu.background_color = glm::u8vec4(
		std::min(255,std::max(0,int32_t(255 * 0.5f * (0.5f + std::sin( 2.0f * M_PI * (background_fade + 0.0f / 3.0f) ) ) ))),
		std::min(255,std::max(0,int32_t(255 * 0.5f * (0.5f + std::sin( 2.0f * M_PI * (background_fade + 1.0f / 3.0f) ) ) ))),
		std::min(255,std::max(0,int32_t(255 * 0.5f * (0.5f + std::sin( 2.0f * M_PI * (background_fade + 2.0f / 3.0f) ) ) ))),
		0xff
	);

	//tilemap gets recomputed every frame as some weird plasma thing:
	//NOTE: don't do this in your game! actually make a map or something :-)
	for (uint32_t y = 0; y < PPU466::BackgroundHeight; ++y) {
		for (uint32_t x = 0; x < PPU466::BackgroundWidth; ++x) {
			//TODO: make weird plasma thing
			ppu.background[x+PPU466::BackgroundWidth*y] = ((x+y)%16);
		}
	}

	//background scroll:
	ppu.background_position.x = int32_t(-0.5f * player_at.x);
	ppu.background_position.y = int32_t(-0.5f * player_at.y);

	//player sprite:
	ppu.sprites[0].x = int32_t(player_at.x);
	ppu.sprites[0].y = int32_t(player_at.y);
	ppu.sprites[0].index = 32;
	ppu.sprites[0].attributes = 7;

	ppu.sprites[1].x = int32_t(64);
	ppu.sprites[1].y = int32_t(8);
	ppu.sprites[1].index = 32;
	ppu.sprites[1].attributes = 6;

	ppu.sprites[2].x = int32_t(72);
	ppu.sprites[2].y = int32_t(8);
	ppu.sprites[2].index = 32;
	ppu.sprites[2].attributes = 6;

	ppu.sprites[3].x = int32_t(80);
	ppu.sprites[3].y = int32_t(8);
	ppu.sprites[3].index = 32;
	ppu.sprites[3].attributes = 6;

	ppu.sprites[4].x = int32_t(88);
	ppu.sprites[4].y = int32_t(8);
	ppu.sprites[4].index = 32;
	ppu.sprites[4].attributes = 6;
	//some other misc sprites:
	for (uint32_t i = 5; i < 63; ++i) {
		ppu.sprites[i].x = 0;
		ppu.sprites[i].y = 250;
		/*float amt = (i + 2.0f * background_fade) / 62.0f;
		ppu.sprites[i].x = int32_t(0.5f * PPU466::ScreenWidth + std::cos( 2.0f * M_PI * amt * 5.0f + 0.01f * player_at.x) * 0.4f * PPU466::ScreenWidth);
		ppu.sprites[i].y = int32_t(0.5f * PPU466::ScreenHeight + std::sin( 2.0f * M_PI * amt * 3.0f + 0.01f * player_at.y) * 0.4f * PPU466::ScreenWidth);*/
		ppu.sprites[i].index = 32;
		ppu.sprites[i].attributes = 6;
		//ppu.sprites[i].attributes |= 0x80; //'behind' bit
	}

	//--- actually draw ---
	ppu.draw(drawable_size);
}
