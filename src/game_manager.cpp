#include "game_manager.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <algorithm> 
#include <cstdlib>

using namespace godot;

void GameManager::_bind_methods() {
    ClassDB::bind_method(D_METHOD("start_game"), &GameManager::start_game);
    ClassDB::bind_method(D_METHOD("_on_start_button_pressed"), &GameManager::_on_start_button_pressed);
    
    // РЕГИСТРАЦИЯ API ДЛЯ БОТА
    // Эти функции будут видны в GDScript! Вызывать так: $GameManager.api_bot_register_player("1234")
    ClassDB::bind_method(D_METHOD("api_bot_register_player", "tg_id"), &GameManager::api_bot_register_player);
    ClassDB::bind_method(D_METHOD("api_bot_receive_answer", "tg_id", "answer_index"), &GameManager::api_bot_receive_answer);
    ClassDB::bind_method(D_METHOD("api_bot_minigame_wires", "tg_id", "wire_index"), &GameManager::api_bot_minigame_wires);
    ClassDB::bind_method(D_METHOD("api_bot_minigame_cups", "tg_id", "cup_index"), &GameManager::api_bot_minigame_cups);
    ClassDB::bind_method(D_METHOD("api_bot_minigame_minefield", "tg_id", "path_index"), &GameManager::api_bot_minigame_minefield);
}

GameManager::GameManager() {
    current_q_idx = 0;
    time_left = 0.0;
    max_time_for_state = 1.0;
    is_timer_running = false;
    current_state = 3; // Старт с экрана лобби
    current_minigame_idx = 0;
    
    // ВНИМАНИЕ: Для тестов без бота оставь false.
    // Когда бот будет готов слать ID, поменяй на true!
    use_real_players = false; 
}

GameManager::~GameManager() {}

void GameManager::_ready() {
    load_questions_from_csv();
    load_profiles_from_csv();
    show_lobby(); 
}

void GameManager::_process(double delta) {
    if (!is_timer_running) return;

    time_left -= delta;

    TextureProgressBar* timer_bar = get_node<TextureProgressBar>("TriviaUI/TimerBar");
    if (timer_bar) {
        timer_bar->set_max(max_time_for_state);
        timer_bar->set_value(time_left);
    }

    // СИМУЛЯЦИЯ ИГРОКОВ (Если use_real_players = false)
    // За 0.2 сек до конца таймера боты делают случайный выбор
    if (!use_real_players && time_left > 0.0 && time_left < 0.2) {
        if (current_state == 0) {
            for (int i = 0; i < players_list.size(); i++) {
                if (players_list[i].is_alive && players_list[i].selected_answer == -1) {
                    api_bot_receive_answer(players_list[i].telegram_id, rand() % 4);
                }
            }
        } else if (current_state == 1) {
            for (int i = 0; i < current_losers_indices.size(); i++) {
                int idx = current_losers_indices[i];
                if (players_list[idx].minigame_choice == -1) {
                    int r_choice = 0;
                    if (current_minigame_idx == 0) r_choice = rand() % 2;
                    else if (current_minigame_idx == 1) r_choice = rand() % 3;
                    else r_choice = rand() % 2;

                    if (current_minigame_idx == 0) api_bot_minigame_wires(players_list[idx].telegram_id, r_choice);
                    else if (current_minigame_idx == 1) api_bot_minigame_cups(players_list[idx].telegram_id, r_choice);
                    else api_bot_minigame_minefield(players_list[idx].telegram_id, r_choice);
                }
            }
        }
    }

    // ОБРАБОТКА ОКОНЧАНИЯ ТАЙМЕРА (АВТОМАТИЧЕСКАЯ ПРОСТАНОВКА ИНДЕКСА 4 ДЛЯ ОПОЗДАВШИХ)
    if (time_left <= 0.0) {
        is_timer_running = false;
        
        if (current_state == 0) {
            for (int i = 0; i < players_list.size(); i++) {
                if (players_list[i].is_alive && players_list[i].selected_answer == -1) {
                    api_bot_receive_answer(players_list[i].telegram_id, 4); 
                }
            }
            end_round();
        } else if (current_state == 1) {
            for (int i = 0; i < current_losers_indices.size(); i++) {
                int idx = current_losers_indices[i];
                if (players_list[idx].minigame_choice == -1) {
                    if (current_minigame_idx == 0) api_bot_minigame_wires(players_list[idx].telegram_id, 4);
                    else if (current_minigame_idx == 1) api_bot_minigame_cups(players_list[idx].telegram_id, 4);
                    else api_bot_minigame_minefield(players_list[idx].telegram_id, 4);
                }
            }
            process_minigame();
        } else if (current_state == 2) {
            int total_alive = 0;
            for (int i = 0; i < players_list.size(); i++) {
                if (players_list[i].is_alive) total_alive++;
            }
            if (total_alive <= 1) show_winner();
            else next_round();
        }
    }
}

void GameManager::show_lobby() {
    current_state = 3; 
    is_timer_running = false;
    players_list.clear(); 

    CanvasItem* lobby_ui = get_node<CanvasItem>("LobbyUI");
    CanvasItem* trivia_ui = get_node<CanvasItem>("TriviaUI");
    CanvasItem* losers_ui = get_node<CanvasItem>("LosersUI");
    CanvasItem* winner_ui = get_node<CanvasItem>("WinnerUI");

    if (lobby_ui) lobby_ui->set_visible(true);
    if (trivia_ui) trivia_ui->set_visible(false);
    if (losers_ui) losers_ui->set_visible(false);
    if (winner_ui) winner_ui->set_visible(false);

    update_lobby_ui();
}

void GameManager::update_lobby_ui() {
    Node* lobby_grid = get_node<Node>("LobbyUI/CenterContainer/LobbyGrid");
    if (!lobby_grid) return;

    while (lobby_grid->get_child_count() > 0) {
        Node* child = lobby_grid->get_child(0);
        lobby_grid->remove_child(child);
        child->queue_free();
    }

    for (int i = 0; i < players_list.size(); i++) {
        VBoxContainer* player_vbox = memnew(VBoxContainer);

        TextureRect* avatar_rect = memnew(TextureRect);
        Ref<Texture2D> avatar_tex = ResourceLoader::get_singleton()->load(players_list[i].avatar_path);
        if (avatar_tex.is_valid()) {
            avatar_rect->set_texture(avatar_tex);
            avatar_rect->set_expand_mode(TextureRect::EXPAND_FIT_WIDTH_PROPORTIONAL);
            avatar_rect->set_custom_minimum_size(Vector2(100, 100)); 
            player_vbox->add_child(avatar_rect);
        }

        Label* name_label = memnew(Label);
        name_label->set_text(players_list[i].name);
        name_label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
        player_vbox->add_child(name_label);

        lobby_grid->add_child(player_vbox);
    }
}

void GameManager::_on_start_button_pressed() {
    if (current_state != 3) return; 

    // Если мы без реальных игроков, заполняем лобби тестовыми данными
    if (!use_real_players && players_list.empty()) {
        for (int i = 0; i < 5; i++) {
            if (i < profiles_db.size()) {
                api_bot_register_player(profiles_db[i].telegram_id);
            }
        }
    }

    start_game();
}

void GameManager::start_game() {
    current_q_idx = 0;
    
    CanvasItem* lobby_ui = get_node<CanvasItem>("LobbyUI");
    if (lobby_ui) lobby_ui->set_visible(false);

    if (!questions_list.empty()) display_question();
}

// =========================================================================================
// API ДЛЯ ИНТЕГРАЦИИ С ТЕЛЕГРАМ БОТОМ (ЭТО ДЛЯ ТВОЕГО ТОВАРИЩА)
// Инструкция:
// 1. Бот ловит нажатия кнопок в Telegram.
// 2. Скрипт (Godot) принимает Webhook/Polling.
// 3. Скрипт вызывает нужную функцию у узла GameManager, передавая ID игрока и индекс кнопки.
// =========================================================================================

void GameManager::api_bot_register_player(String tg_id) {
    // 1. Проверяем, не в лобби ли этот игрок уже
    for (int i = 0; i < players_list.size(); i++) {
        if (players_list[i].telegram_id == tg_id) return; 
    }

    // 2. Ищем игрока в нашей загруженной CSV базе
    for (int i = 0; i < profiles_db.size(); i++) {
        if (profiles_db[i].telegram_id == tg_id) {
            Player new_player;
            new_player.telegram_id = profiles_db[i].telegram_id;
            new_player.name = profiles_db[i].name;
            new_player.avatar_path = profiles_db[i].avatar_path;
            new_player.score = 0;
            new_player.is_alive = true;
            new_player.selected_answer = -1;
            new_player.minigame_choice = -1;
            players_list.push_back(new_player);
            
            if (current_state == 3) update_lobby_ui(); 
            return; 
        }
    }

    // 3. Если игрок не найден в базе, создаем гостя с дефолтной картинкой
    Player unknown_player;
    unknown_player.telegram_id = tg_id;
    unknown_player.name = String(L"Гость_") + tg_id.substr(0, 4); 
    unknown_player.avatar_path = "res://avatars/default.png"; // <-- ДЕФОЛТНАЯ АВАТАРКА
    unknown_player.score = 0;
    unknown_player.is_alive = true;
    unknown_player.selected_answer = -1;
    unknown_player.minigame_choice = -1;
    players_list.push_back(unknown_player);

    if (current_state == 3) update_lobby_ui(); 
}

// Принимает 0, 1, 2, 3 (или 4 от самой игры при таймауте)
void GameManager::api_bot_receive_answer(String tg_id, int answer_index) {
    if (current_state != 0) return; 
    for (int i = 0; i < players_list.size(); i++) {
        if (players_list[i].telegram_id == tg_id && players_list[i].is_alive && players_list[i].selected_answer == -1) {
            players_list[i].selected_answer = answer_index;
            break;
        }
    }
}

// Принимает 0 (Красный), 1 (Синий). (Или 4 при таймауте)
void GameManager::api_bot_minigame_wires(String tg_id, int wire_index) {
    if (current_state != 1 || current_minigame_idx != 0) return;
    for (int i = 0; i < players_list.size(); i++) {
        if (players_list[i].telegram_id == tg_id) {
            players_list[i].minigame_choice = wire_index;
            break;
        }
    }
}

// Принимает 0 (Первый), 1 (Второй), 2 (Третий). (Или 4 при таймауте)
void GameManager::api_bot_minigame_cups(String tg_id, int cup_index) {
    if (current_state != 1 || current_minigame_idx != 1) return;
    for (int i = 0; i < players_list.size(); i++) {
        if (players_list[i].telegram_id == tg_id) {
            players_list[i].minigame_choice = cup_index;
            break;
        }
    }
}

// Принимает 0 (Левая), 1 (Правая). (Или 4 при таймауте)
void GameManager::api_bot_minigame_minefield(String tg_id, int path_index) {
    if (current_state != 1 || current_minigame_idx != 2) return;
    for (int i = 0; i < players_list.size(); i++) {
        if (players_list[i].telegram_id == tg_id) {
            players_list[i].minigame_choice = path_index;
            break;
        }
    }
}
// =========================================================================================

void GameManager::load_profiles_from_csv() {
    String file_path = "res://players.csv";
    if (!FileAccess::file_exists(file_path)) return;
    Ref<FileAccess> file = FileAccess::open(file_path, FileAccess::READ);
    if (file.is_null()) return;

    profiles_db.clear();
    while (!file->eof_reached()) {
        PackedStringArray row = file->get_csv_line(",");
        if (row.size() < 3) continue;
        PlayerProfile p;
        p.telegram_id = row[0].strip_edges(); // Очистка невидимых символов (\r)
        p.name = row[1].strip_edges();
        p.avatar_path = row[2].strip_edges(); 
        profiles_db.push_back(p);
    }
}

void GameManager::load_questions_from_csv() {
    String file_path = "res://questions.csv";
    if (!FileAccess::file_exists(file_path)) return;
    Ref<FileAccess> file = FileAccess::open(file_path, FileAccess::READ);
    if (file.is_null()) return;

    questions_list.clear();
    while (!file->eof_reached()) {
        PackedStringArray row = file->get_csv_line(",");
        if (row.size() < 6) continue;
        Question q;
        q.text = row[0].strip_edges();
        q.answers.push_back(row[1].strip_edges());
        q.answers.push_back(row[2].strip_edges());
        q.answers.push_back(row[3].strip_edges());
        q.answers.push_back(row[4].strip_edges());
        q.correct_answer_idx = row[5].strip_edges().to_int();
        questions_list.push_back(q);
    }
}

void GameManager::display_question() {
    if (current_q_idx >= questions_list.size()) {
        show_winner();
        return;
    }

    current_state = 0; 
    for (int i = 0; i < players_list.size(); i++) {
        players_list[i].selected_answer = -1;
    }
    
    CanvasItem* lobby_ui = get_node<CanvasItem>("LobbyUI");
    CanvasItem* trivia_ui = get_node<CanvasItem>("TriviaUI");
    CanvasItem* losers_ui = get_node<CanvasItem>("LosersUI");
    CanvasItem* winner_ui = get_node<CanvasItem>("WinnerUI");
    
    if (lobby_ui) lobby_ui->set_visible(false);
    if (trivia_ui) trivia_ui->set_visible(true);
    if (losers_ui) losers_ui->set_visible(false);
    if (winner_ui) winner_ui->set_visible(false);

    Question current_q = questions_list[current_q_idx];
    Label* question_label = get_node<Label>("TriviaUI/QuestionText");
    if (question_label) question_label->set_text(current_q.text);

    for (int i = 0; i < 4; i++) {
        String answer_node_name = "TriviaUI/Answer" + String::num_int64(i + 1);
        Label* ans_label = get_node<Label>(NodePath(answer_node_name));
        if (ans_label) ans_label->set_text(current_q.answers[i]);
    }

    Node* top_grid = get_node<Node>("TriviaUI/CenterContainer/TopPlayersGrid");
    if (top_grid) {
        while (top_grid->get_child_count() > 0) {
            Node* child = top_grid->get_child(0);
            top_grid->remove_child(child);
            child->queue_free();
        }

        for (int i = 0; i < players_list.size(); i++) {
            VBoxContainer* player_vbox = memnew(VBoxContainer);

            TextureRect* avatar_rect = memnew(TextureRect);
            Ref<Texture2D> avatar_tex = ResourceLoader::get_singleton()->load(players_list[i].avatar_path);
            if (avatar_tex.is_valid()) {
                avatar_rect->set_texture(avatar_tex);
                avatar_rect->set_expand_mode(TextureRect::EXPAND_FIT_WIDTH_PROPORTIONAL);
                avatar_rect->set_custom_minimum_size(Vector2(60, 60)); 
                if (!players_list[i].is_alive) {
                    avatar_rect->set_modulate(Color(1.0, 1.0, 1.0, 0.3)); 
                }
                player_vbox->add_child(avatar_rect);
            }

            Label* score_label = memnew(Label);
            score_label->set_text(String::num_int64(players_list[i].score));
            score_label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER); 
            player_vbox->add_child(score_label);

            top_grid->add_child(player_vbox);
        }
    }

    time_left = 20.0; 
    max_time_for_state = 20.0;
    is_timer_running = true;
}

void GameManager::end_round() {
    int correct_idx = questions_list[current_q_idx].correct_answer_idx;
    current_losers_indices.clear(); 

    for (int i = 0; i < players_list.size(); i++) {
        if (players_list[i].selected_answer == correct_idx) {
            players_list[i].score += 1; 
        } else {
            if (players_list[i].is_alive) {
                current_losers_indices.push_back(i); 
            }
        }
    }
    
    if (current_losers_indices.size() > 0) {
        show_losers(); 
    } else {
        int total_alive = 0;
        for (int i = 0; i < players_list.size(); i++) {
            if (players_list[i].is_alive) total_alive++;
        }
        if (total_alive <= 1) show_winner();
        else next_round();
    }
}

void GameManager::show_losers() {
    current_state = 1; 
    for (int i = 0; i < players_list.size(); i++) {
        players_list[i].minigame_choice = -1;
    }

    CanvasItem* trivia_ui = get_node<CanvasItem>("TriviaUI");
    CanvasItem* losers_ui = get_node<CanvasItem>("LosersUI");
    if (trivia_ui) trivia_ui->set_visible(false);
    if (losers_ui) losers_ui->set_visible(true);

    Label* status_label = get_node<Label>("LosersUI/MinigameStatus");
    if (status_label) {
        if (current_minigame_idx == 0) status_label->set_text(String(L"МИНИ-ИГРА: Выберите провод (Красный или Синий)!"));
        else if (current_minigame_idx == 1) status_label->set_text(String(L"МИНИ-ИГРА: Перед вами 3 бокала. Один отравлен!"));
        else status_label->set_text(String(L"МИНИ-ИГРА: Минное поле. Налево или направо?"));
    }

    GridContainer* grid = get_node<GridContainer>("LosersUI/CenterContainer/LosersGrid");
    if (grid) {
        while (grid->get_child_count() > 0) {
            Node* child = grid->get_child(0);
            grid->remove_child(child);
            child->queue_free(); 
        }

        for (int i = 0; i < current_losers_indices.size(); i++) {
            int idx = current_losers_indices[i];
            TextureRect* avatar_rect = memnew(TextureRect);
            Ref<Texture2D> avatar_tex = ResourceLoader::get_singleton()->load(players_list[idx].avatar_path);
            if (avatar_tex.is_valid()) {
                avatar_rect->set_texture(avatar_tex);
                avatar_rect->set_expand_mode(TextureRect::EXPAND_FIT_WIDTH_PROPORTIONAL);
                avatar_rect->set_custom_minimum_size(Vector2(150, 150)); 
                grid->add_child(avatar_rect);
            }
        }
    }

    time_left = 5.0; 
    max_time_for_state = 5.0;
    is_timer_running = true;
}

void GameManager::process_minigame() {
    current_state = 2; 

    Label* status_label = get_node<Label>("LosersUI/MinigameStatus");
    GridContainer* grid = get_node<GridContainer>("LosersUI/CenterContainer/LosersGrid");
    std::vector<int> died_now;

    if (current_minigame_idx == 0) run_minigame_wires(died_now, status_label, grid);
    else if (current_minigame_idx == 1) run_minigame_poison(died_now, status_label, grid);
    else run_minigame_minefield(died_now, status_label, grid);
    
    prevent_total_wipeout(died_now, status_label, grid);
    current_minigame_idx = (current_minigame_idx + 1) % 3;
    
    time_left = 5.0;
    max_time_for_state = 5.0;
    is_timer_running = true; 
}

void GameManager::run_minigame_wires(std::vector<int>& died_now, Label* status_label, GridContainer* grid) {
    int deadly_wire = rand() % 2; 
    if (status_label) {
        if (deadly_wire == 0) status_label->set_text(String(L"Тех кто выбрал красный провод убило током"));
        else status_label->set_text(String(L"Тех кто выбрал синий провод убило током"));
    }
    for (int i = 0; i < current_losers_indices.size(); i++) {
        int player_idx = current_losers_indices[i];
        int player_choice = players_list[player_idx].minigame_choice; 
        if (player_choice == deadly_wire || player_choice == 4) {
            players_list[player_idx].is_alive = false;
            died_now.push_back(player_idx); 
            if (grid && i < grid->get_child_count()) {
                TextureRect* rect = Object::cast_to<TextureRect>(grid->get_child(i));
                if (rect) rect->set_modulate(Color(1.0, 0.2, 0.2, 0.4)); 
            }
        }
    }
}

void GameManager::run_minigame_poison(std::vector<int>& died_now, Label* status_label, GridContainer* grid) {
    int poisoned_cup = rand() % 3; 
    if (status_label) {
        if (poisoned_cup == 0) status_label->set_text(String(L"Первый бокал был отравлен"));
        else if (poisoned_cup == 1) status_label->set_text(String(L"Второй бокал был отравлен"));
        else status_label->set_text(String(L"Третий бокал был отравлен"));
    }
    for (int i = 0; i < current_losers_indices.size(); i++) {
        int player_idx = current_losers_indices[i];
        int player_choice = players_list[player_idx].minigame_choice; 
        if (player_choice == poisoned_cup || player_choice == 4) {
            players_list[player_idx].is_alive = false;
            died_now.push_back(player_idx); 
            if (grid && i < grid->get_child_count()) {
                TextureRect* rect = Object::cast_to<TextureRect>(grid->get_child(i));
                if (rect) rect->set_modulate(Color(0.2, 1.0, 0.2, 0.4)); 
            }
        }
    }
}

void GameManager::run_minigame_minefield(std::vector<int>& died_now, Label* status_label, GridContainer* grid) {
    int safe_path = rand() % 2; 
    if (status_label) {
        if (safe_path == 0) status_label->set_text(String(L"Тех кто выбрал правую тропинку подорвало на мине"));
        else status_label->set_text(String(L"Тех кто выбрал левую тропинку подорвало на мине"));
    }
    for (int i = 0; i < current_losers_indices.size(); i++) {
        int player_idx = current_losers_indices[i];
        int player_choice = players_list[player_idx].minigame_choice; 
        if (player_choice != safe_path || player_choice == 4) {
            players_list[player_idx].is_alive = false;
            died_now.push_back(player_idx); 
            if (grid && i < grid->get_child_count()) {
                TextureRect* rect = Object::cast_to<TextureRect>(grid->get_child(i));
                if (rect) rect->set_modulate(Color(0.2, 0.2, 0.2, 0.5)); 
            }
        }
    }
}

void GameManager::prevent_total_wipeout(std::vector<int>& died_now, Label* status_label, GridContainer* grid) {
    int total_alive = 0;
    for (int i = 0; i < players_list.size(); i++) {
        if (players_list[i].is_alive) total_alive++;
    }
    if (total_alive == 0 && died_now.size() > 0) {
        int savior_idx = died_now[rand() % died_now.size()];
        players_list[savior_idx].is_alive = true;
        if (status_label) status_label->set_text(String(L"ВСЕ ПОГИБЛИ! Но чудо спасло игрока: ") + players_list[savior_idx].name);
        for (int i = 0; i < current_losers_indices.size(); i++) {
            if (current_losers_indices[i] == savior_idx) {
                if (grid && i < grid->get_child_count()) {
                    TextureRect* rect = Object::cast_to<TextureRect>(grid->get_child(i));
                    if (rect) rect->set_modulate(Color(1.0, 1.0, 1.0, 1.0)); 
                }
            }
        }
    }
}

void GameManager::next_round() {
    current_q_idx++; 
    display_question();
}

void GameManager::show_winner() {
    CanvasItem* lobby_ui = get_node<CanvasItem>("LobbyUI");
    CanvasItem* trivia_ui = get_node<CanvasItem>("TriviaUI");
    CanvasItem* losers_ui = get_node<CanvasItem>("LosersUI");
    CanvasItem* winner_ui = get_node<CanvasItem>("WinnerUI");
    
    if (lobby_ui) lobby_ui->set_visible(false);
    if (trivia_ui) trivia_ui->set_visible(false);
    if (losers_ui) losers_ui->set_visible(false);
    if (winner_ui) winner_ui->set_visible(true); 

    std::vector<Player> sorted_players = players_list;
    std::sort(sorted_players.begin(), sorted_players.end(), [](const Player& a, const Player& b) {
        return a.score > b.score;
    });

    Player* top_alive = nullptr;
    Player* second_place = nullptr;

    for (int i = 0; i < sorted_players.size(); i++) {
        if (sorted_players[i].is_alive) {
            top_alive = &sorted_players[i];
            break;
        }
    }
    for (int i = 0; i < sorted_players.size(); i++) {
        if (&sorted_players[i] != top_alive) {
            second_place = &sorted_players[i];
            break;
        }
    }
    if (top_alive != nullptr) {
        TextureRect* win_avatar = get_node<TextureRect>("WinnerUI/WinnerAvatar");
        if (win_avatar) {
            Ref<Texture2D> tex = ResourceLoader::get_singleton()->load(top_alive->avatar_path);
            win_avatar->set_texture(tex);
        }
    }
    if (second_place != nullptr) {
        TextureRect* sec_avatar = get_node<TextureRect>("WinnerUI/SecondPlaceAvatar");
        if (sec_avatar) {
            Ref<Texture2D> tex = ResourceLoader::get_singleton()->load(second_place->avatar_path);
            sec_avatar->set_texture(tex);
            sec_avatar->set_modulate(Color(1.0, 1.0, 1.0, 0.6)); 
        }
    }
}