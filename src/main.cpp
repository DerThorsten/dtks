
int main()
{
    ant::Parameters params;
    InitWindow(params.shape[0], params.shape[1], "raylib + CMake + C++");
    SetTargetFPS(60);


    ant::AntSimulation simulation(params);

    auto & food_map = simulation.food_map();
    auto & nest_map = simulation.nest_map();    
    auto & is_land = simulation.is_land();


    // make the border of the image non-land
    for(int y = 0; y < params.shape[1]; ++y)
    {
        for(int x = 0; x < params.shape[0]; ++x)
        {
            if(x < 80 || x >= params.shape[0] - 80 || y < 80 || y >= params.shape[1] - 80)
            {
                is_land(x, y) = 0;
            }
        }
    }

    // a 45 degree non land blockage in the middle so the ants cannot go straight through
    for(int y = 0; y < params.shape[1]; ++y)
    {
        for(int x = 0; x < params.shape[0]; ++x)
        {
            if(x > params.shape[0]/4 && x < params.shape[0])
            {
                int blockage_y = (params.shape[0]*3/4 - x); // 45 degree mirrored
                for(int w = -15; w <= 15; ++w) // 10px wide wall
                {
                    if(y == blockage_y + params.shape[1]/4 + w)
                    {
                        is_land(x, y) = 0;
                    }
                }
            }
        }
    }

    ant::discOpening(
        is_land,
        is_land,
        25
    );

     ant::discClosing(
        is_land,
        is_land,
        5
    );

    ant::discOpening(
        is_land,
        is_land,
        10
    );

    ant::discErosion(
        is_land,
        is_land,
        7
    );



    // draw round nest in upper-left corner
    int nest_radius = 50; 
    std::array<int, 2> nest_center = {nest_radius + 200, nest_radius + 200};
    for(int y = 0; y < params.shape[1]; ++y)
    {
        for(int x = 0; x < params.shape[0]; ++x)
        {
            int dx = x - nest_center[0];
            int dy = y - nest_center[1];
            if(dx*dx + dy*dy <= nest_radius * nest_radius)
            {                
                nest_map(x, y) = 255;
            }
        }
    }

    // draw round food source in bottom-right corner
    int food_radius = 50; 
    std::array<int, 2> food_center = {params.shape[0] - food_radius - 200, params.shape[1] - food_radius - 200};
    for(int y = 0; y < params.shape[1]; ++y)
    {
        for(int x = 0; x < params.shape[0]; ++x)
        {
            int dx = x - food_center[0];
            int dy = y - food_center[1];
            if(dx*dx + dy*dy <= food_radius * food_radius)
            {                
                food_map(x, y) = 255;
            }
        }
    }
    simulation.ready();



    bool simulation_started = false;

    while (!WindowShouldClose())
    {
        if (IsKeyPressed(KEY_SPACE)) {
            simulation_started = true;
        }

        if (simulation_started) {
            for(auto i=0; i<5
                
                
                
                
                ; ++i)
                simulation.step();  
        }
        
        simulation.draw();
    }

    CloseWindow();
    return 0;
}
