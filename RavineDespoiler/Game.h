/*
   Copyright (C) 2020 Ben Combee (@unwiredben)

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#pragma once

#include <Arduboy2.h>
#include <ArduboyTones.h>

#include <FixedPoints.h>
using Number = SFixed<7, 8>;
using BigNumber = SFixed<15, 16>;

#include "Util.h"

#include "Font4x6.h"
#include "logo_bmp.h"
#include "plane_bmp.h"
#include "press_a_bmp.h"
#include "ravine_bmp.h"
#include "titlecard_cmpbmp.h"
#include "unwired_logo_bmp.h"
#include "zeppelin_bmp.h"

/* global definitions for APIs */
Arduboy2 arduboy;
ArduboyTones sound(arduboy.audio.enabled);
Sprites sprites;

namespace RavineDespoilerGame {

// Simulates friction
// Not actually how a real coefficient of friction works
constexpr Number CoefficientOfFriction = 0.95;

// Simulates gravity
// Earth's gravitational pull is 9.8 m/s squared
// But that's far too powerful for the tiny screen
// So I picked something small
constexpr Number CoefficientOfGravity = 0.5;

// Simulates bounciness
// Again, not quite like the real deal
constexpr Number CoefficientOfRestitution = 0.7;

// Prevents never-ending bounciness
constexpr Number RestitutionThreshold = Number::Epsilon * 16;

// Amount of force the player exerts
constexpr Number InputForce = 0.25;

struct GameObject {
  BigNumber x = 0, x_min = 0, x_max = WIDTH, x_vel = 0;
  Number y = 0, y_min = 0, y_max = HEIGHT, y_vel = 0;

  void move(BigNumber newX, Number newY) {
    x = newX;
    x = constrain(x, x_min, x_max);
    y = newY;
    y = constrain(y, y_min, y_max);
  }
  void adjust(BigNumber dx, Number dy) { move(x + dx, y + dy); }

  // for this game, we use a looping x coordinate system
  // where going off one end will move you to the other
  void applyXVelocity() {
    x = x + x_vel;
    if (x < x_min) {
      x = x_min;
      x_vel = -x_vel;
    } else if (x > x_max) {
      x = x_max;
      x_vel = -x_vel;
    }
    // x_vel = x_vel * CoefficientOfFriction;
  }

  void applyYVelocity() {
    if (y_vel != 0) {
      y = y + y_vel;
      if (y < y_min) {
        y = y_min;
        y_vel = -y_vel;
      } else if (y > y_max) {
        y = y_max;
        y_vel = -y_vel * CoefficientOfRestitution;
      }
    }
    y_vel = y_vel + CoefficientOfGravity;
  }

  void applyVelocity() {
    applyXVelocity();
    applyYVelocity();
  }
};

struct Ravine {
  void draw() {
    sprites.drawOverwrite(0, ravine_top, ravine_bmp, 0);
  }
} ravine;

struct Plane : public GameObject {
  static constexpr BigNumber offscreen_x_margin = 10;
  Plane() {
    x_min = -offscreen_x_margin - plane_width;
    x_max = WIDTH + offscreen_x_margin;
    y_min = 0;
    y_max = ravine_top - plane_height - 2;
  }

  void applyXVelocity() {
    GameObject::applyXVelocity();
    if (x == x_min || x == x_max) {
      y = randomSFixed(y_min, y_max);
    }
  }

  void reset() {
    x = 10;
    y = 2;
    x_vel = 1;
  }

  void draw() {
    int8_t ix = x.getInteger(), iy = y.getInteger();
    sprites.drawPlusMask(ix, iy, plane_plus_mask, x_vel > 0);
  }
} plane;

struct Zeppelin : public GameObject {
  static constexpr BigNumber offscreen_x_margin = 20;
  Zeppelin() {
    x_min = -offscreen_x_margin - plane_width;
    x_max = WIDTH + offscreen_x_margin;
    y_min = y_max = 0;
  }

  void reset() {
    x = x_max;
    y = 0;
    x_vel = -0.5;
  }

  void draw() {
    int8_t ix = x.getInteger(), iy = y.getInteger();
    sprites.drawPlusMask(ix, iy, zeppelin_plus_mask, x_vel > 0);
  }
} zeppelin;

// coordinating game state
enum GameState {
  INITIAL_LOGO,
  TITLE_SCREEN,
  OBJECTIVE_SCREEN,
  GAME_ACTIVE,
  LEVEL_COMPLETE
} state = INITIAL_LOGO;

void enter_state(GameState newState) {
  arduboy.frameCount = 0;
  sound.noTone();
  state = newState;

  if (newState == TITLE_SCREEN) {
    // reset game state for a new game
  } else if (newState == GAME_ACTIVE) {
    // reset UI state for a new level
    randomSeed(arduboy.generateRandomSeed());
    plane.reset();
  }
}

void initial_logo() {
  if (arduboy.frameCount == 1) {
    arduboy.clear();
    arduboy.drawCompressed(0, 0, unwiredgames_logo_cmpimg);
  }
  if (arduboy.frameCount > 90) {
    enter_state(TITLE_SCREEN);
  }
}

void title_screen() {
  if (arduboy.frameCount == 1) {
    zeppelin.reset();
    arduboy.clear();
    ravine.draw();
    sprites.drawOverwrite(logo_x, logo_y, logo_bmp, 0);
  }
  if (arduboy.frameCount == 180) {
    arduboy.fillRect(24, 48, press_a_to_start_width, press_a_to_start_height,
                     BLACK);
    arduboy.drawCompressed(24, 48, press_a_to_start_cmpbmp);
  }

  arduboy.fillRect(zeppelin.x.getInteger(), zeppelin.y.getInteger(),
                   zeppelin_width, zeppelin_height, BLACK);
  zeppelin.applyXVelocity();
  zeppelin.draw();

  if (arduboy.justPressed(A_BUTTON)) {
    // enter_state(OBJECTIVE_SCREEN);
    enter_state(GAME_ACTIVE);
  }
}

void objective_screen() {
  if (arduboy.frameCount == 1) {
    arduboy.clear();
    // arduboy.drawCompressed(0, 0, objective_cmpimg);
  }
  if (arduboy.frameCount > 120) {
    enter_state(GAME_ACTIVE);
  }
}

void game_active() {
  // process input
  plane.x_vel = plane.x_vel < 0 ? -1 : 1;
  if (arduboy.pressed(LEFT_BUTTON)) {
    plane.x_vel = plane.x_vel < 0 ? -1.5 : 0.5;
  }
  if (arduboy.pressed(RIGHT_BUTTON)) {
    plane.x_vel = plane.x_vel < 0 ? -0.5 : 1.5;
  }
  plane.applyXVelocity();

  arduboy.clear();
  ravine.draw();
  plane.draw();

  // temporary change to allow resetting
  if (arduboy.pressed(A_BUTTON) && arduboy.justPressed(B_BUTTON)) {
    enter_state(INITIAL_LOGO);
  }
}

void level_complete() {
  if (arduboy.frameCount > 150 && !sound.playing()) {
    enter_state(TITLE_SCREEN);
  }
}

void setup(void) {
  arduboy.begin();
  arduboy.setFrameRate(60);
  enter_state(INITIAL_LOGO);
}

void loop(void) {
  if (!(arduboy.nextFrameDEV()))
  // if (!(arduboy.nextFrame()))
    return;

  arduboy.pollButtons();

  switch (state) {
  case INITIAL_LOGO:
    initial_logo();
    break;
  case TITLE_SCREEN:
    title_screen();
    break;
  case OBJECTIVE_SCREEN:
    objective_screen();
    break;
  case GAME_ACTIVE:
    game_active();
    break;
  case LEVEL_COMPLETE:
    level_complete();
    break;
  }

  arduboy.display();
}

} // namespace RavineDespoilerGame