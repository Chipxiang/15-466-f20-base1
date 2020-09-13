#include "PPU466.hpp"
#include "Mode.hpp"
#include "asset_converter.hpp"
#include <glm/glm.hpp>

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

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up;

	static const int max_jump_speed = 30;
	static const int min_jump_speed = 8;
	static const int unit_jump_speed = 4;
	static const int gravity_constant = 5;
	
	struct JumpState {
		uint8_t pressed = 0;
		float time = 0.0f;
		float ystart = 0.0f;
		float xstart = 0.0f;
		float speed = 0.0f;
		bool is_jumping = false;
	} jump;
	//some weird background animation:
	float background_fade = 0.0f;

	//player position:
	glm::vec2 player_at = glm::vec2(0.0f, 8.0f);

	//----- drawing handled by PPU466 -----

	PPU466 ppu;
};
