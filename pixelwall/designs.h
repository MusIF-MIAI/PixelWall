/*
DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
                    Version 2, December 2004

Copyright (C) 2004 Sam Hocevar <sam@hocevar.net>

Everyone is permitted to copy and distribute verbatim or modified
copies of this license document, and changing it is allowed as long
as the name is changed.

DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION

0. You just DO WHAT THE FUCK YOU WANT TO.
*/

#pragma once

#include "pixelwall.h"

extern Design snakeDesign;
extern Design randomPixelsDesign;
extern Design textDesign;
extern Design pongDesign;
extern Design lifeDesign;
extern Design cm5Design;
extern Design arcadeDesign;

Design *designs[] = {
    &snakeDesign,
    &randomPixelsDesign,
    &textDesign,
    &pongDesign,
    &lifeDesign,
    &cm5Design,
    &arcadeDesign,
};