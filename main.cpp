#include <iostream>
#include <string>
#include <vector>
#include <termios.h>
#include <unistd.h>
#include <random>
#include <algorithm>
#include <sys/ioctl.h>
#include <stack>
#include <chrono>

enum TILE_TYPE{
    TAME_GRASS='.',
    WILD_GRASS=',',
    FOREST='^',
    BUSH=';',
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

    std::cout << "\033[H";
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

char get_character_input() {
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

    ioctl(STDIN_FILENO, FIONREAD, &bytes_waiting);
    char c = '\0';
    if(bytes_waiting > 0){
        read(STDIN_FILENO, &c, 1);
    }
    return c;
}

void reset_terminal() {
    static struct termios oldt;
    tcgetattr(STDIN_FILENO, &oldt);
    oldt.c_lflag |= (ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
}

int main(){
    std::cout << "\033[?25l";
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    const int max_fps = 30;
    const int frame_time_us = 1000000 / max_fps;
    const int map_rows = 1000;
    const int map_cols = 1000;
    const char player_char = '@';
    const int scale = 2;
    const int text_box_duration_frames = 90;

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
    text_boxes.push({"Status", "", {"Health: 100%", "Location: (" +
        std::to_string(player_x) + "," + std::to_string(player_y) + ")"}});
    int frame_count_text_box = text_box_duration_frames;
    while(!done){
        auto start_time = std::chrono::high_resolution_clock::now();

        print_map(map, player_x, player_y, (w.ws_row-1-extra_message_rows)/scale,
                  w.ws_col/scale, scale);

        if(text_boxes.size()==1){
            extra_message_rows = status_rows;
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

        add_text_box(text_boxes.top(), extra_message_rows, w.ws_col);
        char input = get_character_input();
        bool moved = false;
        map[player_y][player_x] = player_tile;
        if (input != '\0') moved = true;
        if (input == 'a' && player_x > 0 && map[player_y][player_x - 1] != WATER) player_x--;
        else if (input == 'd' && player_x < map_cols - 1 && map[player_y][player_x + 1] != WATER) player_x++;
        else if (input == 'w' && player_y > 0 && map[player_y-1][player_x] != WATER) player_y--;
        else if (input == 's' && player_y < map_rows - 1 && map[player_y+1][player_x] != WATER) player_y++;
        else if (input == 'q') done = true;
        player_tile = map[player_y][player_x];
        map[player_y][player_x] = player_char;

        if(moved && check_wild_grass_event(player_tile)){
            //BATTLE SEQUENCE!!
            text_boxes.push({"Battle!", "A wild NULL appeared! Prepare to fight!", {}});
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
