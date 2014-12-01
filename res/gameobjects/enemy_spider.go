enemy
{
    walk_anim = walk
    speed = 1.5
    physics = enemy_spider_collision
    collision_trigger = enemy_spider_trigger
}

collision_object enemy_spider_collision
{
    physics = res/physics/characters.physics#enemy_spider
}

collision_object enemy_spider_trigger
{
    physics = res/physics/characters.physics#enemy_spider_trigger
}

sprite_animation walk
{
    spritesheet = res/spritesheets/enemy.ss
    spriteprefix = spider_walk__
    loop = true
    autostart = true
    fps = 8
}
