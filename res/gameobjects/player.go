player
{
    idle_anim = idle
    walk_anim = walk
    cower_anim = cower
    jump_anim = jump
    climb_anim = climb
    swim_anim = swim
    speed = 9.0
    swim_speed_scale = 0.5
    jump_height = 2.1
    normal_physics = character_normal
}

player_input
{
}

player_hand_of_god
{
}

collision_object character_normal
{
    physics = res/physics/characters.physics#player_normal
}

sprite_animation walk
{
    spritesheet = res/spritesheets/player.ss
    spriteprefix = player_walk__
    loop = true
}

sprite_animation climb : walk
{
    spriteprefix = player_climb__
    fps = 5
}

sprite_animation idle
{
    spritesheet = res/spritesheets/player.ss
    sprite = player_stand
    sprite = player_front
    fps = 0.25
    loop = true
    autostart = true
}

sprite_animation cower
{
    spritesheet = res/spritesheets/player.ss
    spriteprefix = player_hurt
}

sprite_animation jump
{
    spritesheet = res/spritesheets/player.ss
    sprite = player_jump
    sprite = player_hurt
    fps = 3
}

sprite_animation swim
{
    spritesheet = res/spritesheets/player.ss
    spriteprefix = player_swim__
    fps = 4
    loop = true
    autostart = true
}
