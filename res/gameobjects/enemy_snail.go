enemy
{
    walk_anim = walk
    speed = 1.5
    collision_trigger = enemy_snail_trigger
}

collision_object enemy_snail_trigger
{
    physics = res/physics/characters.physics#enemy_snail_trigger
}

sprite_animation walk
{
    spritesheet = res/spritesheets/enemy.ss
    spriteprefix = snail_walk__
    loop = true
    autostart = true
    fps = 3
}
