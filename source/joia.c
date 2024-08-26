
#include <tonc.h>
#include <sheet.h> //sprites
#include <start.h> //start screen
#include <b.h> //background
#include <string.h>
#include <stdlib.h>
#include "constants.h" //game constants

bool update_tiles = true; //if true, need update the tiles
bool need_revert = false; //if true, need revert player action

OBJ_ATTR obj_buffer[128];

void plot_tile(TILE_BLOCK bk){
    OBJ_ATTR *obj = &obj_buffer[bk.mem_pos];
    obj_set_attr(
        obj,
        ATTR0_SQUARE,
        ATTR1_SIZE_16,
        ATTR2_PALBANK(0) | COLORS_TILES[bk.color]
    );
    obj_set_pos(obj, (bk.x*TILESIZE) + RMARGIN, (bk.y*TILESIZE)+TMARGIN);
}

void plot_player(struct PLAYER p){
    OBJ_ATTR *obj = &obj_buffer[PLAYER_OBJ_ID];

    obj_set_attr(
        obj,
        ATTR0_SQUARE,
        ATTR1_SIZE_16,
        ATTR2_PALBANK(0) | SELECTED_TILES[p.frame] | ATTR2_PRIO(3)
    );
    obj_set_pos(obj, (p.x*TILESIZE) + RMARGIN, (p.y*TILESIZE)+TMARGIN);
}

void anim_player(struct PLAYER p) {
    OBJ_ATTR *obj = &obj_buffer[PLAYER_OBJ_ID];
    obj->attr2 = ATTR2_BUILD(SELECTED_TILES[p.in_select ? 0 : p.frame],0, 0);
}

TILE_BLOCK new_block(int x, int y, int mem_pos, int array_pos){
    TILE_BLOCK block;
    block.x = x;
    block.y = y;
    block.color = rand() % 5;
    block.ypos = -20;
    block.xpos = x*TILESIZE;
    block.mem_pos = mem_pos;
    block.array_pos = array_pos;
    block.array_new_pos = array_pos;
    block.status = 0;
    return block;
}

SEQUENCE has_sequence(TILE_BLOCK blocks[], int target_x, int target_y) {
    
    int qtdx = 0;
    int qtdy = 0;
    int last_color=-1;
    int *sequencex= malloc(sizeof(int));
    int *sequencey= malloc(sizeof(int));

    for(int x=0; x<WSIZE; x++){
        int pos = x+target_y*HSIZE;
        if(qtdx==0){
            last_color = blocks[pos].color;
            sequencex[qtdx] = pos;
            qtdx++;
        }else if(last_color == blocks[pos].color){
            sequencex = realloc(sequencex, (qtdx+1)*sizeof(int));
            sequencex[qtdx] = pos;
            qtdx++;
        }else if (qtdx < 3){
            qtdx = 1;
            last_color = blocks[pos].color;
            sequencex = realloc(sequencex, sizeof(int));
            sequencex[0] = pos;
        }else
            break;
    }

    qtdx = qtdx < 3 ? 0 : qtdx;

    for(int y=0; y<HSIZE; y++){
        int pos = target_x+y*HSIZE;
        if(qtdy==0){
            last_color = blocks[pos].color;
            sequencey[qtdy] = pos;
            qtdy++;
        }else if(last_color == blocks[pos].color){
            sequencey = realloc(sequencey, (qtdy+1)*sizeof(int));
            sequencey[qtdy] = pos;
            qtdy++;
        }else if (qtdy < 3){
            qtdy = 1;
            last_color = blocks[pos].color;
            sequencey = realloc(sequencey, sizeof(int));
            sequencey[0] = pos;
        }else
            break;
    }

    qtdy = qtdy < 3 ? 0 : qtdy;

    int *total= malloc((qtdx+qtdy)*sizeof(int));
    if(qtdx>0)
        memcpy(total, sequencex, qtdx * sizeof(int));
    if(qtdy>0)
        memcpy(total+qtdx, sequencey, qtdy * sizeof(int));

    free(sequencex);
    free(sequencey);

    SEQUENCE seq ={ .positions=total, .qtd=qtdx+qtdy, .reverted=false};
    return seq;
}

void changing_block_pos(TILE_BLOCK blocks[], int target_pos, int selected_pos) {
    
    TILE_BLOCK selected = blocks[selected_pos];
    TILE_BLOCK target = blocks[target_pos];

    int xs = selected.x;
    int ys = selected.y;

    selected.y = target.y;
    selected.x = target.x;
    selected.array_new_pos = target.array_pos;

    target.x =xs;
    target.y =ys;
    target.array_new_pos = selected.array_pos;

    player.in_select = false;
    update_tiles=true;

    blocks[selected_pos] = target;
    blocks[target_pos] = selected;
}


void effetive_block_pos(TILE_BLOCK *blocks) {

    TILE_BLOCK *aux = malloc(HSIZE*WSIZE*sizeof(TILE_BLOCK));
    memcpy(aux, blocks, HSIZE*WSIZE*sizeof(TILE_BLOCK));

    for (int y = 0; y < HSIZE; y++){
            for (int x = 0; x < WSIZE; x++){
                int pos = x + y * HSIZE;
                TILE_BLOCK tile = blocks[pos];

                if(tile.array_new_pos != tile.array_pos){
                    tile.array_pos = tile.array_new_pos;
                    aux[tile.array_pos] = tile;
                }
        }
    }

    memcpy(blocks, aux, HSIZE*WSIZE*sizeof(TILE_BLOCK));
    free(aux);
}

bool verify_allmap(TILE_BLOCK blocks[]){

    bool has_changes = false;
    for(int x=0; x<WSIZE; x++) {
        for (int y = 0; y < HSIZE; y++){
            SEQUENCE seq = has_sequence(blocks, x, y);
            if(seq.qtd > 2){
                for(int i=0; i< seq.qtd; i++){
                    blocks[seq.positions[i]].status = 1;
                }
                has_changes=true;
            }
            free(seq.positions);
        }
    }

    if(!has_changes) return has_changes;

    for(int x=0; x<WSIZE; x++) {
        for (int y = HSIZE-1; y >= 0; y--){
            int pos = x + y * HSIZE;
            if(blocks[pos].status == 1){
                if (y > 0){
                    for (int ya = y-1; ya >= 0; ya--){
                        int pos_aux = x + ya * HSIZE;
                        blocks[pos_aux].y += 1;
                        blocks[pos_aux].array_pos = pos_aux; //por algum motivo isso faz funciona TODO - entender
                        blocks[pos_aux].array_new_pos = x + (blocks[pos_aux].y * HSIZE);
                    }
                }

                int auxy = blocks[pos].y-y;
                blocks[pos] = new_block(x, blocks[pos].y-y, blocks[pos].mem_pos, pos);
                blocks[pos].array_new_pos = x + (auxy * HSIZE);
                
                OBJ_ATTR *obj = &obj_buffer[blocks[pos].mem_pos];
                obj->attr2 = ATTR2_BUILD(COLORS_TILES[blocks[pos].color] ,0, 0);
                obj_set_pos(obj, x+RMARGIN, blocks[pos].ypos);
            }
        }
    }

    return has_changes;
}

void anim_block(TILE_BLOCK *blocks, SEQUENCE sequence){

    int counter = 0; //locomove os blocos com base nos valores auxilizares
    for (int y = 0; y < HSIZE; y++){
        for (int x = 0; x < WSIZE; x++){

            int pos = x + y * HSIZE;
            int posy = blocks[pos].y * TILESIZE;
            int posx = blocks[pos].x * TILESIZE;

            if(posy > blocks[pos].ypos)
                blocks[pos].ypos++;
            else if(posy < blocks[pos].ypos)
                blocks[pos].ypos--;
            else if(posx < blocks[pos].xpos)
                blocks[pos].xpos--;
            else if(posx > blocks[pos].xpos)
                blocks[pos].xpos++;
            else
                counter++;
            
            obj_set_pos(&obj_buffer[blocks[pos].mem_pos],  blocks[pos].xpos + RMARGIN,  blocks[pos].ypos + TMARGIN);
        }
    }

    if(counter == HSIZE*WSIZE){ //se verificou mov de todos e nenhum tem, finaliza os updates
        effetive_block_pos(blocks);

        if(!sequence.reverted && sequence.qtd < 2){
            update_tiles = false;
            need_revert = true;
            return;
        } 
        
        if(verify_allmap(blocks)){ //verifica o mapa
            update_tiles = true;
            return;
        }

        update_tiles = false;
    }
}

void init_map(TILE_BLOCK blocks[]) {
    for(int y=0; y <HSIZE; y++){
        for(int x=0; x<WSIZE; x++){
            int pos = x+y*HSIZE;
            blocks[pos] = new_block(x, y, x+y*HSIZE+1, pos);
            plot_tile(blocks[pos]);
        }
    }
}  

void init_player() {
    player.x = 0;
    player.y = 0;
    player.frame = 0;
    player.in_select = false;
    plot_player(player);
}

void game_screen(int frame){ 
    //init tiles
    memcpy(&tile_mem[4][0], sheetTiles, sheetTilesLen);
    memcpy(pal_obj_bank, sheetPal, sheetPalLen);
    memcpy(&tile_mem[0], bTiles, bTilesLen);
    memcpy(&se_mem[30][0], bMap, bMapLen);
	memcpy(pal_bg_mem, bPal, bPalLen);

    //vsync enable
    irq_init(NULL);
    irq_add(II_VBLANK, NULL);

    //tiles buffer
    oam_init(obj_buffer, 128); 
    TILE_BLOCK *blocks = malloc(HSIZE*WSIZE*sizeof(TILE_BLOCK)); //aloca memoria para o array do mapa
    SEQUENCE sequence={.qtd=0, .reverted=false};

    init_map(blocks); //inicia array do mapa
    init_player(); //inicia selecao

    oam_copy(oam_mem, obj_buffer, 128); //copia object attr memory para um buffer ( nao pode mexer direto nela)

    int selected_pos=-1;
    int target_pos=-1;
    update_tiles=true;

    while(1){
        VBlankIntrWait(); //vsync
        key_poll(); //limpa botoes

        if(key_hit(KEY_A) || key_hit(KEY_B)) //block selection
            player.in_select = !player.in_select;

        if(key_hit(KEY_SELECT)){ //close game
            free(blocks); 
            oam_copy(oam_mem, NULL, 128);
            break;
        }

        if(update_tiles) // update blocks map
            anim_block(blocks, sequence);
        else if(need_revert){ //revert player mov
            changing_block_pos(blocks, selected_pos, target_pos);
            selected_pos=-1;
            target_pos=-1;
            need_revert = false;
            sequence.reverted = true;
        }else {
            //player mov around block map
            if(!player.in_select) {
                if( key_hit(KEY_RIGHT) )
                    player.x++;
                else if( key_hit(KEY_LEFT) )
                    player.x--;
                if( key_hit(KEY_DOWN) )
                    player.y++;
                else if( key_hit(KEY_UP) )
                    player.y--;

                if(player.x < 0) //limits
                    player.x = WSIZE-1;
                else if(player.x >= WSIZE)
                    player.x = 0;

                if(player.y < 0)
                    player.y = HSIZE-1;
                else if(player.y >= HSIZE)
                    player.y = 0;

                player.frame = (((frame * 42) >> 9) % 4);
                plot_player(player); //movimento do player
            }else{
                int tposx = 0;
                int tposy = 0;
                
                if(key_hit(KEY_RIGHT)) //player selection
                    tposx += 1;
                else if( key_hit(KEY_LEFT))
                    tposx += -1;
                else if( key_hit(KEY_DOWN) )
                    tposy += 1;
                else if( key_hit(KEY_UP) )
                    tposy += -1;

                if(player.x+tposx < 0)
                    tposx=0;
                else if(player.x+tposx >= WSIZE)
                    tposx=0;

                if(player.y+tposy < 0)
                    tposy=0;
                else if(player.y+tposy >= HSIZE)
                    tposy=0;

                player.frame=0;
                if(tposx != 0 || tposy != 0) { 
                    selected_pos = player.x+player.y*HSIZE; //player positon
                    target_pos = (player.x+tposx)+(player.y+tposy)*HSIZE; //target position
                    changing_block_pos(blocks, selected_pos, target_pos); //change positions
                    sequence = has_sequence(blocks, player.x+tposx, player.y+tposy); //verify if has sequence
                    if(sequence.qtd<2)
                        sequence = has_sequence(blocks, player.x, player.y);
                }
            }
            anim_player(player);
        }

        frame++;
        srand(frame);
        oam_copy(oam_mem, obj_buffer, 128); //atualiza o oam
    }
}

void start_screen(int frame) {
    memcpy(&tile_mem[0], startTiles, startTilesLen); //load start screen
    memcpy(&se_mem[30][0], startMap, bMapLen);
	memcpy(pal_bg_mem, startPal, bPalLen);

    irq_init(NULL);
    irq_add(II_VBLANK, NULL);

    while(1){
        key_poll(); 

        if(key_hit(KEY_START))
            break;
        frame++;
        srand(frame);
    }

    game_screen(frame);
}

int main() {
    //init io
    REG_DISPCNT= DCNT_MODE0 | DCNT_OBJ | DCNT_OBJ_1D | DCNT_BG0 ;
    //init background
    REG_BG0CNT= BG_CBB(0) | BG_SBB(30) | BG_REG_64x64;

    start_screen(0);
    return 0;
}