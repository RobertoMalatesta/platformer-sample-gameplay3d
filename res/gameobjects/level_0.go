level
{
    level = res/levels/0.level
}

collision_handler
{
}

level_renderer
{
    parallax
    {
        spritesheet = res/spritesheets/bg.ss
        fill = 0.470588, 0.627450, 0.643137, 1
        offset = 0, 0

        layer
        {
            id = bg_0
            speed = 0.05
            offset = 0, 0
        }

        layer
        {
            id = bg_1
            speed = 0.025
            offset = 0, -170
        }

        layer
        {
            id = bg_2
            speed = 0.005
            offset = 0, -200
        }
    }
}
