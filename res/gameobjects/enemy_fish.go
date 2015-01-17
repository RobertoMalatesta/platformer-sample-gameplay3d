enemy
{
    walk_anim = swim
    death_anim = dead
    speed = 3.0
    collision_trigger = enemy_fish_trigger
    snap_to_collision_y = false
}

collision_object enemy_fish_trigger
{
    physics = res/physics/characters.physics#enemy_fish_trigger
}

sprite_animation swim
{
    spritesheet = res/spritesheets/enemy.ss
    spriteprefix = fish_swim__
    loop = true
    autostart = true
    fps = 3
}

sprite_animation dead
{
    spritesheet = res/spritesheets/enemy.ss
    spriteprefix = fish_dead
    fps = 0
}
