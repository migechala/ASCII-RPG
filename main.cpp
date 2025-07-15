#include <iostream>
#include <string>
#include <vector>
#include <termios.h>
#include <unistd.h>
#include <random>
#include <algorithm>
#include <sys/ioctl.h>
#include <stack>
#include <memory>
#include <fstream>
#include <chrono>

enum TILE_TYPE{
    TAME_GRASS='.',
    WILD_GRASS=',',
    FOREST='^',
    BUSH=';',
    WATER=' ',
};

enum ATTACK {
    PUNCH=0,
    EMBER,
    WAVE,
    QUICK_ATTACK
};

enum HOSTILE_TYPE{
    WASP=0
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



void print_map(std::vector<std::vector<char>> map, int player_x, int player_y,
               int view_rows, int view_cols, int scale){
    int map_rows = map.size();
    int map_cols = map[0].size();

    int half_rows = view_rows/2;
    int half_cols = view_cols/2;

    int start_row = std::max(0, player_y - half_rows);
    int start_col = std::max(0, player_x - half_cols);

    if(start_row + view_rows > map_rows) start_row = map_rows - view_rows;
    if(start_col + view_cols > map_cols) start_col = map_cols - view_cols;

    for (int i = 0; i < view_rows; i++) {
        for(int line = 0; line < scale; line++){
            for(int j = 0; j < view_cols; j++){
                int map_row = start_row + i;
                int map_col = start_col + j;
                char tile = map[map_row][map_col];
                std::string big_tile = std::string(scale, tile);
                std::cout << big_tile;
            }
            std::cout << std::endl;
        }
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
            else if (val < 0.45){
                map[i][j] = WILD_GRASS;
            }
            else if (val < 0.453){
                map[i][j] = BUSH;
            }
            else if (val < 0.55){
                map[i][j]=FOREST;
            }
            else{
                map[i][j] = TAME_GRASS;
            }
        }
    }
    return map;
}

struct Text{
    std::string title;
    std::string text;
    std::vector<std::string> options;
    Text(std::string title, std::string text, std::vector<std::string>
             options={}) : title(title), text(text), options(options) {}
    Text() : title(""), text(""), options({}) {}
};

void add_text_box(Text text_box, int rows, int cols) {
    std::string title = text_box.title;
    std::string text = text_box.text;
    auto options = text_box.options;


    if (rows < 5) rows = 5;
    if (cols < 10) cols = 10;

    int title_len = (int)title.size();
    int side_space = (cols - 2 - title_len) / 2;

    std::cout << "+" << std::string(cols - 2, '-') << "+\n";

    std::cout << "|"
              << std::string(side_space, ' ')
              << title
              << std::string(cols - 2 - side_space - title_len, ' ')
              << "|\n";

    std::cout << "+" << std::string(cols - 2, '-') << "+\n";
    int text_rows = rows - 6;
    if (text.length()!=0){
        if (text_rows < 1) text_rows = 1;
        std::vector<std::string> text_lines;
        for (size_t start = 0; start < text.size();) {
            int len = std::min(cols - 4, (int)(text.size() - start));
            text_lines.push_back(text.substr(start, len));
            start += len;
        }

        for (int i = 0; i < text_rows; i++) {
            std::string line = i < (int)text_lines.size() ? text_lines[i] : "";
            std::cout << "| " << line;
            std::cout << std::string(cols - 3 - line.size(), ' ') << "|\n";
        }
        std::cout << "+" << std::string(cols - 2, '-') << "+\n";
    }
    else{
        text_rows = 0;
    }
    int option_rows = rows - 4 - text_rows;
    for (int i = 0; i < option_rows && i < (int)options.size(); i++) {
        std::string opt = options[i];
        if ((int)opt.size() > cols - 4) {
            opt = opt.substr(0, cols - 7) + "...";
        }
        std::cout << "| " << opt << std::string(cols - 3 - opt.size(), ' ') << "|\n";
    }

    for (int i = (int)options.size(); i < option_rows; i++) {
        std::cout << "|" << std::string(cols - 2, ' ') << "|\n";
    }

    std::cout << "+" << std::string(cols - 2, '-') << "+\n";
}

char get_character_input(bool blocking=false) {
    static struct termios oldt, newt;
    static bool initialized = false;
    if(!initialized){
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
        initialized = true;
    }

    int bytes_waiting;
    char c = '\0';
    if(!blocking){
        ioctl(STDIN_FILENO, FIONREAD, &bytes_waiting);
        if(bytes_waiting > 0){
            read(STDIN_FILENO, &c, 1);
        }
    }
    else{
        return getchar();
    }
    return c;
}

void reset_terminal() {
    static struct termios oldt;
    tcgetattr(STDIN_FILENO, &oldt);
    oldt.c_lflag |= (ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
}

std::string tile_type_to_string(char tile){
    switch(tile){
        case TAME_GRASS:
            return "Tame Grass (Safe)";
        case WILD_GRASS:
            return "Wild Grass (Dangerous)";
        case BUSH:
            return "Bush (Unsafe)";
        case WATER:
            return "Water (How did you get here...)";
        case FOREST:
            return "Forest (Super Dangerous)";
        default:
            return "NULL";
    }
}



struct HostileType{
    std::shared_ptr<std::ifstream> file_stream;
    std::string name;
    std::string art;
    int health;
    std::vector<ATTACK> attacks;
    void draw(){
        std::cout << art << std::endl;
    }
    HostileType(std::string name, int health, std::vector<ATTACK> attacks): name(name), health(health), attacks(attacks) {
        file_stream = std::make_shared<std::ifstream>("assets/"+this->name);
        if(file_stream && file_stream->is_open()){
            std::string line;
            while(std::getline(*file_stream, line)){
                art += line + "\n";
            }
        }
    }
};

struct AttackType{
    std::string name;
    int damage;
    AttackType(std::string name, int damage): name(name), damage(damage) {}
};

int main(){
    std::cout << "\033[?25l";
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    const int win_width = w.ws_col;
    const int win_height = w.ws_row-1;
    const int max_fps = 60;
    const int frame_time_us = 1000000 / max_fps;
    const int map_rows = 1000;
    const int map_cols = 1000;
    const char player_char = '@';
    const int scale = 2;
    const int text_box_duration_frames = 90;
    bool able_to_move = true;
    bool in_battle = false;
    std::unique_ptr<HostileType> current_enemy;

    int player_x = 0;
    int player_y = 0;
    char player_tile = TAME_GRASS;

    std::vector<std::vector<char>> map(generate_map(map_cols, map_rows));

    map[player_y][player_x] = player_char;
    bool done = false;
    int status_rows = 6;
    int text_box_size = 15;
    int extra_message_rows = status_rows;
    std::stack<Text> text_boxes;
    text_boxes.push({"Status", "", {"Health: 100%", "Location: " + tile_type_to_string(player_tile)}});
    int frame_count_text_box = text_box_duration_frames;

    std::vector<AttackType> attacks = {AttackType("Punch", 10), AttackType("Ember", 15), AttackType("Wave", 10), AttackType("Quick Attack", 5)};
    std::vector<HostileType> hostiles = {HostileType("Wasp", 40, {ATTACK::PUNCH})};

    std::vector<ATTACK>player_attacks = {ATTACK::PUNCH, ATTACK::EMBER, ATTACK::WAVE, ATTACK::QUICK_ATTACK};

    //add_text_box({hostiles[0].name, hostiles[0].art}, 5, win_width);
    while(!done){
        auto start_time = std::chrono::high_resolution_clock::now();
        std::cout << "\033[H";

        if(!in_battle){
            print_map(map, player_x, player_y, (win_height-extra_message_rows)/scale,
                  win_width/scale, scale);
            if(text_boxes.size()==1){
                extra_message_rows = status_rows;
                text_boxes.pop();
                text_boxes.push({"Status", "", {"Health: 100%", "Location: " + tile_type_to_string(player_tile)}});
            }
            else{
                if(frame_count_text_box == 0){
                    text_boxes.pop();
                    frame_count_text_box = text_box_duration_frames;
                }
                else{
                    extra_message_rows = text_box_size;
                    frame_count_text_box -= 1;
                }
            }
            add_text_box(text_boxes.top(), extra_message_rows, win_width);
        }
        else{
            int attack_menu_height = 4+attacks.size();
            add_text_box({current_enemy->name, current_enemy->art}, 5, win_width);
            add_text_box({"Attack Menu", "", {"1. " + attacks[player_attacks[0]].name, "2. " + attacks[player_attacks[1]].name,
                "3. " + attacks[player_attacks[2]].name, "4. " + attacks[player_attacks[3]].name}}, attack_menu_height, win_width);

        }

        char input = get_character_input();
        bool moved = false;
        map[player_y][player_x] = player_tile;
        if (able_to_move && input != '\0') moved = true;
        if (able_to_move && input == 'a' && player_x > 0 && map[player_y][player_x - 1] != WATER) player_x--;
        else if (able_to_move && input == 'd' && player_x < map_cols - 1 && map[player_y][player_x + 1] != WATER) player_x++;
        else if (able_to_move && input == 'w' && player_y > 0 && map[player_y-1][player_x] != WATER) player_y--;
        else if (able_to_move && input == 's' && player_y < map_rows - 1 && map[player_y+1][player_x] != WATER) player_y++;
        else if (input == 'q') done = true;
        player_tile = map[player_y][player_x];
        map[player_y][player_x] = player_char;

        if (in_battle){
            switch(input){
                case '1':
                    break;
                case '2':
                    break;
                case '3':
                    break;
                case '4':
                    break;
                default:
                    break;
            }
        }

        if(moved && check_wild_grass_event(player_tile)){
            static std::random_device rd;
            static std::mt19937 gen(rd());
            static std::uniform_int_distribution<int> dist(0, hostiles.size()-1);

            current_enemy = std::make_unique<HostileType>(hostiles[dist(gen)]);

            text_boxes.push({"Battle!", "A wild " + current_enemy->name + " appeared! Prepare to fight!", {}});
            able_to_move = false;
        }
        if(!in_battle && !able_to_move && text_boxes.size() == 1){
            // Battle Sequence
            able_to_move = false;
            in_battle = true;
            system("clear");
            while(text_boxes.size() > 0){
                text_boxes.pop();
            }
        }
        auto end_time = std::chrono::high_resolution_clock::now();
        auto elapsed_us =
            std::chrono::duration_cast<std::chrono::microseconds>(end_time -
                                                                  start_time).count();
        if(elapsed_us < frame_time_us){
            usleep(frame_time_us - elapsed_us);
        }
    }
    reset_terminal();
    std::cout << "\033[?25h";
    return 0;
}
