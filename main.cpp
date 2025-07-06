#include <iostream>
#include <string>
#include <vector>
#include <termios.h>
#include <unistd.h>
#include <random>


enum TILE_TYPE{
    TAME_GRASS='.',
    WILD_GRASS=',',
    CLEAR=' ',
    WALL='#'
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

void print_map(std::vector<std::vector<char>> map){
    std::cout << "\033[H";
    for (const auto& row : map) {
        for (char tile : row) {
            std::cout << tile;
        }
        std::cout << '\n';
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
    std::vector<std::vector<char>> map(rows, std::vector<char>(cols, CLEAR));

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> prob(0.0, 1.0);

    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            if (i == 0 || j == 0 || i == rows - 1 || j == cols - 1) {
                map[i][j] = '#';
            } else {
                double r = prob(gen);
                double grass_coeff = 1.0;
                double dif_coeff = 0.2;
                if(map[i-1][j] == TAME_GRASS) grass_coeff*=dif_coeff;
                if(map[i][j-1] == TAME_GRASS) grass_coeff*=dif_coeff;
                if (r*grass_coeff < 0.15){
                    map[i][j] = TAME_GRASS;
                    if (r < 0.1){
                        map[i][j] = WILD_GRASS;
                    }
                }
            }
        }
    }

    return map;
}


int main(){
    std::cout << "\033[?25l";
    const int map_rows = 30;
    const int map_cols = map_rows*2;
    const char player_char = '@';
    int player_x = 1;
    int player_y = 1;
    char player_tile = TAME_GRASS;

    std::vector<std::vector<char>> map(generate_map(map_cols, map_rows));

    map[player_y][player_x] = player_char;
    bool done = false;
    while(!done){
        print_map(map);

        char input = get_character_input();
        map[player_y][player_x] = player_tile;
        if (input == 'a' && player_x > 1) player_x--;
        else if (input == 'd' && player_x < map_cols - 2) player_x++;
        else if (input == 'w' && player_y > 1) player_y--;
        else if (input == 's' && player_y < map_rows - 2) player_y++;
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
