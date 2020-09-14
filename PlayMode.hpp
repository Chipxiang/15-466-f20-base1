#include "PPU466.hpp"
#include "Mode.hpp"
#include "asset_converter.hpp"
#include "data_path.hpp"
#include "read_write_chunk.hpp"
#include <glm/glm.hpp>
#include <fstream>
#include <vector>
#include <deque>

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();
	
	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	// assets

	// tile
	std::vector<PPU466::Tile> converted_tiles{};
	// palette
	std::vector<PPU466::Palette> converted_palettes{};
	// read asset info
	std::vector<AssetInfo> asset_infos{};

	//----- game state -----
	enum AssetIndex
	{
		player_stand_id, player_crouch_id, player_jump_id, player_dead_id, fire_id, brick_id, killer_id, transparent_id
	};
	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up;

	static const uint32_t MAX_JUMP_SPEED = 40;
	static const uint32_t MIN_JUMP_SPEED = 8;
	static const uint32_t UNIT_JUMP_SPEED = 4;
	static const uint32_t GRAVITY_CONSTANT = 5;
	static const uint32_t MIN_GAP = 4;
	static const uint32_t MAX_GAP = 8;
	static const uint32_t MIN_WIDTH = 1;
	static const uint32_t MAX_WIDTH = 6;
	static const uint32_t MIN_HEIGHT = 3;
	static const uint32_t MAX_HEIGHT = 8;

	struct JumpState {
		uint32_t asset_id = player_stand_id;
		uint8_t pressed = 0;
		float time = 0.0f;
		float ystart = 0.0f;
		float xstart = 0.0f;
		float yspeed = 0.0f;
		float xspeed = 0.0f;
		bool is_jumping = false;
	} jump;
	//some weird background animation:
	float background_fade = 0.0f;

	struct Platform {
		uint32_t width;
		uint32_t height;
		float x;
	};
	std::deque<Platform> platforms = { Platform{40, 40, 64.0f},  Platform{40, 40, 160.0f} };
	uint32_t new_gap = 4;

	//player information:
	struct PlayerInfo {
		glm::vec2 pos = glm::vec2(64.0f, 40.0f);
		glm::vec2 size = glm::vec2(8.0f, 8.0f);
	} player;

    // scroll move speed (pix/s)
    float scroll_move_speed = 20.0f;

	// since the ppu background is using int, it may round each move to 0, we add a double here
	// for a more pricise track.
	double background_pos_x = 0;
	
	//----- drawing handled by PPU466 -----

	PPU466 ppu;
};
