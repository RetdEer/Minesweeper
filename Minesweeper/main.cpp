#include <SFML/Graphics.hpp>
#include <random>
#include <chrono>
#include <iostream>

enum class Outcome{
    ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN, EIGHT,  // number of adjacent mines
    MINE,
    FLAG,     // tile was flagged
    DEFAULT
};

struct Tile{
    sf::Text t;             // readable text
    bool clickable = true;   // can the tile be clicked
    Outcome oa = Outcome::DEFAULT, ob = Outcome::FLAG;
};

constexpr unsigned int rows = 10;     // useful globals
constexpr unsigned int cols = 10;
int total_mines = 0;
Tile board[rows][cols];

bool gameLost = false;
bool gameWon = false;

void InitMinesweeper(sf::Vector2u rwindow_size, const sf::Font & font);                               // function declarations
void ProcLeftMouseClick(sf::Vector2i local_cursor_position);     // uncover tile
void ProcRightMouseClick(sf::Vector2i local_cursor_position);   // flag tile
void ApplyFirstClickHandicap(sf::Vector2i local_cursor_position, int radius = 3);   // max number of tiles solved = (2 * radius + 1)^2

bool firstClickHandicapAvailable = true;
sf::Vector2u tile_size{};                               // tile dimensions

int main(){
    sf::RenderWindow rw(sf::VideoMode(800, 800), "Minesweeper");
    sf::Font f;

    if(!f.loadFromFile("IBMPlexSans-Medium.ttf"))
        return -1;

    InitMinesweeper(rw.getSize(), f);
    sf::Clock c; sf::Event e;

    while(rw.isOpen() && (!gameLost && !gameWon)){
        while(rw.pollEvent(e)){
            if(e.type == sf::Event::Closed){
                rw.close();
            }
            if(e.type == sf::Event::MouseButtonPressed){                // checks if a mouse button is pressed
                if(e.mouseButton.button == sf::Mouse::Left){
                    firstClickHandicapAvailable ? ApplyFirstClickHandicap(sf::Mouse::getPosition(rw)) :
                                                                 ProcLeftMouseClick(sf::Mouse::getPosition(rw));
                }
                else if(e.mouseButton.button == sf::Mouse::Right){
                    ProcRightMouseClick(sf::Mouse::getPosition(rw));
                }
            }
        }

        rw.clear(sf::Color(192, 192, 192));                 // silver
        for(unsigned int i = 0; i < rows; ++i)
            for(unsigned int j = 0; j < cols; ++j)
                rw.draw(board[i][j].t);

        rw.display();
        gameWon = (total_mines == 0) ? true : false;        // we can only win if there are zero outstanding mines
    }
    if(gameWon) std::cout << "You won! It took you " << (int)c.getElapsedTime().asSeconds() << " seconds to disarm all the mines.\n";
    if(gameLost) std::cout << "You blew yourself up in " << (int)c.getElapsedTime().asSeconds() << " seconds\n";
    return 0;
}

void InitMinesweeper(sf::Vector2u rwindow_size, const sf::Font & font){
    using namespace std::chrono;
    unsigned seed = system_clock::now().time_since_epoch().count();     // returns ticks in an unsigned type
    std::mt19937_64 genr(seed);
    std::uniform_int_distribution distrb (1, 100);   // [1, 100]
    tile_size = {rwindow_size.x / cols, rwindow_size.y / rows};     // board[row number][column number] = {tile width, tile height};

    for(unsigned int i = 0; i < rows; ++i){
        for(unsigned int j = 0; j < cols; ++j){
            board[i][j].t.setFont(font);
            board[i][j].t.setPosition(sf::Vector2f(j * tile_size.x, i * tile_size.y));
            board[i][j].t.setCharacterSize(30);

            const int n = distrb(genr);
            if(n <= 20){                                           // mine spawns with 20% chance in any given tile
                board[i][j].oa = Outcome::MINE;
                ++total_mines;
            }

            board[i][j].t.setString(" ?");                    // All tiles are initialized to green question marks
            board[i][j].t.setFillColor(sf::Color::Green);
        }
    }

    for(int i = 0; i < (int)rows; ++i){         // unsigned int -> int is a safe cast, so no data is lost
        for(int j = 0; j < (int)cols; ++j){
            int adjacent_mines = 0;
            if(board[i][j].oa == Outcome::MINE) continue;
            else{              // check all adjacent spaces to count number of mines around it

                if(i - 1 >= 0 && j - 1 >= 0){adjacent_mines += (board[i - 1][j - 1].oa == Outcome::MINE ? 1 : 0);}
                if(i - 1 >= 0){adjacent_mines += (board[i - 1][j].oa == Outcome::MINE ? 1 : 0);}
                if(j - 1 >= 0){adjacent_mines += (board[i][j - 1].oa == Outcome::MINE ? 1 : 0);}
                if(i - 1 >= 0 && j + 1 < (int)cols){adjacent_mines += (board[i - 1][j + 1].oa == Outcome::MINE ? 1 : 0);}
                if(i + 1 < (int)rows && j - 1 >= 0){adjacent_mines += (board[i + 1][j - 1].oa == Outcome::MINE ? 1 : 0);}
                if(j + 1 < (int)cols){adjacent_mines += (board[i][j + 1].oa == Outcome::MINE ? 1 : 0);}
                if(i + 1 < (int)rows){adjacent_mines += (board[i + 1][j].oa == Outcome::MINE ? 1 : 0);}
                if(i + 1 < (int)rows && j + 1 < (int)cols){adjacent_mines += (board[i + 1][j + 1].oa == Outcome::MINE ? 1 : 0);}

                board[i][j].oa = (Outcome)(adjacent_mines);      // works because of enum class declaration order, ZERO -> 0, ONE -> 1, ... EIGHT -> 8
            }
        }
    }
}

void ProcLeftMouseClick(sf::Vector2i local_cursor_position){
    sf::Vector2i index(local_cursor_position.y / tile_size.y, local_cursor_position.x / tile_size.x);   // gets the coordinates of a mouse click relative to the window

    if(board[index.x][index.y].clickable && board[index.x][index.y].oa != Outcome::FLAG){
        board[index.x][index.y].clickable = false;

        if(board[index.x][index.y].oa != Outcome::MINE){         // Wheew!
            board[index.x][index.y].t.setString(" " +  std::to_string((int)board[index.x][index.y].oa));
            board[index.x][index.y].t.setFillColor(sf::Color::Blue);
        }
        else{                                                                               // Game over
            board[index.x][index.y].t.setString(" !");
            board[index.x][index.y].t.setFillColor(sf::Color::Red);
            gameLost = true;
        }
    }
}

void ProcRightMouseClick(sf::Vector2i local_cursor_position){
    sf::Vector2i index(local_cursor_position.y / tile_size.y, local_cursor_position.x / tile_size.x);

    if(board[index.x][index.y].clickable){         // We may only flag tiles that haven't been clicked yet
        Outcome temp = board[index.x][index.y].oa;                          // swap oa and ob
        board[index.x][index.y].oa = board[index.x][index.y].ob;
        board[index.x][index.y].ob = temp;

        if(board[index.x][index.y].oa == Outcome::FLAG){                // If the main outcome is now a flag, update the tile to a magenta "F"
            board[index.x][index.y].t.setString(" F");
            board[index.x][index.y].t.setFillColor(sf::Color::Magenta);
            total_mines -= (board[index.x][index.y].ob == Outcome::MINE ? 1 : 0);       // Check ob to see if the tile we flagged was indeed a mine
        }
        else{                                                                                      // Only unclicked tiles reach this line, so we just restore the green question mark
            board[index.x][index.y].t.setString(" ?");
            board[index.x][index.y].t.setFillColor(sf::Color::Green);
            total_mines += (board[index.x][index.y].oa == Outcome::MINE ? 1 : 0);       // If we unflag a mine, add it back to the number of outstanding mines
        }
    }
}

void ApplyFirstClickHandicap(sf::Vector2i local_cursor_position, int radius){
    firstClickHandicapAvailable = false;
    sf::Vector2i offset(local_cursor_position.y / tile_size.y, local_cursor_position.x / tile_size.x);

    for(int i = -radius; i <= radius; ++i){     // the tile on which the player first clicks is defined as the center, from there the board auto solves in all directions by "radius" tiles
        int row = std::clamp(i + offset.x, 0, (int)rows - 1);
        for(int j = -radius; j <= radius; ++j){
            int col = std::clamp(j + offset.y, 0, (int)cols - 1);
            if(board[row][col].oa == Outcome::MINE){
                Outcome temp = board[row][col].oa;                      // swap oa and ob
                board[row][col].oa = board[row][col].ob;
                board[row][col].ob = temp;

                board[row][col].t.setString(" F");
                board[row][col].t.setFillColor(sf::Color::Magenta);
                --total_mines;                                                          // no need to check ob aagain
            }
            else if(board[row][col].oa != Outcome::FLAG){         // fills out any non-mine, non-flag tiles
                board[row][col].clickable = false;
                board[row][col].t.setString(" " +  std::to_string((int)board[row][col].oa));
                board[row][col].t.setFillColor(sf::Color::Blue);
            }
        }
    }
}
