/*
vertical plotter

Written in 2022 by Lukas Holzbeierlein (Captain4LK) email: captain4lk [at] tutanota [dot] com

To the extent possible under law, the author(s) have dedicated all copyright and related and neighboring rights to this software to the public domain worldwide. This software is distributed without any warranty.

You should have received a copy of the CC0 Public Domain Dedication along with this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>. 
*/

#ifndef _STEPPER_H_

#define _STEPPER_H_

void stepper_init(void);
void stepper_linear_move_to(int x, int y);
void stepper_move(int step0, int step1, int speed0, int speed1);
void stepper_move_home(void);

#endif
