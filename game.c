#include <ncurses.h>
#include <string.h>  
#include <unistd.h> 
#include <stdlib.h>
#include <locale.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>
#include <errno.h>
#include <time.h>
#include "game.h"
#include "startup.h"
#include "menu.h"
#include "create_map.h"
#define HUNGER_DECREASE_INTERVAL 15 // کاهش گرسنگی هر 5 ثانیه

void show_message(WINDOW *msg_win, const char *message) {
    wclear(msg_win); // پاک کردن محتوای قبلی پنجره
    box(msg_win, 0, 0); // افزودن کادر دور پنجره
    mvwprintw(msg_win, 1, 1, "%s", message); // نوشتن پیام جدید
    wrefresh(msg_win); // به‌روزرسانی پنجره
}
/*
    // ایجاد پنجره برای پیام‌ها
    WINDOW *msg_win = newwin(3, 50, height - 3, width/2 + 2);
    show_message(msg_win, "Game saved successfully!");
    wrefresh(msg_win);
    // مکث برای مشاهده پیام
    sleep(2);
    //------------------
    int height = 3, width = 50, start_y = 1, start_x = 1;
    //ایجاد پنجره برای پیام‌ها
    WINDOW *msg_win = newwin(height, width, start_y, start_x);
    wrefresh(msg_win);
*/
void moveMonster(Monster *m, Player hero, char **map) {
    int dx = (hero.x > m->x) ? 1 : (hero.x < m->x) ? -1 : 0;
    int dy = (hero.y > m->y) ? 1 : (hero.y < m->y) ? -1 : 0;

    switch (m->type) {
        case 1: // Snake: تا زمان نابودی دنبال می‌کند
            if(map[m->y + dy][m->x + dx] == '.'){
                m->x += dx;
                m->y += dy;
            }
            
            break;

        case 2: // Monster خسته‌شونده: پس از ۵ قدم می‌ایستد
            if (m->stepsTaken < 5 && map[m->y + dy][m->x + dx] == '.') {
                m->x += dx;
                m->y += dy;
                m->stepsTaken++;
            }
            break;

        case 3: // تغییر مسیر دهنده: هر ۳ قدم جهت جدید انتخاب می‌کند
            if (m->stepsTaken % 3 == 0) {
                int randDir = rand() % 4;
                dx = (randDir == 0) ? 1 : (randDir == 1) ? -1 : 0;
                dy = (randDir == 2) ? 1 : (randDir == 3) ? -1 : 0;
            }
            if(map[m->y + dy][m->x + dx] == '.'){
                m->x += dx;
                m->y += dy;
                m->stepsTaken++;
            }
            
            break;

        case 4: // هیولاهای دیگر: حرکت ساده به سمت قهرمان
            if((abs(m->x - hero.x) <= 1 || abs(m->y - hero.y) <= 1) && map[m->y + dy][m->x + dx] == '.'){
                m->x += dx;
                m->y += dy; 
            }   
            break;
    }
}

talisman createTalisman(TalismanType type) {
    talisman talisman;
    talisman.type = type;
    switch (type) {
        case HEALTH:
            strcpy(talisman.name, "Health talisman");
            strcpy(talisman.symbol, "\u2695");
            talisman.lifetime = 0;
            talisman.active = 0;
            break;
        case SPEED:
            strcpy(talisman.name, "speed talisman");
            strcpy(talisman.symbol, "\u26f7");
            talisman.lifetime = 0;
            talisman.active = 0;
            break;
        case DAMAGE:
            strcpy(talisman.name, "damage talisman");
            strcpy(talisman.symbol, "\u2620");
            talisman.lifetime = 0;
            talisman.active = 0;
            break;
    }

    return talisman;
}
void talisman_menu(Player *player, WINDOW *menu_win) {
    if (player->talisman_count == 0) {
        mvwprintw(menu_win, 1, 1, "No talismans available!");
        wrefresh(menu_win);
        sleep(1);
        return;
    }

    // نمایش منوی طلسم ها
    mvwprintw(menu_win, 1, 1, "Choose a talisman to equip:");
    for (int i = 0; i < player->talisman_count; i++) {
        mvwprintw(menu_win, 2 + i, 1, "%d. %s ", 
                  i + 1, player->talisman_list[i].name);
    }
    mvwprintw(menu_win, 2 + player->talisman_count, 1, "Press the number of your choice: ");
    wrefresh(menu_win);

    // خواندن انتخاب کاربر
    int choice = wgetch(menu_win) - '0';
    if (choice < 1 || choice > player->talisman_count) {
        mvwprintw(menu_win, 2 + player->talisman_count + 1, 1, "Invalid choice!");
        wrefresh(menu_win);
        sleep(1);
        return;
    }

    // تجهیز طلسم
    talisman selected_talisman = player->talisman_list[choice - 1];
    switch (selected_talisman.type) {
        case HEALTH:
            player->health_speed = 2;
            break;
        case SPEED:
            player->speed = 2;
            break;
        case DAMAGE:
            player->power = 2;
            break;
    }
    player->talisman_list[choice - 1].active = 1;
    player->current_talisman = player->talisman_list[choice - 1];
    player->current_talisman.lifetime = 0;
    mvwprintw(menu_win, 2 + player->talisman_count + 2, 1, "You choosed %s talisman.", selected_talisman.name);
    wrefresh(menu_win);
    sleep(1);
    for (int j = choice - 1; j < player->talisman_count - 1; j++) {
        player->talisman_list[j] = player->talisman_list[j+1];
    }
    player->talisman_count--;  

}
bool all_monsters_defeated(Monster *monsters, int num_monsters) {
    for (int i = 0; i < num_monsters; i++) {
        if (monsters[i].health > 0) {
            return false;  // هنوز دشمنی زنده است.
        }
    }
    return true;  // همه دشمنان شکست خورده‌اند.
}
void display_victory_message(int score) {
    int width, height;
    getmaxyx(stdscr, height, width);
    clear();  // Clear the screen
    attron(A_BOLD);  // Make the text bold
    mvprintw(5, (width-12)/2, "🎉 Victory! 🎉");
    attroff(A_BOLD);
    mvprintw(7, (width-55)/2, "Congratulations! You have defeated all the enemies! 🏆");
    mvprintw(9, (width-25)/2, "Your final score: %d ⭐", score);
    mvprintw(11, (width-50)/2, "Press any key to return to the main menu... 🔙");
    refresh();
    sleep(2);
    getch();  // Wait for the player to press a key
}
void display_game_over_message(int score) {
    int width, height;
    getmaxyx(stdscr, height, width);
    clear();  // پاک کردن صفحه
    attron(A_BOLD);  // برجسته کردن متن
    mvprintw(5, (width-15)/2, "💀 Game Over 💀");
    attroff(A_BOLD);
    mvprintw(7, (width-50)/2, "You fought bravely, but the enemies were too strong... ⚔️");
    mvprintw(9, (width-25)/2, "Your final score: %d 💔", score);
    mvprintw(11, (width-45)/2, "Press any key to try again or return to the menu 🔄");
    refresh();
    sleep(2);
    getch();  // صبر برای فشردن کلید توسط بازیکن
}


// تابعی برای ایجاد یک سلاح جدید
Weapon createWeapon(WeaponType type) {
    Weapon weapon;
    weapon.type = type;
    switch (type) {
        case MACE:
            strcpy(weapon.name, "Mace");
            strcpy(weapon.symbol, "\u2692");
            weapon.damage = 5;
            weapon.is_melee = 1;
            weapon.range = 1;
            weapon.numbers = 0;
            weapon.is_consumable = 0;
            weapon.color_pair = 130;
            break;
        case DAGGER:
            strcpy(weapon.name, "Dagger");
            strcpy(weapon.symbol, "\u2020");
            weapon.damage = 12;
            weapon.is_melee = 1;
            weapon.range = 5;
            weapon.numbers = 0;
            weapon.is_consumable = 1;
            weapon.symbol_on_map = 'e';
            weapon.color_pair = 133;
            break;
        case MAGIC_WAND:
            strcpy(weapon.name, "Magic Wand");
            strcpy(weapon.symbol, "\u269A");
            weapon.damage = 15;
            weapon.is_melee = 0;
            weapon.range = 10;
            weapon.numbers = 0;
            weapon.is_consumable = 1;
            weapon.symbol_on_map = 'w';
            weapon.color_pair = 31;
            break;
        case NORMAL_ARROW:
            strcpy(weapon.name, "Normal Arrow");
            strcpy(weapon.symbol, "\u27B3");
            weapon.damage = 5;
            weapon.range = 5;
            weapon.is_melee = 0;
            weapon.numbers = 0;
            weapon.symbol_on_map = 'n';
            weapon.is_consumable = 1;
            weapon.color_pair = 45;
            break;
        case SWORD:
            strcpy(weapon.name, "Sword");
            strcpy(weapon.symbol, "\u2694");
            weapon.damage = 10;
            weapon.is_melee = 1;
            weapon.range = 1;
            weapon.numbers = 0;
            weapon.is_consumable = 0;
            weapon.symbol_on_map = 'l';
            weapon.color_pair = 143;
            break;
    }

    return weapon;
}
void display_weapons(Player player, int height, int width) {
    mvprintw(height - 2, width/6, "Weapon: ");
    int x = 9;  // موقعیت شروع نمایش سلاح‌ها
    attron(COLOR_PAIR(player.Weapon_list[0].color_pair));
    mvprintw(height - 2, width/6 + x, "%s", player.Weapon_list[0].symbol); 
    x += 3; // فاصله بین سلاح‌ها
    attroff(COLOR_PAIR(player.Weapon_list[0].color_pair));
    for (int i = 1; i < 5; i++) {
        if(player.Weapon_list[i].numbers != 0){
            attron(COLOR_PAIR(player.Weapon_list[i].color_pair));
            mvprintw(height - 2, width/6 + x, "%s", player.Weapon_list[i].symbol); 
            attroff(COLOR_PAIR(player.Weapon_list[i].color_pair));
            x += 3; // فاصله بین سلاح‌ها
        }
    }
}
void equip_weapon_ncurses(Player *player, WINDOW *menu_win) {
    // نمایش منوی سلاح‌ها
    mvwprintw(menu_win, 1, 1, "Choose a weapon to equip:");

    // نمایش دسته‌بندی سلاح‌ها
    mvwprintw(menu_win, 2, 1, "Melee Weapons:");
    int line = 3;
    for (int i = 0; i < 5; i++) {
        if (player->Weapon_list[i].is_melee == 1) {
            mvwprintw(menu_win, line++, 1, "%d. %s %s  (Damage: %d) (x", 
                      i + 1, player->Weapon_list[i].name,player->Weapon_list[i].symbol, player->Weapon_list[i].damage);
            wprintw(menu_win, "%d)", player->Weapon_list[i].numbers);
        }
    }

    mvwprintw(menu_win, line++, 1, "Ranged Weapons:");
    for (int i = 0; i < 5; i++) {
        if (player->Weapon_list[i].is_melee == 0) {
            mvwprintw(menu_win, line++, 1, "%d. %s %s  (Damage: %d) (x", 
                      i + 1, player->Weapon_list[i].name, player->Weapon_list[i].symbol, player->Weapon_list[i].damage);
            wprintw(menu_win, "%d)", player->Weapon_list[i].numbers);
        }
    }

    mvwprintw(menu_win, line++, 1, "Press the number of your choice: ");
    mvwprintw(menu_win, line++, 1, "(click 'w' if you want to put your weapon");
    mvwprintw(menu_win, line++, 1, "in the bag)");
    wrefresh(menu_win);
    int ch = wgetch(menu_win);
    if (ch == 'w') {
        for(int i = 0; i < 5; i++){
            if (strcmp(player->equipped_weapon.name, player->Weapon_list[i].name) == 0) {
                player->Weapon_list[i].numbers++;
            }
        }
        Weapon empty_weapon = {0};  // تمام فیلدها به صفر مقداردهی می‌شوند
        player->equipped_weapon = empty_weapon;
        strcpy(player->equipped_weapon.symbol , " ");
        ch = getch();
    }

    // خواندن انتخاب کاربر
    int choice = ch - '0';
    if (choice < 1 || choice > 5) {
        mvwprintw(menu_win, line + 2, 1, "Invalid choice!");
        wrefresh(menu_win);
        sleep(1);
        return;
    } else if(player->Weapon_list[choice - 1].numbers == 0){
        mvwprintw(menu_win, line + 2, 1, "You don't have this weapon!");
        wrefresh(menu_win);
        sleep(1);
        return;
    } else if(strcmp(player->equipped_weapon.symbol , " ") != 0){
        mvwprintw(menu_win, line + 2, 1, "Put your current weapon in the bag first!");
        wrefresh(menu_win);
        sleep(1);
        return;
    }

    // تجهیز سلاح
    Weapon selected_weapon = player->Weapon_list[choice - 1];
    if (strcmp(player->equipped_weapon.name, selected_weapon.name) == 0) {
        mvwprintw(menu_win, line++, 1, "You already have this weapon equipped.");
        wrefresh(menu_win);
        sleep(1);
        return;
    }

    player->equipped_weapon = selected_weapon;

    // کاهش تعداد سلاح‌های مصرفی
    if (selected_weapon.is_consumable && selected_weapon.numbers > 0) {
        player->Weapon_list[choice - 1].numbers--;
    }

    mvwprintw(menu_win, line++, 1, "You equipped %s.", selected_weapon.name);
    wrefresh(menu_win);
    sleep(1);
}

void update_hunger(Player *player) {
    if (player->hunger > 0) {
        player->hunger -= 1; // کاهش گرسنگی
    } else if (player->health > 0) {
        player->health -= 1; // کاهش سلامتی به دلیل گرسنگی شدید
    }
}
void display_hunger_bar_ncurses(int hunger, int max_hunger, int y, int x) {
    int bar_length = 20; // طول نوار گرسنگی
    int filled = (hunger * bar_length) / max_hunger;
    init_pair(162, 162, COLOR_BLACK);
    mvprintw(y, x, "Satiety: [");
    attron(COLOR_PAIR(162));
    for (int i = 0; i < filled; i++) {
        printw("#"); // قسمت پر شده
    }
    attroff(COLOR_PAIR(162));
    for (int i = filled; i < bar_length; i++) {
        printw("-"); // قسمت خالی
    }
    printw("] %d%%", hunger);

    refresh(); // به‌روزرسانی پنجره
}
void display_health_bar_ncurses(int health, int max_health, int y, int x) {
    int bar_length = 20; // طول نوار گرسنگی
    int filled = (health * bar_length) / max_health;
    init_pair(85, 85, COLOR_BLACK);
    mvprintw(y, x, "Health: [");
    attron(COLOR_PAIR(85));
    for (int i = 0; i < filled; i++) {
        printw("#"); // قسمت پر شده
    }
    attroff(COLOR_PAIR(85));
    for (int i = filled; i < bar_length; i++) {
        printw("-"); // قسمت خالی
    }
    printw("]");
    printw("%d%%", health);

    refresh(); // به‌روزرسانی پنجره
}
void consume_food_ncurses(Player *player, WINDOW *menu_win) {
    if (player->food_count == 0) {
        mvwprintw(menu_win, 1, 1, "No food available!");
        wrefresh(menu_win);
        sleep(1);
        return;
    }

    // نمایش منوی غذا
    mvwprintw(menu_win, 1, 1, "Choose a food to consume:");
    for (int i = 0; i < player->food_count; i++) {
        mvwprintw(menu_win, 2 + i, 1, "%d. %s", 
                  i + 1, player->inventory[i].name);
    }
    mvwprintw(menu_win, 2 + player->food_count, 1, "Press the number of your choice: ");
    wrefresh(menu_win);

    // خواندن انتخاب کاربر
    int choice = wgetch(menu_win) - '0'; // تبدیل ورودی کاراکتری به عدد
    if (choice < 1 || choice > player->food_count) {
        mvwprintw(menu_win, 2 + player->food_count + 1, 1, "Invalid choice!");
        wrefresh(menu_win);
        sleep(1);
        return;
    }

    // مصرف غذا
    if(player->hunger == 100){
        mvwprintw(menu_win, 2 + player->food_count + 2, 1, "You are full!");
        wrefresh(menu_win);
        sleep(1);
    } else{
        Food selected_food = player->inventory[choice - 1];
        player->hunger += selected_food.restore_hunger;
        player->health += selected_food.restore_health;
        if (player->hunger > 100) {
            player->hunger = 100;
        }

        // حذف غذا از موجودی
        for (int i = choice - 1; i < player->food_count - 1; i++) {
            player->inventory[i] = player->inventory[i + 1];
        }
        player->food_count--; 
        mvwprintw(menu_win, 2 + player->food_count + 2, 1, "You consumed %s.", selected_food.name);
        wrefresh(menu_win);               
        sleep(1);
    }
}

// تابع بررسی و بازگرداندن هیولا در موقعیت مشخص
Monster* getMonsterAtPosition(Monster monsters[], int monster_count, int x, int y) {
    for (int i = 0; i < monster_count; i++) {
        if (monsters[i].x == x && monsters[i].y == y && monsters[i].health > 0) {
            return &monsters[i];  // بازگرداندن اشاره‌گر به هیولای پیدا شده
        }
    }
    return NULL;  // در صورتی که هیولایی پیدا نشود
}

int can_go(int y, int x, char **map, Player* player, int ***map_visited, int g_clicked, Food foods[7], int num_rooms, Room rooms[num_rooms]){
    int height, width;
    getmaxyx(stdscr, height, width);
    if(map[y][x] == '.' || map[y][x] == '#' || map[y][x] == '+' || map[y][x] == 'h'){
        (*map_visited)[y][x] = 1;
        return 1;
    } else if(map[y][x] == 'y'){
        player->health -= 10;
        return 1;
    } else if(map[y][x] == 'g' && g_clicked == 0){
        player->collected_golds++;
        return 1;
    } else if(map[y][x] == '<'){
        player->is_in_floor++;
        return 1;
    } else if(map[y][x] == 'v'){
        return 1;
    } else if(map[y][x] == 'f'){
        Food food;
        for(int i = 0; i < 7; i++){
            if(foods[i].x == x && foods[i].y == y){
                food = foods[i];
            }
        }
        if (player->food_count < 5) {
            player->inventory[player->food_count++] = food;
            return 1;
        } else{
            WINDOW *msg_win = newwin(3, 50, height - 3, width/2 + 2);
            show_message(msg_win, "You can't collect more than 5 foods");
            wrefresh(msg_win);
            // مکث برای مشاهده پیام
            sleep(1);
            return 0;
        }
    } else if(map[y][x] == 'b'){
        player->collected_golds += 5;
        return 1;
    } else if(map[y][x] == 'M'){
        WINDOW *msg_win = newwin(3, 50, (height - 5)/2, (width - 50)/2);
        show_message(msg_win, "Do you want to pick up this weapon?(click y/n)");
        wrefresh(msg_win);
        int choice = getch();
        if(choice == 'y'){
            if (player->Weapon_count < 10) {
                player->Weapon_list[0].numbers ++;
                return 1;
            } else{
                WINDOW *msg_win = newwin(3, 50, height - 3, width/2 + 2);
                show_message(msg_win, "You can't collect more than 10 weapons");
                wrefresh(msg_win);
                // مکث برای مشاهده پیام
                sleep(1);
                return 0;
            }            
        } else{
            return 0;
        }
    } else if(map[y][x] == 'E'){
        WINDOW *msg_win = newwin(3, 50, (height - 5)/2, (width - 50)/2);
        show_message(msg_win, "Do you want to pick up this weapon?(click y/n)");
        wrefresh(msg_win);
        int choice = getch();
        if(choice == 'y'){
            player->Weapon_list[1].numbers +=10;
            return 1;
        } else{
            return 0;
        }
    } else if(map[y][x] == 'W'){
        WINDOW *msg_win = newwin(3, 50, (height - 5)/2, (width - 50)/2);
        show_message(msg_win, "Do you want to pick up this weapon?(click y/n)");
        wrefresh(msg_win);
        int choice = getch();
        if(choice == 'y'){
            player->Weapon_list[2].numbers += 8;
            return 1;
        } else{
            return 0;
        }
    } else if(map[y][x] == 'N'){
        WINDOW *msg_win = newwin(3, 50, (height - 5)/2, (width - 50)/2);
        show_message(msg_win, "Do you want to pick up this weapon?(click y/n)");
        wrefresh(msg_win);
        int choice = getch();
        if(choice == 'y'){
            player->Weapon_list[3].numbers += 20;
            return 1;
        } else{
            return 0;
        }
    } else if(map[y][x] == 'L'){
        WINDOW *msg_win = newwin(3, 50, (height - 5)/2, (width - 50)/2);
        show_message(msg_win, "Do you want to pick up this weapon?(click y/n)");
        wrefresh(msg_win);
        int choice = getch();
        if(choice == 'y' && player->Weapon_list[4].numbers == 0){
            player->Weapon_list[4].numbers += 1;
            return 1;
        } else if(player->Weapon_list[4].numbers > 0){
            WINDOW *msg_win = newwin(3, 50, height - 3, width/2 + 2);
            show_message(msg_win, "You have already sword!");
            wrefresh(msg_win);
            // مکث برای مشاهده پیام
            sleep(1);
            return 0;
        } else return 0;
    } else if(map[y][x] == 'e'){
        WINDOW *msg_win = newwin(3, 50, (height - 5)/2, (width - 50)/2);
        show_message(msg_win, "Do you want to pick up this weapon?(click y/n)");
        wrefresh(msg_win);
        int choice = getch();
        if(choice == 'y'){
            player->Weapon_list[1].numbers ++;
            return 1;
        } else{
            return 0;
        }
    } else if(map[y][x] == 'w'){
        WINDOW *msg_win = newwin(3, 50, (height - 5)/2, (width - 50)/2);
        show_message(msg_win, "Do you want to pick up this weapon?(click y/n)");
        wrefresh(msg_win);
        int choice = getch();
        if(choice == 'y'){
            player->Weapon_list[2].numbers ++;
            return 1;
        } else{
            return 0;
        }
    } else if(map[y][x] == 'n'){
        WINDOW *msg_win = newwin(3, 50, (height - 5)/2, (width - 50)/2);
        show_message(msg_win, "Do you want to pick up this weapon?(click y/n)");
        wrefresh(msg_win);
        int choice = getch();
        if(choice == 'y'){
            player->Weapon_list[3].numbers ++;
            return 1;
        } else{
            return 0;
        }
    } else if(map[y][x] == 'l'){
        WINDOW *msg_win = newwin(3, 50, (height - 5)/2, (width - 50)/2);
        show_message(msg_win, "Do you want to pick up this weapon?(click y/n)");
        wrefresh(msg_win);
        int choice = getch();
        if(choice == 'y' && player->Weapon_list[4].numbers == 0){
            player->Weapon_list[4].numbers ++;
            return 1;
        } else if(player->Weapon_list[4].numbers > 0){
            WINDOW *msg_win = newwin(3, 50, height - 3, width/2 + 2);
            show_message(msg_win, "You have already sword!");
            wrefresh(msg_win);
            // مکث برای مشاهده پیام
            sleep(1);
            return 0;
        } else return 0;
    } else if(map[y][x] == 'H'){
        WINDOW *msg_win = newwin(3, 50, (height - 5)/2, (width - 50)/2);
        show_message(msg_win, "Do you want to pick up this talisman?(click y/n)");
        wrefresh(msg_win);
        int choice = getch();
        if(choice == 'y' || choice == 'Y'){
            talisman health = createTalisman(HEALTH);
            if (player->talisman_count < 10){
                player->talisman_list[player->talisman_count++] = health;
                return 1;
            } else{
                WINDOW *msg_win = newwin(3, 50, height - 3, width/2 + 2);
                show_message(msg_win, "You can't collect more than 10 talismans");
                wrefresh(msg_win);
                // مکث برای مشاهده پیام
                sleep(1);
                return 0;
            }
        } else return 0;
    } else if(map[y][x] == 's'){
        WINDOW *msg_win = newwin(3, 50, (height - 5)/2, (width - 50)/2);
        show_message(msg_win, "Do you want to pick up this talisman?(click y/n)");
        wrefresh(msg_win);
        int choice = getch();
        if(choice == 'y' || choice == 'Y'){
            talisman speed = createTalisman(SPEED);
            if (player->talisman_count < 10){
                player->talisman_list[player->talisman_count++] = speed;
                return 1;
            } else{
                WINDOW *msg_win = newwin(3, 50, height - 3, width/2 + 2);
                show_message(msg_win, "You can't collect more than 10 talismans");
                wrefresh(msg_win);
                // مکث برای مشاهده پیام
                sleep(1);
                return 0;
            }
        } else return 0;
    } else if(map[y][x] == 'd'){
        WINDOW *msg_win = newwin(3, 50, (height - 5)/2, (width - 50)/2);
        show_message(msg_win, "Do you want to pick up this talisman?(click y/n)");
        wrefresh(msg_win);
        int choice = getch();
        if(choice == 'y' || choice == 'Y'){
            talisman damage = createTalisman(DAMAGE);
            if (player->talisman_count < 10){
                player->talisman_list[player->talisman_count++] = damage;
                return 1;
            } else{
                WINDOW *msg_win = newwin(3, 50, height - 3, width/2 + 2);
                show_message(msg_win, "You can't collect more than 10 talismans");
                wrefresh(msg_win);
                // مکث برای مشاهده پیام
                sleep(1);
                return 0;
            }
        } else return 0;
    } else if (map[y][x] == '*') {
        // بررسی اتاق‌ها
        for (int i = 0; i < num_rooms; i++) {
            if (rooms[i].theme != 't') {  // فقط اتاق‌هایی که تم گنج ندارند
                bool active_monster_found = false;

                // بررسی وجود هیولای فعال در نقشه
                for (int row = rooms[i].start_y; row < rooms[i].start_y + rooms[i].height; row++) {
                    for (int col = rooms[i].start_x; col < rooms[i].start_x + rooms[i].width; col++) {
                        if (map[row][col] == 'D' || map[row][col] == 'F' || map[row][col] == 'G' || 
                            map[row][col] == 'S' || map[row][col] == 'U') {

                            active_monster_found = true;
                            break;
                        }
                    }
                    if (active_monster_found) break;  // اگر هیولای فعال پیدا شد، نیازی به بررسی بیشتر نیست
                }

                // اگر هیولای فعال پیدا شد، هشدار نمایش داده و تابع را متوقف کن
                if (active_monster_found) {
                    int height, width;
                    getmaxyx(stdscr, height, width);
                    WINDOW *msg_win = newwin(3, 65, (height - 5)/2, (width - 65)/2);
                    show_message(msg_win, "⚠️ There are still active monsters in non-treasure rooms! ⚠️");
                    wrefresh(msg_win);
                    usleep(1500000);  // نمایش پیام به مدت 2 ثانیه
                    return 0;
                }
            }
        }
        // اگر هیچ هیولای فعالی پیدا نشد
        return 1;
    }

    else return 0;
}

void move_fast(char **map, int width, int height, Player *player, int **map_visited, Room *rooms, int num_rooms, int display_completely, Food foods[7]) {
    // نمایش اولیه برای منتظر بودن جهت حرکت
    mvprintw(height + 3, width/2 + 2, "Press a direction key (arrow keys) after 'f' to move fast...");
    refresh();

    // دریافت کلید جهت پس از زدن 'f'
    int direction_key = getch();

    // تعیین جهت حرکت بر اساس کلید جهت
    int dx = 0, dy = 0;
    switch (direction_key) {
        case KEY_UP:
            dy = -1;
            break;
        case KEY_DOWN:
            dy = 1;
            break;
        case KEY_LEFT:
            dx = -1;
            break;
        case KEY_RIGHT:
            dx = 1;
            break;
        default:
            return; // اگر کلید معتبر نباشد، تابع را ترک می‌کنیم
    }

    // حرکت سریع در جهت مشخص‌شده
    while (1) {
        int new_y = player->y + dy;
        int new_x = player->x + dx;

        // بررسی محدودیت‌های نقشه و مانع‌ها
        if (new_y < 0 || new_y >= height || new_x < 0 || new_x >= width || !can_go(new_y, new_x, map, player, &map_visited, 0, foods, num_rooms, rooms)) {
            break;
        }

        player->y = new_y;
        player->x = new_x;

        // توقف در صورت برخورد به شیء خاص
        if (map[new_y][new_x] == 'g' || map[new_y][new_x] == 'h') {
            break;
        }
    }
}
int finished = 0;
void new_game() {
    finished = 0;
    // تعریف متغیرها برای نقشه بازی
    setlocale(LC_ALL, "");
    int width, height;
    getmaxyx(stdscr, height, width);
    int whole_height = height;
    height -= 5;
    Player player;
    player.color = hero_color;
    player.is_in_floor = 1;
    player.collected_golds = 0;
    player.score = 0;
    player.lives = 5;
    player.health = 100;
    player.health_increaser = 0;
    player.food_count = 0;
    player.hunger = 100;
    player.Weapon_count = 1;
    player.equipped_weapon = createWeapon(MACE);
    player.Weapon_list[0] = createWeapon(MACE);
    player.Weapon_list[1] = createWeapon(DAGGER);
    player.Weapon_list[2] = createWeapon(MAGIC_WAND);
    player.Weapon_list[3] = createWeapon(NORMAL_ARROW);
    player.Weapon_list[4] = createWeapon(SWORD);
    player.talisman_count = 0;
    talisman current_talisman = {0};  // تمام فیلدها صفر می‌شوند.
    player.current_talisman = current_talisman;
    player.speed = 1;
    player.health_speed = 1;
    player.power = 1;
    
    // پنجره منوی غذا
    WINDOW *menu_win = newwin(20, 45, 0, 0);
    
    int num_rooms;
    if(level_difficulty == 1){
        num_rooms = 6 + rand() % 2; // عداد اتاق‌ها بین 6 تا 7
    } else if(level_difficulty == 2){
        num_rooms = 7 + rand() % 2; // عداد اتاق‌ها بین 7 تا 8
    } else if(level_difficulty == 3){
        num_rooms = 8 + rand() % 2; // عداد اتاق‌ها بین 8 تا 9
    }
    Room *rooms = (Room *)malloc(num_rooms * sizeof(Room));
    Food foods[7];
    int num_monster;
    if(level_difficulty == 1){
        num_monster = rand() % 6;
    } else if(level_difficulty == 2){
        num_monster = 2 + rand() % 6;
    } else if(level_difficulty == 3){
        num_monster = 4 + rand() % 6;
    }
    Monster monsters[num_monster];
    // ساخت نقشه بازی
    char **map = create_map(width, height, level_difficulty, &player, rooms, num_rooms, foods, num_monster, monsters);

    //مکان هایی از نقشه که دیده شدند
    int **map_visited = (int **)malloc(height * sizeof(int *));
    for (int i = 0; i < height; i++) {
        map_visited[i] = (int *)malloc(width * sizeof(int));
    }
    for (int i = 0; i < width ; i++){
        for(int j = 0; j < height; j++){
            map_visited[j][i] = 0;
        }
    }
    //اتاق اول دیده شه
    rooms[0].visited = 1;

    // افزودن بازیکن در موقعیت تصادفی
    player.x = rooms[0].start_x + rooms[0].width / 2 ;
    player.y = rooms[0].start_y + rooms[0].height  -1 - rand() % 5;
    while (!is_valid_position(map, player.x, player.y)){ // پیدا کردن موقعیت خالی
        player.y = rooms[0].start_y + rooms[0].height  -1 - rand() % 5;
    }
    LastShot last_shot = {0};
    int display_completely = 0;
    bool game_running = true;
    int g_clicked = 0;
    time_t last_update = time(NULL);   
    time_t last_update2 = time(NULL);

    // حلقه اصلی بازی
    while (game_running) {
        player.lives = player.health/20;
        if(player.health > 0 && player.health < 20){
            player.lives = 1;
        }
        //مدیریت زمان اثر طلسم
        if(player.current_talisman.active){
            player.current_talisman.lifetime++;
                if(player.current_talisman.lifetime >= 10){
                    switch (player.current_talisman.type) {
                    case HEALTH:
                        player.health_speed = 1;
                        break;
                    case SPEED:
                        player.speed = 1;
                        break;
                    case DAMAGE:
                        player.power = 1;
                        break;
                    }
                    player.current_talisman = current_talisman;
                }
        }
        player.health_increaser += player.health_speed;
        if(player.health_increaser >= 30 && player.health < 100 && player.hunger == 100){
            player.health++;
            player.health_increaser = 0;
        }
        if(player.health >= 100){
            player.health = 100;
        } 
        if(player.hunger >= 100){
            player.hunger = 100;
        }
        // بررسی زمان برای کاهش گرسنگی
        if (difftime(time(NULL), last_update) >= HUNGER_DECREASE_INTERVAL) {
            update_hunger(&player);
            last_update = time(NULL);
        }
        // پیدا کردن اتاقی که بازیکن در آن قرار دارد
        int current_room = 0; 
        for (int r = 0; r < num_rooms; r++){
            if (player.x >= rooms[r].start_x && player.x < rooms[r].start_x + rooms[r].width &&
                player.y >= rooms[r].start_y && player.y < rooms[r].start_y + rooms[r].height) {
                current_room = r;
                break;
            }
        }
        if(rooms[current_room].theme == 'e'){
            if (difftime(time(NULL), last_update2) >= 5) {
                player.health--;
                last_update2 = time(NULL);
            }
        }

        int countMonster = 0;
        Room current = rooms[current_room];

        // if(player.is_in_floor > 4){
        //     WINDOW *msg_win = newwin(10, 50, height/2, (width - 50)/2);
        //     wclear(msg_win); // پاک کردن محتوای قبلی پنجره
        //     box(msg_win, 0, 0); // افزودن کادر دور پنجره
        //     mvwprintw(msg_win, 1, 21, "You win!"); // نوشتن پیام جدید
        //     mvwprintw(msg_win, 3, 12, "You collected %d golds.",player.collected_golds); // نوشتن پیام جدید
        //     mvwprintw(msg_win, 5, 12, "Your Score is %d.", player.score); // نوشتن پیام جدید
        //     wrefresh(msg_win); // به‌روزرسانی پنجره
        //     sleep(5);
        //     break;
        // }

        // بررسی پایان بازی
        if (player.health <= 0) {
            display_game_over_message(player.score);
            finished = 1;
            break;
        }
        if (player.is_in_floor == 4 && all_monsters_defeated(monsters, num_monster) && rooms[current_room].theme == 't') {
            player.score += player.health;
            display_victory_message(player.score);
            finished = 1;
            // بازگشت به منوی اصلی یا پایان بازی
            break;  // حلقه بازی را متوقف کن
        }


        // ذخیره وضعیت خانه قبلی
        char previous_cell = map[player.y][player.x];
        if(!g_clicked){
            map_visited[player.y][player.x] = 1;   
        }

        if(previous_cell == '<'){
            map = create_map(width, height, level_difficulty, &player, rooms, num_rooms, foods, num_monster, monsters);
            for (int i = 0; i < width ; i++){
                for(int j = 0; j < height; j++){
                    map_visited[j][i] = 0;
                }
            }
            //اتاق اول دیده شه
            rooms[current_room].visited = 1;
            rooms[0].visited = 0;
            previous_cell = '.';
        }
        // رسم بازیکن روی نقشه
        map[player.y][player.x] = '@';

        //حرکت دادن هیولاهای توی اتاق
        for (int i = 0; i < num_monster; i++) {
            if (monsters[i].x > current.start_x && monsters[i].x < current.start_x + current.width &&
                monsters[i].y > current.start_y && monsters[i].y < current.start_y + current.height && monsters[i].active == true) {
                if(monsters[i].health == 0){
                    map[monsters[i].y][monsters[i].x] = '.';
                    monsters[i].active = false;
                    mvprintw(whole_height - 2, width/2 + 2, "You killed the enemy %s!", monsters[i].name);
                    player.score += 100;
                    refresh();
                    usleep(1500000);
                } else{
                    map[monsters[i].y][monsters[i].x] = '.';
                    moveMonster(&monsters[i], player , map);
                    map[monsters[i].y][monsters[i].x] = monsters[i].symbol;
                    int hit = rand() % 3;
                    // بررسی برخورد بازیکن با هیولا (اگر بازیکن کنار هیولا باشد)
                    if(hit){
                        if ((player.x == monsters[i].x && (player.y == monsters[i].y - 1 || player.y == monsters[i].y + 1)) || 
                            (player.y == monsters[i].y && (player.x == monsters[i].x - 1 || player.x == monsters[i].x + 1))) {
                            
                            player.health -= 3;  // کم کردن 3 واحد از سلامت بازیکن
                            
                            // چاپ پیام حمله هیولا
                            mvprintw(whole_height - 3, width / 2 + 2, "The enemy %s attacked you! -3 HP", monsters[i].name);
                            
                            refresh();
                            usleep(1000000);  // مکث 1 ثانیه‌ای برای نمایش پیام
                        }
                    }
                }
            }
        }
        // نمایش نقشه
        clear();
        if(display_completely){
            display_whole_map(map, width, height, player, rooms, num_rooms, map_visited);
        } else{
            display_map(map, width, height, player, rooms, num_rooms, map_visited);
        }
        for (int i = 0; i < width; i++) {
            mvaddch(whole_height - 5, i, '-'); // در ردیف 0، کاراکتر "-" را رسم می‌کنیم
        }
        for (int i = 0; i < 5; i++) {
            mvaddch(whole_height - i, width/2, '|'); // در ردیف 0، کاراکتر "-" را رسم می‌کنیم
        }
        attron(A_BOLD);
        mvprintw(whole_height - 4, 5, "Lives: %d", player.lives);
        mvprintw(whole_height - 3, 5, "Floor: %d", player.is_in_floor);
        mvprintw(whole_height - 2, 5, "Golds: %d", player.collected_golds);
        mvprintw(whole_height - 1, 5, "Score: %d", player.score);
        //mvprintw(whole_height - 4, width/6, "Health: %d%%", player.health);
        display_health_bar_ncurses(player.health, 100, whole_height - 4 , width/6);
        // mvprintw(whole_height - 2, width/6, "hunger: %d%%", player.hunger);
        display_hunger_bar_ncurses(player.hunger, 100, whole_height - 3 , width/6);
        display_weapons(player, whole_height, width);
        mvprintw(whole_height - 1, width/6, "Equiped weapon: %s  ", player.equipped_weapon.name);  
        attron(COLOR_PAIR(player.equipped_weapon.color_pair));
        mvprintw(whole_height - 1, width/6 + strlen(player.equipped_weapon.name) + 18, "%s",player.equipped_weapon.symbol);         
        attroff(COLOR_PAIR(player.equipped_weapon.color_pair));
        attroff(A_BOLD);
        mvprintw(whole_height - 4, width/2 + 2, "Press q/Esc to exit the game (note:game will be saved).");        
        refresh();
        // دریافت ورودی از کاربر
        int ch = getch();

        // پاک کردن موقعیت قبلی بازیکن از نقشه
        map[player.y][player.x] = previous_cell; // بازگرداندن خانه قبلی به وضعیت قبلی
        if(previous_cell == 'g' && g_clicked == 0){
            WINDOW *msg_win = newwin(3, 25, whole_height - 3, width/2 + 2);
            show_message(msg_win, "You collected 1 gold!");
            wrefresh(msg_win);
            map[player.y][player.x] = '.';
            player.score += 10;
            // مکث برای مشاهده پیام
            sleep(1);
        } else if(previous_cell == 'b' && g_clicked == 0){
            WINDOW *msg_win = newwin(3, 30, whole_height - 3, width/2 + 2);
            show_message(msg_win, "You collected 1 black gold!");
            wrefresh(msg_win);
            player.score += 20;
            // مکث برای مشاهده پیام
            sleep(1);
            map[player.y][player.x] = '.';
        } else if(previous_cell == '*' && g_clicked == 0){
            WINDOW *msg_win = newwin(3, 30, whole_height - 3, width/2 + 2);
            show_message(msg_win, "You found the treasure!");
            wrefresh(msg_win);
            // مکث برای مشاهده پیام
            sleep(1);
            map[player.y][player.x] = '.';
            int index_treasure = 0;
            for (int r = 0; r < num_rooms; r++) {
                if (rooms[r].theme == 't') {
                    index_treasure = r;
                    break;
                }
            }
            player.x = rooms[index_treasure].start_x + rooms[index_treasure].width / 2 ;
            player.y = rooms[index_treasure].start_y + rooms[index_treasure].height  -1 - rand() % 5;
            while (!is_valid_position(map, player.x, player.y)){ // پیدا کردن موقعیت خالی
                player.y = rooms[index_treasure].start_y + rooms[index_treasure].height  -1 - rand() % 5;
            }
                    
        }
        if((previous_cell != '<' && previous_cell != 'Y' && previous_cell != '#' && previous_cell != '+' && previous_cell != 'v') && g_clicked == 0){
            map[player.y][player.x] = '.';
        }
        g_clicked = 0;
        // مدیریت ورودی‌ها
        switch (ch) {
            case KEY_UP:
                if (player.y >= player.speed && can_go(player.y-player.speed , player.x, map, &player, &map_visited, g_clicked, foods, num_rooms, rooms)){
                    player.y-= player.speed;
                    player.direction[0] = 'y';
                    player.direction[1] = '-';
                }
                    
                break;
            case KEY_DOWN:
                if (player.y < height - player.speed && can_go(player.y + player.speed , player.x, map, &player, &map_visited, g_clicked, foods, num_rooms, rooms)){
                    player.y += player.speed;
                    player.direction[0] = 'y';
                    player.direction[1] = '+';
                }
                    
                break;
            case KEY_LEFT:
                if (player.x >= player.speed && can_go(player.y , player.x - player.speed, map, &player, &map_visited, g_clicked, foods, num_rooms, rooms)){
                    player.x -= player.speed;
                    player.direction[0] = 'x';
                    player.direction[1] = '-';
                }
                   
                break;
            case KEY_RIGHT:
                if (player.x < width - player.speed && can_go(player.y , player.x + player.speed, map, &player, &map_visited, g_clicked, foods, num_rooms, rooms)){
                    player.x += player.speed;
                    player.direction[0] = 'x';
                    player.direction[1] = '+';
                }
                break;
            case KEY_PPAGE:
                if (player.y >= player.speed && player.x < width - player.speed && can_go(player.y-player.speed , player.x + player.speed, map, &player, &map_visited, g_clicked, foods, num_rooms, rooms)){
                    player.x += player.speed;
                    player.y-= player.speed;
                    player.direction[0] = 'y';
                    player.direction[1] = '-';
                }
                break;
            case KEY_NPAGE:
                if (player.y < height - player.speed && player.x < width - player.speed && can_go(player.y + player.speed , player.x + player.speed, map, &player, &map_visited, g_clicked, foods, num_rooms, rooms)){
                    player.x += player.speed;
                    player.y += player.speed;
                    player.direction[0] = 'y';
                    player.direction[1] = '+';
                }
                break;
            case KEY_HOME:
                if (player.y >= player.speed && player.x >= player.speed && can_go(player.y-player.speed , player.x - player.speed, map, &player, &map_visited, g_clicked, foods, num_rooms, rooms)){
                    player.x -= player.speed;
                    player.y-= player.speed;
                    player.direction[0] = 'y';
                    player.direction[1] = '-';
                }
                break;
            case KEY_END:
                if (player.y < height - player.speed && player.x >= player.speed && can_go(player.y + player.speed , player.x - player.speed, map, &player, &map_visited, g_clicked, foods, num_rooms, rooms)){
                    player.x -= player.speed;
                    player.y += player.speed;
                    player.direction[0] = 'y';
                    player.direction[1] = '+';
                }
                break;
            case 'M': 
            case 'm':
                if(display_completely){
                    display_completely = 0;
                } else{
                    display_completely = 1;
                }
                break;
            case 'F':
            case 'f': // حرکت سریع پس از دریافت جهت
                move_fast(map, width, height, &player, map_visited, rooms, num_rooms, display_completely, foods);
                break;
            case 'G':
            case 'g':
                g_clicked = 1;
                int ch2 = getch();
                switch (ch2) {
                    case KEY_UP:
                        if (player.y >= player.speed && can_go(player.y-player.speed , player.x, map, &player, &map_visited, g_clicked, foods, num_rooms, rooms)){
                            player.y-= player.speed;
                            player.direction[0] = 'y';
                            player.direction[1] = '-';
                        }
                            
                        break;
                    case KEY_DOWN:
                        if (player.y < height - player.speed && can_go(player.y + player.speed , player.x, map, &player, &map_visited, g_clicked, foods, num_rooms, rooms)){
                            player.y += player.speed;
                            player.direction[0] = 'y';
                            player.direction[1] = '+';
                        }
                            
                        break;
                    case KEY_LEFT:
                        if (player.x >= player.speed && can_go(player.y , player.x - player.speed, map, &player, &map_visited, g_clicked, foods, num_rooms, rooms)){
                            player.x -= player.speed;
                            player.direction[0] = 'x';
                            player.direction[1] = '-';
                        }
                        
                        break;
                    case KEY_RIGHT:
                        if (player.x < width - player.speed && can_go(player.y , player.x + player.speed, map, &player, &map_visited, g_clicked, foods, num_rooms, rooms)){
                            player.x += player.speed;
                            player.direction[0] = 'x';
                            player.direction[1] = '+';
                        }
                        break;
                    case KEY_PPAGE:
                        if (player.y >= player.speed && player.x < width - player.speed && can_go(player.y-player.speed , player.x + player.speed, map, &player, &map_visited, g_clicked, foods, num_rooms, rooms)){
                            player.x += player.speed;
                            player.y-= player.speed;
                            player.direction[0] = 'y';
                            player.direction[1] = '-';
                        }
                        break;
                    case KEY_NPAGE:
                        if (player.y < height - player.speed && player.x < width - player.speed && can_go(player.y + player.speed , player.x + player.speed, map, &player, &map_visited, g_clicked, foods, num_rooms, rooms)){
                            player.x += player.speed;
                            player.y += player.speed;
                            player.direction[0] = 'y';
                            player.direction[1] = '+';
                        }
                        break;
                    case KEY_HOME:
                        if (player.y >= player.speed && player.x >= player.speed && can_go(player.y-player.speed , player.x - player.speed, map, &player, &map_visited, g_clicked, foods, num_rooms, rooms)){
                            player.x -= player.speed;
                            player.y-= player.speed;
                            player.direction[0] = 'y';
                            player.direction[1] = '-';
                        }
                        break;
                    case KEY_END:
                        if (player.y < height - player.speed && player.x >= player.speed && can_go(player.y + player.speed , player.x - player.speed, map, &player, &map_visited, g_clicked, foods, num_rooms, rooms)){
                            player.x -= player.speed;
                            player.y += player.speed;
                            player.direction[0] = 'y';
                            player.direction[1] = '+';
                        }
                        break;
                }
                break;
            case 'E':
            case 'e':
                werase(menu_win); // پاک کردن محتوای قبلی
                box(menu_win, 0, 0);
                consume_food_ncurses(&player, menu_win);
                break;
            case 'i':
            case 'I':
                werase(menu_win); // پاک کردن محتوای قبلی
                box(menu_win, 0, 0);
                equip_weapon_ncurses(&player, menu_win);
                break;
            case 't':
            case 'T':
                werase(menu_win); // پاک کردن محتوای قبلی
                box(menu_win, 0, 0);
                talisman_menu(&player, menu_win);
                break;
            case ' ':
                if (player.equipped_weapon.numbers <= 0 && player.equipped_weapon.is_consumable) {
                    mvprintw(whole_height - 2, width/2 + 2, "Your weapon is over!");
                    refresh();
                    sleep(1);
                    break;
                }
                if (player.equipped_weapon.is_melee) { 
                    // ضربه به ۸ جهت
                    int dirs[8][2] = {{-1, -1}, {-1, 0}, {-1, 1}, {0, -1}, 
                                    {0, 1}, {1, -1}, {1, 0}, {1, 1}};
                    for (int i = 0; i < 8; i++) {
                        int new_x = player.x + dirs[i][0];
                        int new_y = player.y + dirs[i][1];
                        Monster *enemy = getMonsterAtPosition(monsters, num_monster, new_x, new_y);
                        if (enemy) {
                            enemy->health -= player.equipped_weapon.damage;
                            if(enemy->health <= 0){
                                enemy->health = 0;
                            }
                            else mvprintw(whole_height - 2, width/2 + 2, "You hit %s, remaining health: %d", enemy->name, enemy->health);
                            refresh();
                            sleep(1);
                        }
                    }
                } else {
                    // دریافت جهت پرتاب
                    int dir = getch();
                    int dx = 0, dy = 0;
                    if (dir == KEY_UP) dy = -1;
                    else if (dir == KEY_DOWN) dy = 1;
                    else if (dir == KEY_LEFT) dx = -1;
                    else if (dir == KEY_RIGHT) dx = 1;

                    int x = player.x, y = player.y;
                    int max_range = player.equipped_weapon.range; // حداکثر برد تیر
                    for (int i = 0; i < max_range; i++) {
                        x += dx;
                        y += dy;
                        if(!(y <= height && y >= 0 && x >= 0 && x <= width)){
                            break;
                        }
                        if (map[y][x] == '|' || map[y][x] == '_' || map[y][x] == 'o' || map[y][x] == '+') {
                            bool placed = false;
                            while(x != player.x || y != player.y) {
                                x -= dx;
                                y -= dy;
                                // بررسی اینکه موقعیت جدید داخل نقشه است و خالی است
                                if (y >= 0 && y < height && x >= 0 && x < width && map[y][x] == '.') {
                                    map[y][x] = player.equipped_weapon.symbol_on_map;
                                    placed = true;
                                    break;  // وقتی تیر گذاشته شد، از حلقه خارج شو
                                }
                            }

                            mvprintw(whole_height - 2, width / 2 + 2, "The projectile hit the wall");
                            refresh();
                            sleep(1);
                            break;
                        }
                        Monster *enemy = getMonsterAtPosition(monsters, num_monster, x, y);
                        if (enemy) {
                            enemy->health -= player.equipped_weapon.damage;
                            if(enemy->health <= 0){
                                enemy->health = 0;
                            } 
                            else mvprintw(whole_height - 2, width/2 + 2, "The projectile hit %s, remaining Health: %d", enemy->name, enemy->health);
                            refresh();
                            sleep(1);
                        }
                    }

                    if (player.equipped_weapon.is_consumable) {
                        player.equipped_weapon.numbers--;
                    }

                    // ذخیره آخرین شلیک
                    last_shot.dx = dx;
                    last_shot.dy = dy;
                    last_shot.weapon_used = player.equipped_weapon;
                }
                break;
            case 'q': // خروج از بازی
                game_running = false; // پایان حلقه
                break;
            case 27: // خروج از بازی
                game_running = false; // پایان حلقه
                break;
        }
    }

    if(have_account){
        user1->points = player.score;
        user1->golds = player.collected_golds;
        user1->times_played = 1;
        save_user(user1);
    }

   save_game_to_binary_file(map, height, width,rooms, num_rooms, &player, map_visited, foods, num_monster ,monsters);


    // آزاد کردن حافظه نقشه
    for (int i = 0; i < height; i++) {
        free(map[i]);
    }
    free(map);
    free(rooms);
}

void continue_game() {
    if(finished){
        return;
    }
    // تعریف متغیرها برای نقشه بازی
    setlocale(LC_ALL, "");
    int width, height, num_rooms;    
    getmaxyx(stdscr, height, width);
    int whole_height = height;
    height -= 5;
    char **map;
    int **map_visited;
    Room *rooms;
    Player player;
    Food foods[7];
    int num_monster;
    Monster *monsters;
    talisman current_talisman = {0};

    // بارگذاری اطلاعات بازی
    load_game_from_binary_file(&map, &height, &width, &rooms, &num_rooms, &player, &map_visited, &foods, &num_monster, &monsters);

    // تنظیم رنگ بازیکن
    player.color = hero_color;

    int display_completely = 0;
    // متغیر برای وضعیت بازی
    LastShot last_shot = {0};
    bool game_running = true;
    int g_clicked = 0;
    time_t last_update = time(NULL);  
    WINDOW *menu_win = newwin(20, 45, 0, 0);
    
    // حلقه اصلی بازی
    while (game_running) {
        player.lives = player.health/20;
        if(player.health > 0 && player.health < 20){
            player.lives = 1;
        }
        //مدیریت زمان اثر طلسم
        if(player.current_talisman.active){
            player.current_talisman.lifetime++;
                if(player.current_talisman.lifetime >= 10){
                    switch (player.current_talisman.type) {
                    case HEALTH:
                        player.health_speed = 1;
                        break;
                    case SPEED:
                        player.speed = 1;
                        break;
                    case DAMAGE:
                        player.power = 1;
                        break;
                    }
                    player.current_talisman = current_talisman;
                }
        }
        player.health_increaser += player.health_speed;
        if(player.health_increaser >= 30 && player.health < 100 && player.hunger == 100){
            player.health++;
            player.health_increaser = 0;
        }
        if(player.health >= 100){
            player.health = 100;
        } 
        if(player.hunger >= 100){
            player.hunger = 100;
        }
        // بررسی زمان برای کاهش گرسنگی
        if (difftime(time(NULL), last_update) >= HUNGER_DECREASE_INTERVAL) {
            update_hunger(&player);
            last_update = time(NULL);
        }
        // پیدا کردن اتاقی که بازیکن در آن قرار دارد
        int current_room = 0; 
        for (int r = 0; r < num_rooms; r++){
            if (player.x >= rooms[r].start_x && player.x < rooms[r].start_x + rooms[r].width &&
                player.y >= rooms[r].start_y && player.y < rooms[r].start_y + rooms[r].height) {
                current_room = r;
                break;
            }
        }

        int countMonster = 0;
        Room current = rooms[current_room];

        // if(player.is_in_floor > 4){
        //     WINDOW *msg_win = newwin(10, 50, height/2, (width - 50)/2);
        //     wclear(msg_win); // پاک کردن محتوای قبلی پنجره
        //     box(msg_win, 0, 0); // افزودن کادر دور پنجره
        //     mvwprintw(msg_win, 1, 21, "You win!"); // نوشتن پیام جدید
        //     mvwprintw(msg_win, 3, 12, "You collected %d golds.",player.collected_golds); // نوشتن پیام جدید
        //     mvwprintw(msg_win, 5, 12, "Your Score is %d.", player.score); // نوشتن پیام جدید
        //     wrefresh(msg_win); // به‌روزرسانی پنجره
        //     sleep(5);
        //     break;
        // }

        // بررسی پایان بازی
        if (player.health <= 0) {
            display_game_over_message(player.score);
            finished = 1;
            break;
        }
        if (player.is_in_floor == 4 && all_monsters_defeated(monsters, num_monster) && rooms[current_room].theme == 't') {
            player.score += player.health + 1000;
            display_victory_message(player.score);
            finished = 1;
            // بازگشت به منوی اصلی یا پایان بازی
            break;  // حلقه بازی را متوقف کن
        }


        // ذخیره وضعیت خانه قبلی
        char previous_cell = map[player.y][player.x];
        if(!g_clicked){
            map_visited[player.y][player.x] = 1;   
        }

        if(previous_cell == '<'){
            map = create_map(width, height, level_difficulty, &player, rooms, num_rooms, foods, num_monster, monsters);
            previous_cell = '.';
        }
        // رسم بازیکن روی نقشه
        map[player.y][player.x] = '@';

        //حرکت دادن هیولاهای توی اتاق
        for (int i = 0; i < num_monster; i++) {
            if (monsters[i].x > current.start_x && monsters[i].x < current.start_x + current.width &&
                monsters[i].y > current.start_y && monsters[i].y < current.start_y + current.height && monsters[i].active == true) {
                if(monsters[i].health == 0){
                    map[monsters[i].y][monsters[i].x] = '.';
                    monsters[i].active = false;
                    mvprintw(whole_height - 2, width/2 + 2, "You killed the enemy  %s", monsters[i].name);
                    player.score += 100;
                    refresh();
                    usleep(1500000);
                } else{
                    map[monsters[i].y][monsters[i].x] = '.';
                    moveMonster(&monsters[i], player , map);
                    map[monsters[i].y][monsters[i].x] = monsters[i].symbol;
                    int hit = rand() % 3;
                    // بررسی برخورد بازیکن با هیولا (اگر بازیکن کنار هیولا باشد)
                    if(hit){
                        if ((player.x == monsters[i].x && (player.y == monsters[i].y - 1 || player.y == monsters[i].y + 1)) || 
                            (player.y == monsters[i].y && (player.x == monsters[i].x - 1 || player.x == monsters[i].x + 1))) {
                            
                            player.health -= 3;  // کم کردن 3 واحد از سلامت بازیکن
                            
                            // چاپ پیام حمله هیولا
                            mvprintw(whole_height - 3, width / 2 + 2, "The enemy %s attacked you! -3 HP", monsters[i].name);
                            
                            refresh();
                            usleep(1000000);  // مکث 1 ثانیه‌ای برای نمایش پیام
                        }
                    }
                }
            }
        }
        // نمایش نقشه
        clear();
        if(display_completely){
            display_whole_map(map, width, height, player, rooms, num_rooms, map_visited);
        } else{
            display_map(map, width, height, player, rooms, num_rooms, map_visited);
        }
        for (int i = 0; i < width; i++) {
            mvaddch(whole_height - 5, i, '-'); // در ردیف 0، کاراکتر "-" را رسم می‌کنیم
        }
        for (int i = 0; i < 5; i++) {
            mvaddch(whole_height - i, width/2, '|'); // در ردیف 0، کاراکتر "-" را رسم می‌کنیم
        }
        attron(A_BOLD);
        mvprintw(whole_height - 4, 5, "Lives: %d", player.lives);
        mvprintw(whole_height - 3, 5, "Floor: %d", player.is_in_floor);
        mvprintw(whole_height - 2, 5, "Golds: %d", player.collected_golds);
        mvprintw(whole_height - 1, 5, "Score: %d", player.score);
        //mvprintw(whole_height - 4, width/6, "Health: %d%%", player.health);
        display_health_bar_ncurses(player.health, 100, whole_height - 4 , width/6);
        // mvprintw(whole_height - 2, width/6, "hunger: %d%%", player.hunger);
        display_hunger_bar_ncurses(player.hunger, 100, whole_height - 3 , width/6);
        display_weapons(player, whole_height, width);
        mvprintw(whole_height - 1, width/6, "Equiped weapon: %s  ", player.equipped_weapon.name);  
        attron(COLOR_PAIR(player.equipped_weapon.color_pair));
        mvprintw(whole_height - 1, width/6 + strlen(player.equipped_weapon.name) + 18, "%s",player.equipped_weapon.symbol);         
        attroff(COLOR_PAIR(player.equipped_weapon.color_pair));
        attroff(A_BOLD);
        mvprintw(whole_height - 4, width/2 + 2, "Press q/Esc to exit the game (note:game will be saved).");        
        refresh();
        // دریافت ورودی از کاربر
        int ch = getch();

        // پاک کردن موقعیت قبلی بازیکن از نقشه
        map[player.y][player.x] = previous_cell; // بازگرداندن خانه قبلی به وضعیت قبلی
        if(previous_cell == 'g' && g_clicked == 0){
            WINDOW *msg_win = newwin(3, 25, whole_height - 3, width/2 + 2);
            show_message(msg_win, "You collected 1 gold!");
            wrefresh(msg_win);
            map[player.y][player.x] = '.';
            player.score += 10;
            // مکث برای مشاهده پیام
            sleep(1);
        } else if(previous_cell == 'b' && g_clicked == 0){
            WINDOW *msg_win = newwin(3, 30, whole_height - 3, width/2 + 2);
            show_message(msg_win, "You collected 1 black gold!");
            wrefresh(msg_win);
            player.score += 20;
            // مکث برای مشاهده پیام
            sleep(1);
            map[player.y][player.x] = '.';
        } else if(previous_cell == '*' && g_clicked == 0){
            WINDOW *msg_win = newwin(3, 30, whole_height - 3, width/2 + 2);
            show_message(msg_win, "You found the treasure!");
            wrefresh(msg_win);
            // مکث برای مشاهده پیام
            sleep(1);
            map[player.y][player.x] = '.';
            int index_treasure = 0;
            for (int r = 0; r < num_rooms; r++) {
                if (rooms[r].theme == 't') {
                    index_treasure = r;
                    break;
                }
            }
            player.x = rooms[index_treasure].start_x + rooms[index_treasure].width / 2 ;
            player.y = rooms[index_treasure].start_y + rooms[index_treasure].height  -1 - rand() % 5;
            while (!is_valid_position(map, player.x, player.y)){ // پیدا کردن موقعیت خالی
                player.y = rooms[index_treasure].start_y + rooms[index_treasure].height  -1 - rand() % 5;
            }
                    
        }
        if((previous_cell != '<' && previous_cell != 'Y' && previous_cell != '#' && previous_cell != '+' && previous_cell != 'v') && g_clicked == 0){
            map[player.y][player.x] = '.';
        }
        g_clicked = 0;
        // مدیریت ورودی‌ها
        switch (ch) {
            case KEY_UP:
                if (player.y >= player.speed && can_go(player.y-player.speed , player.x, map, &player, &map_visited, g_clicked, foods, num_rooms, rooms)){
                    player.y-= player.speed;
                    player.direction[0] = 'y';
                    player.direction[1] = '-';
                }
                    
                break;
            case KEY_DOWN:
                if (player.y < height - player.speed && can_go(player.y + player.speed , player.x, map, &player, &map_visited, g_clicked, foods, num_rooms, rooms)){
                    player.y += player.speed;
                    player.direction[0] = 'y';
                    player.direction[1] = '+';
                }
                    
                break;
            case KEY_LEFT:
                if (player.x >= player.speed && can_go(player.y , player.x - player.speed, map, &player, &map_visited, g_clicked, foods, num_rooms, rooms)){
                    player.x -= player.speed;
                    player.direction[0] = 'x';
                    player.direction[1] = '-';
                }
                   
                break;
            case KEY_RIGHT:
                if (player.x < width - player.speed && can_go(player.y , player.x + player.speed, map, &player, &map_visited, g_clicked, foods, num_rooms, rooms)){
                    player.x += player.speed;
                    player.direction[0] = 'x';
                    player.direction[1] = '+';
                }
                break;
            case KEY_PPAGE:
                if (player.y >= player.speed && player.x < width - player.speed && can_go(player.y-player.speed , player.x + player.speed, map, &player, &map_visited, g_clicked, foods, num_rooms, rooms)){
                    player.x += player.speed;
                    player.y-= player.speed;
                    player.direction[0] = 'y';
                    player.direction[1] = '-';
                }
                break;
            case KEY_NPAGE:
                if (player.y < height - player.speed && player.x < width - player.speed && can_go(player.y + player.speed , player.x + player.speed, map, &player, &map_visited, g_clicked, foods, num_rooms, rooms)){
                    player.x += player.speed;
                    player.y += player.speed;
                    player.direction[0] = 'y';
                    player.direction[1] = '+';
                }
                break;
            case KEY_HOME:
                if (player.y >= player.speed && player.x >= player.speed && can_go(player.y-player.speed , player.x - player.speed, map, &player, &map_visited, g_clicked, foods, num_rooms, rooms)){
                    player.x -= player.speed;
                    player.y-= player.speed;
                    player.direction[0] = 'y';
                    player.direction[1] = '-';
                }
                break;
            case KEY_END:
                if (player.y < height - player.speed && player.x >= player.speed && can_go(player.y + player.speed , player.x - player.speed, map, &player, &map_visited, g_clicked, foods, num_rooms, rooms)){
                    player.x -= player.speed;
                    player.y += player.speed;
                    player.direction[0] = 'y';
                    player.direction[1] = '+';
                }
                break;
            case 'M': 
            case 'm':
                if(display_completely){
                    display_completely = 0;
                } else{
                    display_completely = 1;
                }
                break;
            case 'F':
            case 'f': // حرکت سریع پس از دریافت جهت
                move_fast(map, width, height, &player, map_visited, rooms, num_rooms, display_completely, foods);
                break;
            case 'G':
            case 'g':
                g_clicked = 1;
                int ch2 = getch();
                switch (ch2) {
                    case KEY_UP:
                        if (player.y >= player.speed && can_go(player.y-player.speed , player.x, map, &player, &map_visited, g_clicked, foods, num_rooms, rooms)){
                            player.y-= player.speed;
                            player.direction[0] = 'y';
                            player.direction[1] = '-';
                        }
                            
                        break;
                    case KEY_DOWN:
                        if (player.y < height - player.speed && can_go(player.y + player.speed , player.x, map, &player, &map_visited, g_clicked, foods, num_rooms, rooms)){
                            player.y += player.speed;
                            player.direction[0] = 'y';
                            player.direction[1] = '+';
                        }
                            
                        break;
                    case KEY_LEFT:
                        if (player.x >= player.speed && can_go(player.y , player.x - player.speed, map, &player, &map_visited, g_clicked, foods, num_rooms, rooms)){
                            player.x -= player.speed;
                            player.direction[0] = 'x';
                            player.direction[1] = '-';
                        }
                        
                        break;
                    case KEY_RIGHT:
                        if (player.x < width - player.speed && can_go(player.y , player.x + player.speed, map, &player, &map_visited, g_clicked, foods, num_rooms, rooms)){
                            player.x += player.speed;
                            player.direction[0] = 'x';
                            player.direction[1] = '+';
                        }
                        break;
                    case KEY_PPAGE:
                        if (player.y >= player.speed && player.x < width - player.speed && can_go(player.y-player.speed , player.x + player.speed, map, &player, &map_visited, g_clicked, foods, num_rooms, rooms)){
                            player.x += player.speed;
                            player.y-= player.speed;
                            player.direction[0] = 'y';
                            player.direction[1] = '-';
                        }
                        break;
                    case KEY_NPAGE:
                        if (player.y < height - player.speed && player.x < width - player.speed && can_go(player.y + player.speed , player.x + player.speed, map, &player, &map_visited, g_clicked, foods, num_rooms, rooms)){
                            player.x += player.speed;
                            player.y += player.speed;
                            player.direction[0] = 'y';
                            player.direction[1] = '+';
                        }
                        break;
                    case KEY_HOME:
                        if (player.y >= player.speed && player.x >= player.speed && can_go(player.y-player.speed , player.x - player.speed, map, &player, &map_visited, g_clicked, foods, num_rooms, rooms)){
                            player.x -= player.speed;
                            player.y-= player.speed;
                            player.direction[0] = 'y';
                            player.direction[1] = '-';
                        }
                        break;
                    case KEY_END:
                        if (player.y < height - player.speed && player.x >= player.speed && can_go(player.y + player.speed , player.x - player.speed, map, &player, &map_visited, g_clicked, foods, num_rooms, rooms)){
                            player.x -= player.speed;
                            player.y += player.speed;
                            player.direction[0] = 'y';
                            player.direction[1] = '+';
                        }
                        break;
                }
                break;
            case 'E':
            case 'e':
                werase(menu_win); // پاک کردن محتوای قبلی
                box(menu_win, 0, 0);
                consume_food_ncurses(&player, menu_win);
                break;
            case 'i':
            case 'I':
                werase(menu_win); // پاک کردن محتوای قبلی
                box(menu_win, 0, 0);
                equip_weapon_ncurses(&player, menu_win);
                break;
            case 't':
            case 'T':
                werase(menu_win); // پاک کردن محتوای قبلی
                box(menu_win, 0, 0);
                talisman_menu(&player, menu_win);
                break;
            case ' ':
                if (player.equipped_weapon.numbers <= 0 && player.equipped_weapon.is_consumable) {
                    mvprintw(whole_height - 2, width/2 + 2, "Your weapon is over!");
                    refresh();
                    sleep(1);
                    break;
                }
                if (player.equipped_weapon.is_melee) { 
                    // ضربه به ۸ جهت
                    int dirs[8][2] = {{-1, -1}, {-1, 0}, {-1, 1}, {0, -1}, 
                                    {0, 1}, {1, -1}, {1, 0}, {1, 1}};
                    for (int i = 0; i < 8; i++) {
                        int new_x = player.x + dirs[i][0];
                        int new_y = player.y + dirs[i][1];
                        Monster *enemy = getMonsterAtPosition(monsters, num_monster, new_x, new_y);
                        if (enemy) {
                            enemy->health -= player.equipped_weapon.damage;
                            if(enemy->health <= 0){
                                enemy->health = 0;
                            }
                            else mvprintw(whole_height - 2, width/2 + 2, "You hit %s, remaining health: %d", enemy->name, enemy->health);
                            refresh();
                            sleep(1);
                        }
                    }
                } else {
                    // دریافت جهت پرتاب
                    int dir = getch();
                    int dx = 0, dy = 0;
                    if (dir == KEY_UP) dy = -1;
                    else if (dir == KEY_DOWN) dy = 1;
                    else if (dir == KEY_LEFT) dx = -1;
                    else if (dir == KEY_RIGHT) dx = 1;

                    int x = player.x, y = player.y;
                    int max_range = player.equipped_weapon.range; // حداکثر برد تیر
                    for (int i = 0; i < max_range; i++) {
                        x += dx;
                        y += dy;
                        if(!(y <= height && y >= 0 && x >= 0 && x <= width)){
                            break;
                        }
                        if (map[y][x] == '|' || map[y][x] == '_' || map[y][x] == 'o' || map[y][x] == '+') {
                            bool placed = false;
                            while(x != player.x || y != player.y) {
                                x -= dx;
                                y -= dy;
                                // بررسی اینکه موقعیت جدید داخل نقشه است و خالی است
                                if (y >= 0 && y < height && x >= 0 && x < width && map[y][x] == '.') {
                                    map[y][x] = player.equipped_weapon.symbol_on_map;
                                    placed = true;
                                    break;  // وقتی تیر گذاشته شد، از حلقه خارج شو
                                }
                            }

                            mvprintw(whole_height - 2, width / 2 + 2, "The projectile hit the wall");
                            refresh();
                            sleep(1);
                            break;
                        }
                        Monster *enemy = getMonsterAtPosition(monsters, num_monster, x, y);
                        if (enemy) {
                            enemy->health -= player.equipped_weapon.damage;
                            if(enemy->health <= 0){
                                enemy->health = 0;
                            } 
                            else mvprintw(whole_height - 2, width/2 + 2, "The projectile hit %s, remaining Health: %d", enemy->name, enemy->health);
                            refresh();
                            sleep(1);
                        }
                    }

                    if (player.equipped_weapon.is_consumable) {
                        player.equipped_weapon.numbers--;
                    }

                    // ذخیره آخرین شلیک
                    last_shot.dx = dx;
                    last_shot.dy = dy;
                    last_shot.weapon_used = player.equipped_weapon;
                }
                break;
            case 'q': // خروج از بازی
                game_running = false; // پایان حلقه
                break;
            case 27: // خروج از بازی
                game_running = false; // پایان حلقه
                break;
        }
    }
    if(have_account){
        user1->points = player.score;
        user1->golds = player.collected_golds;
        user1->times_played = 1;
        save_user(user1);
    }
    save_game_to_binary_file(map, height, width,rooms, num_rooms, &player, map_visited, foods, num_monster, monsters);

    // آزاد کردن حافظه
    for (int i = 0; i < height; i++) {
        free(map[i]);
    }
    free(map);
    free(rooms);
}