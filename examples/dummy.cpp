#include <iostream>
#include <memory>
#include <vector>

struct Actor {
    int32_t hp;
};

struct World {
    std::vector<Actor> actors;
};

std::unique_ptr<World> world;

int main(int argc, char *argv[])
{
    world.reset(new World);

    std::cout << ".bss " << (void*)&world << std::endl;

    world->actors.push_back({});

    world->actors.at(0).hp = 300;

    while(1) {
        std::cout << "hp " << world->actors.at(0).hp << std::endl;
        getchar();
        world->actors.at(0).hp -= 1;
    }

    return 0;
}
