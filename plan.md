# Fundamental Functionalities

## Organize code

- remove excessive comments
- better way to represent objects to be rendered in the scene
- separate shaders and integrate them better

### Objective

- Make sure it still compiles

## Load obj files

- player
- objects
- enemies
- map

### Objective

- Make sure it render a scene with any number of objects in it
- Create simple map with objects and the player
	- map may be just a square room
	- objects can be as simple as some boxes or spheres or blue rabbits

## Camera

- free
- look-at

### Objectives

- Free camera: ability to move the camera around the room created earlier
- Look-at: ability to set a look at point or view vector arbitrary

### Player/enemies movement

### Objectives

- Move player around the map
- Make the enemies move toward the player (?)

### Questions

- How to integrate the bezier curves into this step?
	- Maybe the bezier curves should be part of other feature like the drone feature?

## Colision detection

- player against wall/objects/enemies
- bullets against enemies

### Objectives

- Player/enemies should be constrained to the map
- Player/enemies should bump into objects
- Can't have objects going through the wall

## Physics

- Interaction between player and objects/enemies
