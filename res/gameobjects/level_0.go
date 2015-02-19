level_loader
{
    level = res/levels/0.level
}

collision_handler
{
}

level
{
    parallax
    {
        spritesheet = res/spritesheets/bg.ss
        fill = 0.470588, 0.627450, 0.643137, 1
        offset = 0, 320

        layer
        {
            id = bg_0
            speed = 0.25
            offset = 0, 0
        }

        layer
        {
            id = bg_1
            speed = 0.125
            offset = 0, 170
        }

        layer
        {
            id = bg_2
            speed = 0.065
            offset = 0, 200
        }

    }
}
