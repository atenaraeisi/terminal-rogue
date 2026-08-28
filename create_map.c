#include <ncurses.h>
#include <locale.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "create_map.h"
#include "game.h"
#include "menu.h"

bool is_valid_position(char **map, int x, int y) {
    return map[y][x] == '.';
}
Monster createMonster(int type) {
    Monster m;
    switch (type) {
        case 0:
            m = (Monster){'D', "Demon", 5, 2, 0, 0, 3, 0, true};
            break;
        case 1:
            m = (Monster){'F', "Fire Breathing Monster", 10, 4, 0, 0, 3, 0, true};
            break;
        case 2:
            m = (Monster){'G', "Giant", 15, 7, 0, 0, 2, 0, true};
            break;
        case 3:
            m = (Monster){'S', "Snake", 20, 1, 0, 0, 1, 0, true};
            break;
        case 4:
            m = (Monster){'U', "Undead", 30, 5, 0, 0, 4, 0, true};
            break;
        default:
            m = (Monster){'?', "Unknown", 1, 1, 0, 0, 3, 0, false}; // Unknown monsters are inactive
            break;
    }
    return m;
}


void connect_rooms(char **map, int x1, int y1, int x2, int y2) {
    int x = x1;
    int y = y1;
    while (x != x2) {
        if((map[y][x] == '_' || map[y][x] == '|') && (map[y][x+1] == '_' || map[y][x-1] == '_' || map[y][x+1] == '|' || map[y][x-1] == '|')){
            map[y][x] = '+'; // در
            break;
        }
        if (map[y][x] == '|' || map[y][x] == '_') {
            map[y][x] = '+'; // در
        } else if (map[y][x] == ' ') {
            map[y][x] = '#'; // راهرو
        }
        x += (x < x2) ? 1 : -1; // حرکت به سمت x2
    }

    // حرکت در جهت عمودی
    while (y != y2) {
        if((map[y][x] == '_' || map[y][x] == '|') && (map[y+1][x] == '_' || map[y-1][x] == '_' || map[y+1][x] == '|' || map[y-1][x] == '|')){
            map[y][x] = '+'; // در
            break;
        }
        if (map[y][x] == '_' || map[y][x] == '|') {
            map[y][x] = '+'; // در
        } else if (map[y][x] == ' ') {
            map[y][x] = '#'; // راهرو
        }
        y += (y < y2) ? 1 : -1; // حرکت به سمت y2
    }
    while (x != x2) {
        if((map[y][x] == '_' || map[y][x] == '|') && (map[y][x+1] == '_' || map[y][x-1] == '_' || map[y][x+1] == '|' || map[y][x-1] == '|')){
            break;
        }
        if (map[y][x] == '|' || map[y][x] == '_') {
            map[y][x] = '+'; // در
        } else if (map[y][x] == ' ') {
            map[y][x] = '#'; // راهرو
        }
        x += (x < x2) ? 1 : -1; // حرکت به سمت x2
    }



}

char **create_map(int width, int height, int level_difficulty, Player* player, Room *rooms, int num_rooms, Food foods[7], int num_monster, Monster monsters[]) {
    // تخصیص حافظه برای نقشه
    char **map = (char **)malloc(height * sizeof(char *));
    for (int i = 0; i < height; i++) {
        map[i] = (char *)malloc(width * sizeof(char));
        memset(map[i], ' ', width); // پر کردن نقشه با ' '
    }

    int preserved_room_index = -1;
    Room preserved_room;
    if(player->is_in_floor > 1){
        // شناسایی اتاق بازیکن در نقشه قبلی
        for (int i = 0; i < num_rooms; i++) {
            if (player->x >= rooms[i].start_x && player->x < rooms[i].start_x + rooms[i].width &&
                player->y >= rooms[i].start_y && player->y < rooms[i].start_y + rooms[i].height) {
                preserved_room_index = i;
                break;
            }
        }

        // بازسازی اتاق بازیکن در نقشه جدید
        if(preserved_room_index != -1){
            preserved_room = rooms[preserved_room_index];
            for (int y = preserved_room.start_y; y < preserved_room.start_y + preserved_room.height; y++) {
                for (int x = preserved_room.start_x; x < preserved_room.start_x + preserved_room.width; x++) {
                    if ((x == preserved_room.start_x || x == preserved_room.start_x + preserved_room.width - 1) && 
                        (y != preserved_room.start_y && y != preserved_room.start_y + preserved_room.height - 1)) {
                        map[y][x] = '|'; // دیوارهای عمودی
                    } else if (y == preserved_room.start_y || y == preserved_room.start_y + preserved_room.height - 1) {
                        map[y][x] = '_'; // دیوارهای افقی
                    } else {
                        map[y][x] = '.'; // کف اتاق
                    }
                }
            }            
        }
    }


    int r = 0;
    for (r = 0; r < num_rooms; r++) {
        if (preserved_room_index != -1 && r == preserved_room_index) {
            // این اتاق قبلاً بازسازی شده است
            rooms[r] = preserved_room;
            continue;
        }
        Room room;
        room.width = (width / 10) + rand() % (width / 6 - width / 10 + 1);
        room.height = (height / 6) + rand() % (height / 6 - height / 10 + 1) + 1;
        room.start_x = rand() % (width - room.width); // موقعیت X شروع اتاق
        room.start_y = rand() % (height - room.height); // موقعیت Y شروع اتاق
        room.visited = 0;
        room.floor = 1;

        // بررسی تداخل
        bool overlap = false;
        for (int y = room.start_y; y < room.start_y + room.height && !overlap; y++) {
            for (int x = room.start_x; x < room.start_x + room.width; x++) {
                if (map[y][x] == '.' || map[y][x] == '|' || map[y][x] == '_') {
                    overlap = true;
                    break;
                }
            }
        }
        // اگر تداخل وجود ندارد، اتاق را رسم کنید
        if (!overlap) {
            for (int y = room.start_y; y < room.start_y + room.height; y++) {
                for (int x = room.start_x; x < room.start_x + room.width; x++) {
                    if((x == room.start_x && y!=room.start_y )|| (x == room.start_x + room.width-1  && y!=room.start_y)){
                        map[y][x] = '|';
                    }
                    else if(y == room.start_y|| y == room.start_y + room.height-1){
                        map[y][x] = '_';
                    }
                    else map[y][x] = '.'; // پر کردن خانه‌های اتاق
                }
            }
            rooms[r] = room; // ذخیره اطلاعات اتاق در آرایه
        }
        else{
            r--;
            continue;
        }
        
    }
    //تم بندی اتاق ها
    // تم بندی اتاق‌ها
    int num_regular_rooms = num_rooms / 2; 
    int num_treasure_rooms = 0, num_enchant_rooms = 1, num_nightmare_rooms = 1;

    if (player->is_in_floor == 4) {
        num_treasure_rooms = 1;  // اضافه کردن اتاق گنج در طبقه 4
    }

    // تکمیل تعداد اتاق‌ها با اتاق‌های جادویی
    while (num_treasure_rooms + num_enchant_rooms + num_nightmare_rooms + num_regular_rooms < num_rooms) {
        num_enchant_rooms++;
    }

    // تخصیص اتاق‌های معمولی
    for (int i = 0; i < num_regular_rooms; i++) {
        rooms[i].theme = 'r';  // regular rooms
    }

    // تخصیص اتاق‌های جادویی
    for (int i = num_regular_rooms; i < num_regular_rooms + num_enchant_rooms; i++) {
        rooms[i].theme = 'e';  // enchant rooms
    }

    // تخصیص اتاق‌های کابوس
    for (int i = num_regular_rooms + num_enchant_rooms; 
            i < num_regular_rooms + num_enchant_rooms + num_nightmare_rooms; 
            i++) {
        rooms[i].theme = 'n';  // nightmare rooms
    }

    // اگر اتاق گنج وجود دارد، آن را به آخرین عضو آرایه اختصاص بده
    if (num_treasure_rooms == 1) {
        rooms[num_rooms - 1].theme = 't';  // treasure room در آخرین اتاق
    }

    //وصل کردن اتاق ها با راهرو
    int prev_center_x = -1, prev_center_y = -1;
    for(int i = 0; i < num_rooms ; i++){
        if(rooms[i].theme == 't'){
            break;
        }
        //rooms[i]
        //مختصات مرکز اتاق
        int center_x = rooms[i].start_x + rooms[i].width / 2;
        int center_y = rooms[i].start_y + rooms[i].height / 2;

        // اگر اتاق قبلی وجود دارد، راهرو بین اتاق حاضر و قبلی رسم کن
        if (prev_center_x != -1 && prev_center_y != -1) {
            connect_rooms(map, prev_center_x, prev_center_y, center_x, center_y);
        }

        for (int j = 0; j < i; j++) { // اتصال اتاق جدید به اتاق‌های قبلی
            if (rand() % 6 == 0) { // به صورت تصادفی یکی از اتاق‌های قبلی را انتخاب کنید
                int prev_center_x2 = rooms[j].start_x + rooms[j].width / 2;
                int prev_center_y2 = rooms[j].start_y + rooms[j].height / 2;
                //if(abs(prev_center_x2-center_x) < width/3 && abs(prev_center_y2-center_y) < width/3)
                connect_rooms(map, prev_center_x2, prev_center_y2, center_x, center_y);
            }
        }
        // به‌روزرسانی مرکز اتاق قبلی
        prev_center_x = center_x;
        prev_center_y = center_y;
    }
    rooms[0].visited = 1;
    //گذاشتن چیز میزای دیگه تو اتاقا
    int stair_room = rand() % (num_rooms-1) + 1;
    while(rooms[stair_room].theme == 'n'){
        stair_room = rand() % (num_rooms-1) + 1;
    }
    for(int i = 0; i < num_rooms ; i++){
        //پنجره داخل هر اتاق به طور تصادفی
        int num_window = rand() % 2;
        for(int j = 0 ; j < num_window ; j++){
            int win_x = rooms[i].start_x;
            int win_y = rooms[i].start_y + rand() % rooms[i].height;
            while(map[win_y][win_x] != '|'){
                win_y = rooms[i].start_y + rand() % rooms[i].height;
            }
            map[win_y][win_x] = '=';
        }
        //موانع داخل هر اتاق به طور تصادفی
        int num_obstacle = rooms[i].height/3 + rand() % 2;
        for(int j = 0 ; j < num_obstacle ; j++){
            int obs_x = rooms[i].start_x + rand() % (rooms[i].width);
            int obs_y = rooms[i].start_y + rand() % (rooms[i].height);
            while(map[obs_y][obs_x] != '.' 
                || (map[obs_y+1][obs_x] == '+') 
                || (map[obs_y-1][obs_x] == '+') 
                || (map[obs_y][obs_x+1] == '+') 
                || (map[obs_y][obs_x-1] == '+')){
                obs_x = rooms[i].start_x + rand() % (rooms[i].width);
                obs_y = rooms[i].start_y + rand() % (rooms[i].height);
            }
            map[obs_y][obs_x] = 'o';
        }
        int num_traps, num_talisman, num_gold, num_black_gold, num_weapon;
        if(rooms[i].theme == 'r'){
            num_gold = 2;
            num_black_gold = rand() % 2;
            num_talisman = rand() % 2;
            num_traps = rand() % 2;
            num_weapon = rand() % 3;
        } else if(rooms[i].theme == 't'){
            num_gold = 4;      
            num_black_gold = 2;
            num_talisman = 0;             
            num_traps = rooms[i].width/3 + rand() % 3;      
            num_weapon = 0;     
        }  else if(rooms[i].theme == 'n'){
            int num_gold_halluc = rand() % 4; 
            num_gold = 0;
            num_talisman= 0;
            num_traps = 0;
            num_black_gold = 0;
            num_weapon = 0;   
            //گذاشتن طلا در اتاق
            for(int j = 0 ; j < num_gold_halluc ; j++){
                int gold_x = rooms[i].start_x + rand() % (rooms[i].width);
                int gold_y = rooms[i].start_y + rand() % (rooms[i].height);
                while(map[gold_y][gold_x] != '.'){
                    gold_x = rooms[i].start_x + rand() % (rooms[i].width);
                    gold_y = rooms[i].start_y + rand() % (rooms[i].height);
                }
                map[gold_y][gold_x] = 'h';
            }
        } else if(rooms[i].theme == 'e'){
            num_black_gold = 0;
            num_gold =  rand() % 2;
            num_talisman = rand() % 3 + 2;
            num_traps =  rand() % 2;
            num_weapon = rand() % 2;
            //مخفی کردن در های اتاق
            for (int y = rooms[i].start_y; y < rooms[i].start_y + rooms[i].height; y++) {
                for (int x = rooms[i].start_x; x < rooms[i].start_x + rooms[i].width; x++) {
                    if (map[y][x] == '+') {
                        map[y][x] = 'v';
                    }
                }
            }
        }
        //گذاشتن طلا در اتاق
        for(int j = 0 ; j < num_gold ; j++){
            int gold_x = rooms[i].start_x + rand() % (rooms[i].width);
            int gold_y = rooms[i].start_y + rand() % (rooms[i].height);
            while(map[gold_y][gold_x] != '.'){
                gold_x = rooms[i].start_x + rand() % (rooms[i].width);
                gold_y = rooms[i].start_y + rand() % (rooms[i].height);
            }
            map[gold_y][gold_x] = 'g';
        }
        //گذاشتن طلای سیاه در اتاق
        for(int j = 0 ; j < num_black_gold ; j++){
            int gold_x = rooms[i].start_x + rand() % (rooms[i].width);
            int gold_y = rooms[i].start_y + rand() % (rooms[i].height);
            while(map[gold_y][gold_x] != '.'){
                gold_x = rooms[i].start_x + rand() % (rooms[i].width);
                gold_y = rooms[i].start_y + rand() % (rooms[i].height);
            }
            map[gold_y][gold_x] = 'b';
        }
        ///گذاشتن طلسم در اتاق
        for(int j = 0 ; j < num_talisman ; j++){
            int weapon_x = rooms[i].start_x + rand() % (rooms[i].width);
            int weapon_y = rooms[i].start_y + rand() % (rooms[i].height);
            while(map[weapon_y][weapon_x] != '.'){
                weapon_x = rooms[i].start_x + rand() % (rooms[i].width);
                weapon_y = rooms[i].start_y + rand() % (rooms[i].height);
            }
            int type_num = rand() % 5;
            char type[] = {'H', 's', 'd'};
            map[weapon_y][weapon_x] = type[type_num];
        }
        //گذاشتن تله در اتاق
        for(int j = 0 ; j < num_traps ; j++){
            int trap_x = rooms[i].start_x + rand() % (rooms[i].width);
            int trap_y = rooms[i].start_y + rand() % (rooms[i].height);
            while(map[trap_y][trap_x] != '.'){
                trap_x = rooms[i].start_x + rand() % (rooms[i].width);
                trap_y = rooms[i].start_y + rand() % (rooms[i].height);
            }
            map[trap_y][trap_x] = 'y';
        }
        //گذاشتن اسلحه ها 
        for(int j = 0 ; j < num_weapon ; j++){
            int weapon_x = rooms[i].start_x + rand() % (rooms[i].width);
            int weapon_y = rooms[i].start_y + rand() % (rooms[i].height);
            while(map[weapon_y][weapon_x] != '.'){
                weapon_x = rooms[i].start_x + rand() % (rooms[i].width);
                weapon_y = rooms[i].start_y + rand() % (rooms[i].height);
            }
            int type_num = rand() % 4;
            char type[] = {'E', 'W', 'N', 'L'};
            map[weapon_y][weapon_x] = type[type_num];
        }
        //گذاشتن پله
        if(i == stair_room && player->is_in_floor < 4){
            int stair_x = rooms[i].start_x + rand() % (rooms[i].width);
            int stair_y = rooms[i].start_y + rand() % (rooms[i].height);
            while(map[stair_y][stair_x] != '.' || rooms[i].theme == 't'){
                stair_x = rooms[i].start_x + rand() % (rooms[i].width);
                stair_y = rooms[i].start_y + rand() % (rooms[i].height);
            }
            map[stair_y][stair_x] = '<';
        }

        if(rooms[i].theme == 'e'){
            for (int y = rooms[i].start_y; y < rooms[i].start_y + rooms[i].height; y++) {
                for (int x = rooms[i].start_x; x < rooms[i].start_x + rooms[i].width; x++) {
                    if (map[y][x] == '+') { // در اتاق
                        map[y][x] = 'v';
                    }
                }
            }
        }
        
        
    }

    //گذاشتن غذا
    for(int i = 0; i < 7; i++){
        if(rooms[i].theme == 'n'){
            continue;
        }
        int room_index = rand() % num_rooms;
        int food_x = rooms[room_index].start_x + rand() % (rooms[room_index].width);
        int food_y = rooms[room_index].start_y + rand() % (rooms[room_index].height);
        while(map[food_y][food_x] != '.' || rooms[room_index].theme == 'n'){
            room_index = rand() % num_rooms;
            food_x = rooms[room_index].start_x + rand() % (rooms[room_index].width);
            food_y = rooms[room_index].start_y + rand() % (rooms[room_index].height);
        }
        int random_type = 1 + rand() % 4;
        foods[i].x = food_x;
        foods[i].y = food_y;
        
        foods[i].type = random_type;
        switch (random_type){
            case 1:
                strcpy(foods[i].name, "normal food");
                foods[i].restore_hunger = 10;
                foods[i].restore_health = 3;
                break;
            case 2:
                strcpy(foods[i].name, "high quality food");
                foods[i].restore_hunger = 20;
                foods[i].restore_health = 5;
                break;
            case 3:
                strcpy(foods[i].name, "magic food");
                foods[i].restore_hunger = 10;
                foods[i].restore_health = 10;
                break;
            case 4:
                strcpy(foods[i].name, "normal food");
                foods[i].restore_hunger = 0;
                foods[i].restore_health = -3;
                break;

        }
        map[food_y][food_x] = 'f';
    }

    
    //گذاشتن هیولا
    for(int i = 0; i < num_monster; i++){
        int room_index = rand() % num_rooms;
        int monster_x = rooms[room_index].start_x + rand() % (rooms[room_index].width);
        int monster_y = rooms[room_index].start_y + rand() % (rooms[room_index].height);
        while(map[monster_y][monster_x] != '.' || rooms[room_index].theme == 'n'|| rooms[room_index].theme == 'e'){
            room_index = rand() % num_rooms;
            monster_x = rooms[room_index].start_x + rand() % (rooms[room_index].width);
            monster_y = rooms[room_index].start_y + rand() % (rooms[room_index].height);
        }
        int type_num = rand() % 5;
        char type[] = {'D', 'F', 'G', 'S', 'U'};
        monsters[i] = createMonster(type_num); // نوع معمولی
        monsters[i].x = monster_x;
        monsters[i].y = monster_y;
        map[monster_y][monster_x] = type[type_num];
    }
    
    //گذاشتن علامت گنج
    if(player->is_in_floor == 4){
        int room_index = rand() % num_rooms;
        int Treasure_x = rooms[room_index].start_x + rand() % (rooms[room_index].width);
        int Treasure_y = rooms[room_index].start_y + rand() % (rooms[room_index].height);
        while(map[Treasure_y][Treasure_x] != '.' || rooms[room_index].theme == 't' || rooms[room_index].theme == 'n'){
            room_index = rand() % num_rooms;
            Treasure_x = rooms[room_index].start_x + rand() % (rooms[room_index].width);
            Treasure_y = rooms[room_index].start_y + rand() % (rooms[room_index].height);
        }
        map[Treasure_y][Treasure_x] = '*';
    }

    return map;
}

void display_map(char **map, int width, int height, Player player, Room *rooms, int num_rooms, int **map_visited) {
    start_color();
    init_pair(140,140,COLOR_BLACK);
    init_pair(179, 179, COLOR_BLACK);
    init_pair(25, 25, COLOR_BLACK);
    init_pair(95, 95, COLOR_BLACK);
    init_pair(1, COLOR_YELLOW, COLOR_BLACK);
    init_pair(2, COLOR_RED, COLOR_BLACK);
    init_pair(3, COLOR_BLUE, COLOR_BLACK);
    init_pair(4, COLOR_GREEN, COLOR_BLACK);
    init_pair(220, 220, COLOR_BLACK);   // gold
    init_pair(162, 162, COLOR_BLACK);  // ☁️ (Cloud)
    init_pair(196, 196, COLOR_BLACK); // ☺️ (Smile)
    init_pair(93, 93, COLOR_BLACK);  // ⚕️ (Medical Symbol)
    init_pair(35, 35, COLOR_BLACK);    // traps
    init_pair(205, 205, COLOR_BLACK);   // stairs
    init_pair(130, 130, COLOR_BLACK);
    init_pair(133, 133, COLOR_BLACK);
    init_pair(31, 31, COLOR_BLACK);
    init_pair(45, 45, COLOR_BLACK);
    init_pair(143, 143, COLOR_BLACK);
    init_pair(82, 82, COLOR_BLACK);
    init_pair(88, 88, COLOR_BLACK);
    init_pair(226, 226, COLOR_BLACK);
    const char *emojis[] = {"☠️", "☁️", "☺️", "\u2695", "\u26c0", "\u25B2", "\u2615", "\u2663" , "\u2660"};
    const char *emojisn[] = {"\u2663", "\u2020", "\u269A", "\u27B3", "\u2694", "\u2695", "\u26f7", "\u2620"};
    // متغیر برای چک کردن آیا بازیکن در اتاق با تم 'n' است یا خیر
    int in_nightmare_room = 0;

    // پیدا کردن اتاقی که بازیکن در آن قرار دارد
    for (int r = 0; r < num_rooms; r++) {
        if (player.x >= rooms[r].start_x && player.x < rooms[r].start_x + rooms[r].width &&
            player.y >= rooms[r].start_y && player.y < rooms[r].start_y + rooms[r].height) {
            if (rooms[r].theme == 'n') {
                in_nightmare_room = 1;
            }
            break;
        }
    } //اتاق کابوس
    if(in_nightmare_room){
        int view_radius = 2; // شعاع دید
        int room_color = 95;
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                if (abs(player.x - x) <= view_radius && abs(player.y - y) <= view_radius) {
                    attron(COLOR_PAIR(room_color));
                    if (map[y][x] == '|' || map[y][x] == '_') { // دیوارهای اتاق
                            mvprintw(y, x, "%c", map[y][x]);
                        } else if (map[y][x] == '.') { // فضای خالی اتاق
                            mvprintw(y, x, ".");
                        } else if (map[y][x] == '+') { // در اتاق
                            mvprintw(y, x, "+");
                        } else if (map[y][x] == 'o') { // نمایش اشیاء خاص (مثلاً آیتم‌ها)
                            attron(COLOR_PAIR(1));
                            mvprintw(y, x, "o");
                            attroff(COLOR_PAIR(1));
                            attron(COLOR_PAIR(room_color));
                        } else if (map[y][x] == '=') { // پنجره اتاق
                            mvprintw(y, x, "=");
                        } else if(map[y][x] == 'y'){
                            if(map_visited[y][x] == 0){
                                mvprintw(y, x, ".");
                            } else if(map_visited[y][x] == 1){
                                attron(COLOR_PAIR(35));
                                mvprintw(y, x, "\u2227");
                                attroff(COLOR_PAIR(35));
                                attron(COLOR_PAIR(room_color));
                            }
                        } else if(map[y][x] == 'h'){
                            if(map_visited[y][x] == 0){
                                int random_emoji = rand() % 8;
                                attron(COLOR_PAIR(220));
                                mvprintw(y, x, "%s", emojisn[random_emoji]);
                                attroff(COLOR_PAIR(220));
                                attron(COLOR_PAIR(room_color));
                            } else if(map_visited[y][x] == 1){
                                mvprintw(y, x, ".");
                            }
                        } else if(map[y][x] == 'v'){
                            if((y+1 < height && player.y == y + 1 && player.x == x) 
                            || (y > 1 && player.y == y - 1 && player.x == x) 
                            || (x+1 < width && player.x == x + 1 && player.y == y) 
                            || (x > 1 && player.x == x - 1 && player.y == y)){
                                mvprintw(y, x, "+");
                            }
                            else mvprintw(y, x, " ");
                        } else if(map[y][x] == '<'){
                            attron(COLOR_PAIR(205));
                            mvprintw(y, x, "<");
                            attroff(COLOR_PAIR(205));
                            attron(COLOR_PAIR(room_color));
                        } else if (map[y][x] == '@') { // نمایش بازیکن
                            if (player.color == 'r') {
                                attron(COLOR_PAIR(2));
                                mvprintw(y, x, "@");
                                attroff(COLOR_PAIR(2));
                            } else if (player.color == 'g') {
                                attron(COLOR_PAIR(4));
                                mvprintw(y, x, "@");
                                attroff(COLOR_PAIR(4));
                            } else if (player.color == 'b') {
                                attron(COLOR_PAIR(3));
                                mvprintw(y, x, "@");
                                attroff(COLOR_PAIR(3));
                            }
                        }
                    attroff(COLOR_PAIR(room_color));
                } else {
                    // خارج از محدوده دید
                    mvprintw(y, x, " ");
                }
            }
        }
    } 
    else{    
        //مشخص کردن اتاق های دیده شده
        for (int r = 0; r < num_rooms; r++) {
            for (int y = rooms[r].start_y; y < rooms[r].start_y + rooms[r].height; y++) {
                for (int x = rooms[r].start_x; x < rooms[r].start_x + rooms[r].width; x++) {
                    if(map_visited[y][x] == 1){
                        rooms[r].visited = 1;
                    }
                }
            }
            
        }

        // حلقه برای نمایش اتاق‌ها که بازدید شده‌اند
        for (int r = 0; r < num_rooms; r++) {
            if (rooms[r].visited == 1) { // فقط اتاق‌های بازدید شده
                int room_color;
                if(rooms[r].theme == 'r'){
                    room_color = 140;
                } else if(rooms[r].theme == 't'){
                    room_color = 179;
                } else if(rooms[r].theme == 'e'){
                    room_color = 25;
                } else if(rooms[r].theme == 'n'){
                    room_color = 95;
                }
                attron(COLOR_PAIR(room_color));
                // نمایش چیزمیزای اتاق
                for (int y = rooms[r].start_y; y < rooms[r].start_y + rooms[r].height; y++) {
                    for (int x = rooms[r].start_x; x < rooms[r].start_x + rooms[r].width; x++) {
                        if (map[y][x] == '|' || map[y][x] == '_') { // دیوارهای اتاق
                            mvprintw(y, x, "%c", map[y][x]);
                        } else if (map[y][x] == '.') { // فضای خالی اتاق
                            mvprintw(y, x, ".");
                        } else if (map[y][x] == '+') { // در اتاق
                            mvprintw(y, x, "+");
                        } else if (map[y][x] == 'o') { // نمایش اشیاء خاص (مثلاً آیتم‌ها)
                            attron(COLOR_PAIR(1));
                            mvprintw(y, x, "o");
                            attroff(COLOR_PAIR(1));
                            attron(COLOR_PAIR(room_color));
                        } else if (map[y][x] == '=') { // پنجره اتاق
                            mvprintw(y, x, "=");
                        } else if(map[y][x] == 'g'){
                            if(map_visited[y][x] == 0){
                                attron(COLOR_PAIR(220));
                                mvprintw(y, x, "\u26c0");
                                attroff(COLOR_PAIR(220));
                                attron(COLOR_PAIR(room_color));
                            } else if(map_visited[y][x] == 1){
                                mvprintw(y, x, ".");
                            }
                        } else if(map[y][x] == 'b'){
                            if(map_visited[y][x] == 0){
                                attron(COLOR_PAIR(162));
                                mvprintw(y, x, "\u26c0");
                                attroff(COLOR_PAIR(162));
                                attron(COLOR_PAIR(room_color));
                            } else if(map_visited[y][x] == 1){
                                mvprintw(y, x, ".");
                            }
                        } else if(map[y][x] == 'y'){
                            if(map_visited[y][x] == 0){
                                mvprintw(y, x, ".");
                            } else if(map_visited[y][x] == 1){
                                attron(COLOR_PAIR(35));
                                mvprintw(y, x, "\u2227");
                                attroff(COLOR_PAIR(35));
                                attron(COLOR_PAIR(room_color));
                            }
                        } else if(map[y][x] == 'h'){
                            if(map_visited[y][x] == 0){
                                attron(COLOR_PAIR(220));
                                mvprintw(y, x, "\u26c0");
                                attroff(COLOR_PAIR(220));
                                attron(COLOR_PAIR(room_color));
                            } else if(map_visited[y][x] == 1){
                                mvprintw(y, x, ".");
                            }
                        } else if(map[y][x] == 'v'){
                            if((y+1 < height && player.y == y + 1 && player.x == x) 
                            || (y > 1 && player.y == y - 1 && player.x == x) 
                            || (x+1 < width && player.x == x + 1 && player.y == y) 
                            || (x > 1 && player.x == x - 1 && player.y == y)){
                                mvprintw(y, x, "+");
                            }
                            else {
                                // بررسی اطراف برای پیدا کردن | یا _
                                char new_char = ' ';  // پیش‌فرض فضای خالی
                                
                                // بررسی بالا
                                if (y > 0 && map[y - 1][x] == '|') {
                                    new_char = '|';
                                }
                                // بررسی پایین
                                else if (y + 1 < height && map[y + 1][x] == '|') {
                                    new_char = '|';
                                }
                                // بررسی چپ
                                else if (x > 0 && map[y][x - 1] == '|') {
                                    new_char = '|';
                                }
                                // بررسی راست
                                else if (x + 1 < width && map[y][x + 1] == '|') {
                                    new_char = '|';
                                }

                                // بررسی بالا برای _
                                if (new_char == ' ' && y > 0 && map[y - 1][x] == '_') {
                                    new_char = '_';
                                }
                                // بررسی پایین برای _
                                else if (new_char == ' ' && y + 1 < height && map[y + 1][x] == '_') {
                                    new_char = '_';
                                }
                                // بررسی چپ برای _
                                else if (new_char == ' ' && x > 0 && map[y][x - 1] == '_') {
                                    new_char = '_';
                                }
                                // بررسی راست برای _
                                else if (new_char == ' ' && x + 1 < width && map[y][x + 1] == '_') {
                                    new_char = '_';
                                }

                                mvprintw(y, x, "%c", new_char);  // چاپ کاراکتر مناسب
                            }
                        } else if(map[y][x] == '<'){
                            attron(COLOR_PAIR(205));
                            mvprintw(y, x, "<");
                            attroff(COLOR_PAIR(205));
                            attron(COLOR_PAIR(room_color));
                        } else if(map[y][x] == 'f'){
                            if(map_visited[y][x] == 0){
                                attron(COLOR_PAIR(196));
                                mvprintw(y, x, "%s", emojis[7]);
                                attroff(COLOR_PAIR(196));
                                attron(COLOR_PAIR(room_color));
                            } else if(map_visited[y][x] == 1){
                                mvprintw(y, x, ".");
                            }
                        } else if(map[y][x] == 'E' || map[y][x] == 'e'){
                            if(map_visited[y][x] == 0){
                                attron(COLOR_PAIR(133));
                                mvprintw(y, x, "\u2020");
                                attroff(COLOR_PAIR(133));
                                attron(COLOR_PAIR(room_color));
                            } else if(map_visited[y][x] == 1){
                                mvprintw(y, x, ".");
                            }
                        } else if(map[y][x] == 'W' || map[y][x] == 'w'){
                            if(map_visited[y][x] == 0){
                                attron(COLOR_PAIR(31));
                                mvprintw(y, x, "\u269A");
                                attroff(COLOR_PAIR(31));
                                attron(COLOR_PAIR(room_color));
                            } else if(map_visited[y][x] == 1){
                                mvprintw(y, x, ".");
                            }
                        } else if(map[y][x] == 'N' || map[y][x] == 'n'){
                            if(map_visited[y][x] == 0){
                                attron(COLOR_PAIR(45));
                                mvprintw(y, x, "\u27B3");
                                attroff(COLOR_PAIR(45));
                                attron(COLOR_PAIR(room_color));
                            } else if(map_visited[y][x] == 1){
                                mvprintw(y, x, ".");
                            }
                        } else if(map[y][x] == 'L' || map[y][x] == 'l'){
                            if(map_visited[y][x] == 0){
                                attron(COLOR_PAIR(143));
                                mvprintw(y, x, "\u2694");
                                attroff(COLOR_PAIR(143));
                                attron(COLOR_PAIR(room_color));
                            } else if(map_visited[y][x] == 1){
                                mvprintw(y, x, ".");
                            }
                        } else if(map[y][x] == 'H'){
                            if(map_visited[y][x] == 0){
                                attron(COLOR_PAIR(93));
                                mvprintw(y, x, "\u2695");
                                attroff(COLOR_PAIR(93));
                                attron(COLOR_PAIR(room_color));
                            } else if(map_visited[y][x] == 1){
                                mvprintw(y, x, ".");
                            }
                        } else if(map[y][x] == 's'){
                            if(map_visited[y][x] == 0){
                                attron(COLOR_PAIR(226));
                                mvprintw(y, x, "\u26f7");
                                attroff(COLOR_PAIR(226));
                                attron(COLOR_PAIR(room_color));
                            } else if(map_visited[y][x] == 1){
                                mvprintw(y, x, ".");
                            }
                        } else if(map[y][x] == 'd'){
                            if(map_visited[y][x] == 0){
                                attron(COLOR_PAIR(88));
                                mvprintw(y, x, "\u2620");
                                attroff(COLOR_PAIR(88));
                                attron(COLOR_PAIR(room_color));
                            } else if(map_visited[y][x] == 1){
                                mvprintw(y, x, ".");
                            }
                        } else if(map[y][x] == 'D'|| map[y][x] == 'F'|| map[y][x] == 'G'|| map[y][x] == 'S'|| map[y][x] == 'U'){
                            mvprintw(y, x, "%c", map[y][x]);
                        } else if(map[y][x] == '*'){
                            attron(COLOR_PAIR(226));
                            mvprintw(y, x, "\u2600");
                            attroff(COLOR_PAIR(226));
                            attron(COLOR_PAIR(room_color));
                        } else{
                            mvprintw(y, x, "%c", map[y][x]);
                        }
                    }
                }
                attroff(COLOR_PAIR(room_color));
            }
        }

        // نمایش برای چیزهای دیگری که دیده شده و خود کاراکتر اصلی
        for (int i = 0; i < width; i++) {
            for (int j = 0; j < height; j++) {
                if (map[j][i] == '@') { // نمایش بازیکن
                    if (player.color == 'r') {
                        attron(COLOR_PAIR(2));
                        mvprintw(j, i, "@");
                        attroff(COLOR_PAIR(2));
                    } else if (player.color == 'g') {
                        attron(COLOR_PAIR(4));
                        mvprintw(j, i, "@");
                        attroff(COLOR_PAIR(4));
                    } else if (player.color == 'b') {
                        attron(COLOR_PAIR(3));
                        mvprintw(j, i, "@");
                        attroff(COLOR_PAIR(3));
                    }
                }
                if (map_visited[j][i] == 1 && (map[j][i] == '#')){
                    mvprintw(j, i, "%c", map[j][i]);
                }
            }
        }

        //نمایش 2 واحد جلوتر در راهرو
        if(map[player.y +1][player.x] == '#' || map[player.y][player.x - 1 ] == '#' || map[player.y][player.x + 1] == '#' || map[player.y -1][player.x] == '#'){
            if(player.direction[0] == 'x'){
                for(int a = 0; a < 3; a++){
                    if(player.direction[1] == '+'){
                        if(player.x + a > 0 && player.x + a < width){
                            if(map[player.y][player.x + a] == '#'){
                                mvprintw(player.y, player.x + a, "#");
                            } 
                        }
                    }
                    else if(player.direction[1] == '-'){
                            if(player.x - a > 0 && player.x - a < width){
                                if(map[player.y][player.x - a] == '#'){
                                    mvprintw(player.y, player.x - a, "#");
                                } 
                            }
                    }
                        
                }
            }
            else if(player.direction[0] == 'y'){     
                if(player.direction[1] == '+'){
                    for(int b = 0; b < 3; b++){
                        if(player.y + b < height && player.y + b > 0){
                            if(map[player.y + b][player.x] == '#'){
                                mvprintw(player.y + b, player.x, "#");
                            } 
                        }
                        
                    }
                }
                else if(player.direction[1] == '-'){
                    for(int b = 0; b < 3; b++){
                        if(player.y - b < height && player.y - b > 0){
                            if(map[player.y - b][player.x] == '#'){
                                mvprintw(player.y - b, player.x, "#");
                            } 
                        }
                        
                    }
                }     
                
            }
            
        }
    }
}

void display_whole_map(char ** map, int width, int height, Player player, Room *rooms, int num_rooms, int **map_visited){
    start_color();
    init_pair(1, COLOR_YELLOW, COLOR_BLACK);
    init_pair(2, COLOR_RED, COLOR_BLACK);
    init_pair(3, COLOR_BLUE, COLOR_BLACK);
    init_pair(4, COLOR_GREEN, COLOR_BLACK);
    init_pair(140,140,COLOR_BLACK);
    init_pair(178, 178, COLOR_BLACK);
    init_pair(25, 25, COLOR_BLACK);
    init_pair(95, 95, COLOR_BLACK);
    init_pair(220, 220, COLOR_BLACK);   // gold
    init_pair(162, 162, COLOR_BLACK);  // ☁️ (Cloud)
    init_pair(196, 196, COLOR_BLACK); // ☺️ (Smile)
    init_pair(93, 93, COLOR_BLACK);  // ⚕️ (Medical Symbol)
    init_pair(35, 35, COLOR_BLACK);    // traps
    init_pair(205, 205, COLOR_BLACK);   // stairs
    init_pair(130, 130, COLOR_BLACK);
    init_pair(133, 133, COLOR_BLACK);
    init_pair(31, 31, COLOR_BLACK);
    init_pair(45, 45, COLOR_BLACK);
    init_pair(143, 143, COLOR_BLACK);
    init_pair(82, 82, COLOR_BLACK);
    init_pair(88, 88, COLOR_BLACK);
    init_pair(226, 226, COLOR_BLACK);
    const char *emojis[] = {"☠️", "☁️", "☺️", "\u2695", "\u26c0", "\u25B2", "\u2615", "\u2663" , "\u2660"};
    const char *emojisn[] = {"\u2663", "\u2020", "\u269A", "\u27B3", "\u2694", "\u2695", "\u26f7", "\u2620"};
    int room_color;
    for (int r = 0; r < num_rooms; r++) {
        if(rooms[r].theme == 'r'){
            room_color = 140;
        } else if(rooms[r].theme == 't'){
            room_color = 179;
        } else if(rooms[r].theme == 'e'){
            room_color = 25;
        } else if(rooms[r].theme == 'n'){
            room_color = 95;
        }
        attron(COLOR_PAIR(room_color));
        // نمایش دیوارهای اتاق
        for (int y = rooms[r].start_y; y < rooms[r].start_y + rooms[r].height; y++) {
            for (int x = rooms[r].start_x; x < rooms[r].start_x + rooms[r].width; x++) {
                switch (map[y][x]) {
                    case '|':
                    case '_': // دیوارها
                        mvprintw(y, x, "%c", map[y][x]);
                        break;
                    case '.': // فضای خالی
                        mvprintw(y, x, ".");
                        break;
                    case '+': // درب‌ها
                        mvprintw(y, x, "+");
                        break;
                    case '=':
                        mvprintw(y, x, "=");
                        break;
                    case 'o': // موانع
                        attron(COLOR_PAIR(1));
                        mvprintw(y, x, "o");
                        attroff(COLOR_PAIR(1));
                        attron(COLOR_PAIR(room_color));
                        break;
                    case 'g': // گنج
                        if (map_visited[y][x] == 0) {
                            attron(COLOR_PAIR(220));
                            mvprintw(y, x, "\u26c0");
                            attroff(COLOR_PAIR(220));
                            attron(COLOR_PAIR(room_color));
                        } else {
                            mvprintw(y, x, ".");
                        }
                        break;
                    case 'b':
                        if(map_visited[y][x] == 0){
                            attron(COLOR_PAIR(162));
                            mvprintw(y, x, "\u26c0");
                            attroff(COLOR_PAIR(162));
                            attron(COLOR_PAIR(room_color));
                        } else if(map_visited[y][x] == 1){
                            mvprintw(y, x, ".");
                        }
                        break;
                    case '<': //تله
                        attron(COLOR_PAIR(205));
                        mvprintw(y, x, "<");
                        attroff(COLOR_PAIR(205));
                        attron(COLOR_PAIR(room_color));
                        break;
                    case 'y':
                        if(map_visited[y][x] == 0){
                            mvprintw(y, x, ".");
                        } else if(map_visited[y][x] == 1){
                            attron(COLOR_PAIR(35));
                            mvprintw(y, x, "\u2227");
                            attroff(COLOR_PAIR(35));
                            attron(COLOR_PAIR(room_color));
                        }
                        break;
                    case 'h':
                        if(map_visited[y][x] == 0){
                            int random_emoji = rand() % 8;
                            attron(COLOR_PAIR(220));
                            mvprintw(y, x, "%s", emojisn[random_emoji]);
                            attroff(COLOR_PAIR(220));
                            attron(COLOR_PAIR(room_color));
                        } else if(map_visited[y][x] == 1){
                            mvprintw(y, x, ".");
                        }
                        break;
                    case 'v':
                        if((y+1 < height && player.y == y + 1 && player.x == x) 
                        || (y > 1 && player.y == y - 1 && player.x == x) 
                        || (x+1 < height && player.x == x + 1 && player.y == y) 
                        || (x > 1 && player.x == x - 1 && player.y == y)){
                            mvprintw(y, x, "+");
                        }
                        else {
                                // بررسی اطراف برای پیدا کردن | یا _
                                char new_char = ' ';  // پیش‌فرض فضای خالی
                                
                                // بررسی بالا
                                if (y > 0 && map[y - 1][x] == '|') {
                                    new_char = '|';
                                }
                                // بررسی پایین
                                else if (y + 1 < height && map[y + 1][x] == '|') {
                                    new_char = '|';
                                }
                                // بررسی چپ
                                else if (x > 0 && map[y][x - 1] == '|') {
                                    new_char = '|';
                                }
                                // بررسی راست
                                else if (x + 1 < width && map[y][x + 1] == '|') {
                                    new_char = '|';
                                }

                                // بررسی بالا برای _
                                if (new_char == ' ' && y > 0 && map[y - 1][x] == '_') {
                                    new_char = '_';
                                }
                                // بررسی پایین برای _
                                else if (new_char == ' ' && y + 1 < height && map[y + 1][x] == '_') {
                                    new_char = '_';
                                }
                                // بررسی چپ برای _
                                else if (new_char == ' ' && x > 0 && map[y][x - 1] == '_') {
                                    new_char = '_';
                                }
                                // بررسی راست برای _
                                else if (new_char == ' ' && x + 1 < width && map[y][x + 1] == '_') {
                                    new_char = '_';
                                }

                                mvprintw(y, x, "%c", new_char);  // چاپ کاراکتر مناسب
                            }
                        break;
                    case 'f':
                        if(map_visited[y][x] == 0){
                            attron(COLOR_PAIR(196));
                            mvprintw(y, x, "%s", emojis[7]);
                            attroff(COLOR_PAIR(196));
                            attron(COLOR_PAIR(room_color));
                        } else if(map_visited[y][x] == 1){
                            mvprintw(y, x, ".");
                        }
                        break;
                    case 'M':
                        if(map_visited[y][x] == 0){
                            attron(COLOR_PAIR(130));
                            mvprintw(y, x, "\u2692");
                            attroff(COLOR_PAIR(130));
                            attron(COLOR_PAIR(room_color));
                        } else if(map_visited[y][x] == 1){
                            mvprintw(y, x, ".");
                        }
                        break;
                    case 'e':
                    case 'E':
                        if(map_visited[y][x] == 0){
                            attron(COLOR_PAIR(133));
                            mvprintw(y, x, "\u2020");
                            attroff(COLOR_PAIR(133));
                            attron(COLOR_PAIR(room_color));
                        } else if(map_visited[y][x] == 1){
                            mvprintw(y, x, ".");
                        }
                        break;
                    case 'W':
                    case 'w':
                        if(map_visited[y][x] == 0){
                            attron(COLOR_PAIR(31));
                            mvprintw(y, x, "\u269A");
                            attroff(COLOR_PAIR(31));
                            attron(COLOR_PAIR(room_color));
                        } else if(map_visited[y][x] == 1){
                            mvprintw(y, x, ".");
                        }
                        break;
                    case 'N':
                    case 'n':
                        if(map_visited[y][x] == 0){
                            attron(COLOR_PAIR(45));
                            mvprintw(y, x, "\u27B3");
                            attroff(COLOR_PAIR(45));
                            attron(COLOR_PAIR(room_color));
                        } else if(map_visited[y][x] == 1){
                            mvprintw(y, x, ".");
                        }
                        break;
                    case 'L':
                    case 'l':
                        if(map_visited[y][x] == 0){
                            attron(COLOR_PAIR(143));
                            mvprintw(y, x, "\u2694");
                            attroff(COLOR_PAIR(143));
                            attron(COLOR_PAIR(room_color));
                        } else if(map_visited[y][x] == 1){
                            mvprintw(y, x, ".");
                        }
                        break;
                    case 'H':
                        if(map_visited[y][x] == 0){
                            attron(COLOR_PAIR(93));
                            mvprintw(y, x, "\u2695");
                            attroff(COLOR_PAIR(93));
                            attron(COLOR_PAIR(room_color));
                        } else if(map_visited[y][x] == 1){
                            mvprintw(y, x, ".");
                        }
                        break;
                    case 's':
                        if(map_visited[y][x] == 0){
                            attron(COLOR_PAIR(226));
                            mvprintw(y, x, "\u26f7");
                            attroff(COLOR_PAIR(226));
                            attron(COLOR_PAIR(room_color));
                        } else if(map_visited[y][x] == 1){
                            mvprintw(y, x, ".");
                        }
                        break;
                    case 'd':
                        if(map_visited[y][x] == 0){
                            attron(COLOR_PAIR(88));
                            mvprintw(y, x, "\u2620");
                            attroff(COLOR_PAIR(88));
                            attron(COLOR_PAIR(room_color));
                        } else if(map_visited[y][x] == 1){
                            mvprintw(y, x, ".");
                        }
                        break;
                    case '*':
                        attron(COLOR_PAIR(226));
                        mvprintw(y, x, "\u2600");
                        attroff(COLOR_PAIR(226));
                        attron(COLOR_PAIR(room_color));
                        break;
                    default:
                        mvprintw(y, x, "%c", map[y][x]);
                        break;
                }
            }
        }
        attroff(COLOR_PAIR(room_color));
    
    }

        // نمایش برای چیزهای دیگری که دیده شده و خود کاراکتر اصلی
    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
            if (map[j][i] == '@') { // نمایش بازیکن
                if (player.color == 'r') {
                    attron(COLOR_PAIR(2));
                    mvprintw(j, i, "@");
                    attroff(COLOR_PAIR(2));
                } else if (player.color == 'g') {
                    attron(COLOR_PAIR(4));
                    mvprintw(j, i, "@");
                    attroff(COLOR_PAIR(4));
                } else if (player.color == 'b') {
                    attron(COLOR_PAIR(3));
                    mvprintw(j, i, "@");
                    attroff(COLOR_PAIR(3));
                }
            }
            if (map[j][i] == '#'){
                mvprintw(j, i, "%c", map[j][i]);
            }
        }
    }
    
}