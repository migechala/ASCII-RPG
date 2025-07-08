#include <iostream>
#include <string>
#include <vector>
#include <termios.h>
#include <unistd.h>
#include <random>
#include <algorithm>
#include <sys/ioctl.h>

enum TILE_TYPE{
    TAME_GRASS='.',
    WILD_GRASS=',',
    WATER=' ',
};

class Perlin {
private:
    std::vector<int> p;

    static double fade(double t) {
        return t * t * t * (t * (t * 6 - 15) + 10);
    }

    static double lerp(double t, double a, double b) {
        return a + t * (b - a);
    }

    static double grad(int hash, double x, double y) {
        int h = hash & 15;
        double u = h < 8 ? x : y;
        double v = h < 4 ? y : (h == 12 || h == 14 ? x : 0.0);
        return ((h & 1) ? -u : u) + ((h & 2) ? -v : v);
    }

public:
    Perlin() {
        p.resize(512);
        std::iota(p.begin(), p.begin() + 256, 0);
        std::shuffle(p.begin(), p.begin() + 256, std::mt19937{std::random_device{}()});
        for (int i = 0; i < 256; ++i)
            p[256 + i] = p[i];
    }

    double noise(double x, double y) const {
        int X = (int)std::floor(x) & 255;
        int Y = (int)std::floor(y) & 255;

        x -= std::floor(x);
        y -= std::floor(y);

        double u = fade(x);
        double v = fade(y);

        int A = p[X] + Y, B = p[X + 1] + Y;

        return lerp(v,
                    lerp(u, grad(p[A], x, y), grad(p[B], x - 1, y)),
                    lerp(u, grad(p[A + 1], x, y - 1), grad(p[B + 1], x - 1, y - 1))
        );
    }
};


char get_character_input(){
    termios oldt{}, newt{};
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    char ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}

void print_map(std::vector<std::vector<char>> map, int player_x, int player_y,
               int view_rows, int view_cols){
    int map_rows = map.size();
    int map_cols = map[0].size();

    int half_rows = view_rows/2;
    int half_cols = view_cols/2;

    int start_row = std::max(0, player_y - half_rows);
    int start_col = std::max(0, player_x - half_cols);

    if(start_row + view_rows > map_rows) start_row = map_rows - view_rows;
    if(start_col + view_cols > map_cols) start_col = map_cols - view_cols;

    std::cout << "\033[H";
    for (int i = 0; i < view_rows; i++) {
        for(int j = 0; j < view_cols; j++){
            int map_row = start_row + i;
            int map_col = start_col + j;

            std::cout << map[map_row][map_col];
        }
        std::cout << std::endl;
    }
}

bool check_wild_grass_event(char tile) {
    if (tile == WILD_GRASS) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<int> dist(1, 100);

        if (dist(gen) <= 25) {
            return true;
        }
    }
    return false;
}

std::vector<std::vector<char>> generate_map(int cols, int rows) {
    std::vector<std::vector<char>> map(rows, std::vector<char>(cols, WATER));
    Perlin noise;

    double scale = 0.1;
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            double n = noise.noise(i * scale, j * scale);
            double val = (n + 1) / 2.0;
            if (val < 0.35){
                map[i][j] = WATER;
            }
            else if (val < 0.5){
                map[i][j] = WILD_GRASS;
            }
            else{
                map[i][j] = TAME_GRASS;
            }
        }
    }
    return map;
}
int main(){
    std::cout << "\033[?25l";
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

    const int map_rows = 1000;
    const int map_cols = 1000;
    const char player_char = '@';
    int player_x = 0;
    int player_y = 0;
    char player_tile = TAME_GRASS;

    std::vector<std::vector<char>> map(generate_map(map_cols, map_rows));

    map[player_y][player_x] = player_char;
    bool done = false;
    while(!done){
        print_map(map, player_x, player_y, w.ws_row-1, w.ws_col);

        char input = get_character_input();
        map[player_y][player_x] = player_tile;
        if (input == 'a' && player_x > 0 && map[player_y][player_x - 1] != WATER) player_x--;
        else if (input == 'd' && player_x < map_cols - 1 &&
                map[player_y][player_x + 1] != WATER) player_x++;
        else if (input == 'w' && player_y > 0 &&
                map[player_y-1][player_x] != WATER) player_y--;
        else if (input == 's' && player_y < map_rows - 1 &&
                map[player_y+1][player_x] != WATER) player_y++;
        else if (input == 'q') done = true;
        player_tile = map[player_y][player_x];
        map[player_y][player_x] = player_char;

        if(check_wild_grass_event(player_tile)){
            //BATTLE SEQUENCE!!
        }

    }
    std::cout << "\033[?25h";
    return 0;
}
