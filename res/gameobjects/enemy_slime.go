enemy
{
    walk_anim = walk
    speed = 1.0
    physics = enemy_slime_collision
    collision_trigger = enemy_slime_trigger
}

collision_object enemy_slime_collision
{
    physics = res/physics/characters.physics#enemy_slime
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
