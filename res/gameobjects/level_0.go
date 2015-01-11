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
        offset = 0, 300

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

        layer
        {
            id = bg_3
            speed = 1
            camera_independent = true
            offset = 0, 500
        }

        layer
        {
            id = bg_4
            speed = 0.65
            camera_independent = true
            offset = 0, 600
        }

        layer
        {
            id = bg_5
            speed = 0.25
            camera_independent = true
            offset = 0, 350
        }

        layer
        {
            id = bg_3
            speed = 1.8
            camera_independent = true
            offset = 400, 800
        }

        layer
        {
            id = bg_5
            speed = 0.4
            camera_independent = true
            offset = 400, 850
        }
    }
}
