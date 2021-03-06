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

#ifndef _BATTLE_ANIMATION_H_
#define _BATTLE_ANIMATION_H_

// Headers
#include "system.h"
#include "rpg_animation.h"
#include "drawable.h"

struct FileRequestResult;

class BattleAnimation : public Drawable {
public:
	BattleAnimation(int x, int y, const RPG::Animation* animation);
	~BattleAnimation();

	void Draw();
	int GetZ() const;
	void SetZ(int nz);
	DrawableType GetType() const;

	int GetX() const;
	void SetX(int nx);
	int GetY() const;
	void SetY(int ny);
	void Update();
	int GetFrame() const;
	int GetFrames() const;
	void SetFrame(int);
	bool IsDone() const;

protected:
	void OnBattleSpriteReady(FileRequestResult* result);
	void OnBattle2SpriteReady(FileRequestResult* result);

	int x;
	int y;
	int z;
	const RPG::Animation* animation;
	int frame;
	bool large;
	bool frame_update;
	BitmapRef screen;
};

#endif
