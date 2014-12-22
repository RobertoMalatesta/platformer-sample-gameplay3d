enemy
{
    walk_anim = walk
    speed = 3.0
    collision_trigger = enemy_slime_trigger
}

collision_object enemy_slime_trigger
{
    physics = res/physics/characters.physics#enemy_slime_trigger
}

sprite_animation walk
{
    spritesheet = res/spritesheets/enemy.ss
    spriteprefix = slime_walk__
    loop = true
    autostart = true
    fps = 3
}
