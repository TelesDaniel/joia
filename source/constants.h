#define WSIZE 10 //number of block columns
#define HSIZE 10 //number of block lines
#define TILESIZE 15 //blocksize
#define RMARGIN ((SCREEN_WIDTH / 4)-TILESIZE) //right margin
#define TMARGIN 5 //top margin
#define PLAYER_OBJ_ID 0

#define COLORS_TILES (int[]) {0, 4, 8, 12, 16, 20, 40}//tile id in sheet.png of each block
#define SELECTED_TILES (int[]) {24, 28, 32, 36} //selected frames

typedef struct {
    int mem_pos; //posicao na memoria
    int x; //posicao no mapa
    int y;
    int color;
    int array_pos; //posicao atual no array
    int array_new_pos; //nova posicao no array
    int ypos; //posicao inicial
    int xpos;
    int status; //0 - ok; 1 - "deletado"
} TILE_BLOCK;

typedef struct {
    int *positions;
    int qtd;
    bool reverted;
} SEQUENCE;

struct PLAYER {
    int x;
    int y;
    unsigned int frame;
    bool in_select;
} player;
