/*
 * This file is part of EasyRPG Player.
 *
 * EasyRPG Player is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * EasyRPG Player is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with EasyRPG Player. If not, see <http://www.gnu.org/licenses/>.
 */

// Headers
#ifdef _MSC_VER
#  pragma warning(disable: 4003)
#endif

#include <map>

#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/static_assert.hpp>

#include "async_handler.h"
#include "cache.h"
#include "filefinder.h"
#include "exfont.h"
#include "bitmap.h"
#include "output.h"
#include "player.h"
#include "data.h"

namespace {
	typedef std::pair<std::string,std::string> string_pair;
	typedef std::pair<std::string, int> tile_pair;

	typedef std::map<string_pair, EASYRPG_WEAK_PTR<Bitmap> > cache_type;
	cache_type cache;

	typedef std::map<tile_pair, EASYRPG_WEAK_PTR<Bitmap> > cache_tiles_type;
	cache_tiles_type cache_tiles;

	static std::string system_name;

	BitmapRef LoadBitmap(std::string const& folder_name, const std::string& filename,
						 bool transparent, uint32_t const flags) {
		string_pair const key(folder_name, filename);

		cache_type::const_iterator const it = cache.find(key);

		if (it == cache.end() || it->second.expired()) {
			std::string const path = FileFinder::FindImage(folder_name, filename);

			if (path.empty()) {
				return BitmapRef();
			}

			return (cache[key] = Bitmap::Create(path, transparent, flags)).lock();
		} else { return it->second.lock(); }
	}

	struct Material {
		enum Type {
			REND = -1,
			Backdrop,
			Battle,
			Charset,
			Chipset,
			Faceset,
			Gameover,
			Monster,
			Panorama,
			Picture,
			System,
			Title,
			System2,
			Battle2,
			Battlecharset,
			Battleweapon,
			Frame,
			END
		};

	}; // struct Material

	struct Spec {
		char const* directory;
		bool transparent;
		int min_width , max_width ;
		int min_height, max_height;
	} const spec[] = {
		{ "Backdrop", false, 320, 320, 160, 160 },
		{ "Battle", true, 480, 480, 96, 480 },
		{ "CharSet", true, 288, 288, 256, 256 },
		{ "ChipSet", true, 480, 480, 256, 256 },
		{ "FaceSet", true, 192, 192, 192, 192 },
		{ "GameOver", false, 320, 320, 240, 240 },
		{ "Monster", true, 16, 320, 16, 160 },
		{ "Panorama", false, 80, 640, 80, 480 },
		{ "Picture", true, 1, 640, 1, 480 },
		{ "System", true, 160, 160, 80, 80 },
		{ "Title", false, 320, 320, 240, 240 },
		{ "System2", true, 80, 80, 96, 96 },
		{ "Battle2", true, 640, 640, 640, 640 },
		{ "BattleCharSet", true, 144, 144, 384, 384 },
		{ "BattleWeapon", true, 192, 192, 512, 512 },
		{ "Frame", true, 320, 320, 240, 240 },
	};

	template<Material::Type T>
	BitmapRef LoadDummyBitmap(std::string const& folder_name, const std::string& filename) {
		BOOST_STATIC_ASSERT(Material::REND < T && T < Material::END);

		Spec const& s = spec[T];

		string_pair const key(folder_name, filename);

		BitmapRef bitmap = Bitmap::Create(s.max_width, s.max_height, false);

		// ToDo: Maybe use different renderers depending on material
		// Will look ugly for some image types (especially System)

		// Draw chess board
		Color color[2] = { Color(255, 255, 255, 255), Color(128, 128, 128, 255) };
		for (int i = 0; i < s.max_width / 16; ++i) {
			for (int j = 0; j < s.max_height / 16; ++j) {
				bitmap->FillRect(Rect(i * 16, j * 16, 16, 16), color[(i + j) % 2]);
			}
		}

		// Draw filename
		bitmap->TextDraw(4, 4, Color(255, 0, 0, 0), folder_name + "/" + filename);

		return (cache[key] = bitmap).lock();
	}

	template<Material::Type T>
	BitmapRef LoadBitmap(std::string const& f, bool transparent) {
		BOOST_STATIC_ASSERT(Material::REND < T && T < Material::END);

		Spec const& s = spec[T];

		// Test if the file was requested asynchronously before.
		// If not the file can't be expected to exist -> bug.
		FileRequestAsync* request = AsyncHandler::RequestFile(s.directory, f);
		if (!request->IsReady()) {
			Output::Debug("BUG Not Requested: %s/%s", s.directory, f.c_str());
			return BitmapRef();
		}

		BitmapRef ret = LoadBitmap(s.directory, f, transparent,
										 T == Material::Chipset? Bitmap::Chipset:
										 T == Material::System? Bitmap::System:
										 0);

		if (!ret) {
			Output::Warning("Image not found: %s/%s", s.directory, f.c_str());

			return LoadDummyBitmap<T>(s.directory, f);
		}

		if(
			ret->GetWidth () < s.min_width  || s.max_width  < ret->GetWidth () ||
			ret->GetHeight() < s.min_height || s.max_height < ret->GetHeight()
		) {
			Output::Debug("Image has non-default size: %s/%s\nwidth  (min, max, actual) = (%d, %d, %d)\nheight (min, max, actual) = (%d, %d, %d)",
						  s.directory, f.c_str(), s.min_width , s.max_width , ret->GetWidth (), s.min_height, s.max_height, ret->GetHeight());
		}

		return ret;
	}
}

#define macro(r, data, elem) \
	BitmapRef Cache::elem(const std::string& f) { \
		bool trans = spec[Material::elem].transparent; \
		return LoadBitmap<Material::elem>(f, trans); \
	}

BOOST_PP_SEQ_FOR_EACH(macro, ,
					  (Backdrop)(Battle)(Battle2)(Battlecharset)(Battleweapon)
					  (Charset)(Chipset)(Faceset)(Gameover)(Monster)
					  (Panorama)(System)(System2)(Frame)(Title)
					  )

#undef macro

BitmapRef Cache::Picture(const std::string& f, bool trans) {
	return LoadBitmap<Material::Picture>(f, trans);
}

BitmapRef Cache::Exfont() {
	string_pair const hash("\x00","ExFont");

	cache_type::const_iterator const it = cache.find(hash);

	if (it == cache.end() || it->second.expired()) {
		return(cache[hash] = Bitmap::Create(exfont_h, sizeof(exfont_h), true)).lock();
	} else { return it->second.lock(); }
}

BitmapRef Cache::Tile(const std::string& filename, int tile_id) {
	tile_pair const key(filename, tile_id);
	cache_tiles_type::const_iterator const it = cache_tiles.find(key);

	if (it == cache_tiles.end() || it->second.expired()) {
		BitmapRef chipset = Cache::Chipset(filename);
		Rect rect = Rect(0, 0, 16, 16);

		int sub_tile_id = 0;

		if (tile_id > 0 && tile_id < 48) {
			sub_tile_id = tile_id;
			rect.x += 288;
			rect.y += 128;
		} else if (tile_id >= 48 && tile_id < 96) {
			sub_tile_id = tile_id - 48;
			rect.x += 384;
		} else if (tile_id >= 96 && tile_id < 144) {
			sub_tile_id = tile_id - 96;
			rect.x += 384;
			rect.y += 128;
		} else { // Invalid -> Use empty file (first one)
			rect.x = 288;
			rect.y = 128;
		}

		rect.x += sub_tile_id % 6 * 16;
		rect.y += sub_tile_id / 6 * 16;

		return(cache_tiles[key] = Bitmap::Create(*chipset, rect)).lock();
	} else { return it->second.lock(); }
}

void Cache::Clear() {
	for(cache_type::const_iterator i = cache.begin(); i != cache.end(); ++i) {
		if(i->second.expired()) { continue; }
		Output::Debug("possible leak in cached bitmap %s/%s",
					  i->first.first.c_str(), i->first.second.c_str());
	}
	cache.clear();

	for(cache_tiles_type::const_iterator i = cache_tiles.begin(); i != cache_tiles.end(); ++i) {
		if(i->second.expired()) { continue; }
		Output::Debug("possible leak in cached tilemap %s/%d",
					  i->first.first.c_str(), i->first.second);
	}
	cache_tiles.clear();
}

void Cache::SetSystemName(std::string const& filename) {
	system_name = filename;
}

BitmapRef Cache::System() {
	if (!system_name.empty()) {
		return Cache::System(system_name);
	} else {
		if (!Data::system.system_name.empty()) {
			// Load the system file for the shadow and text color
			return Cache::System(Data::system.system_name);
		} else {
			return Bitmap::Create(160, 80, false);
		}
	}
}
