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
}

GameManager::GameManager() {
    current_q_idx = 0;
    time_left = 0.0;
    max_time_for_state = 1.0;
    is_timer_running = false;
    current_state = 3; // Старт с экрана лобби
    current_minigame_idx = 0;
    
    // Включаем gRPC интеграцию по умолчанию!
    use_real_players = true; 

    // Настраиваем gRPC канал к Python боту, запущенному локально
    auto channel = grpc::CreateChannel("127.0.0.1:50051", grpc::InsecureChannelCredentials());
    stub_ = ::BotRPC::NewStub(channel);
}

GameManager::~GameManager() {}

void GameManager::_ready() {
    load_questions_from_csv();
    load_profiles_from_csv();
    show_lobby(); 
}

void GameManager::_process(double delta) {
    // Переменная для контроля частоты опроса gRPC
    static double lobby_update_timer = 0.0;

    if (current_state == 3) { // Мы в лобби / регистрации
        lobby_update_timer += delta;
        if (lobby_update_timer >= 1.0) { // Опрашиваем бота раз в 1 секунду
            lobby_update_timer = 0.0;
            if (use_real_players) {
                fetch_registered_players();
            }
        }   
        return; // Прекращаем выполнение логики игры, пока мы в лобби
    }

    if (!is_timer_running) return;

    time_left -= delta;

    TextureProgressBar* timer_bar = get_node<TextureProgressBar>("TriviaUI/TimerBar");
    if (timer_bar) {
        timer_bar->set_max(max_time_for_state);
        timer_bar->set_value(time_left);
    }

    // СИМУЛЯЦИЯ ИГРОКОВ (Если use_real_players = false)
    if (!use_real_players && time_left > 0.0 && time_left < 0.2) {
        if (current_state == 0) {
            for (int i = 0; i < players_list.size(); i++) {
                if (players_list[i].is_alive && players_list[i].selected_answer == -1) {
                    players_list[i].selected_answer = rand() % 4; 
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
                    players_list[idx].minigame_choice = r_choice; 
                }
            }
        }
    }

    // ОБРАБОТКА ОКОНЧАНИЯ ТАЙМЕРА
    if (time_left <= 0.0) {
        is_timer_running = false;
        
        // Получаем ответы от бота по gRPC
        if (use_real_players) {
            fetch_player_answers();
        }
        
        if (current_state == 0) {
            for (int i = 0; i < players_list.size(); i++) {
                if (players_list[i].is_alive && players_list[i].selected_answer == -1) {
                    players_list[i].selected_answer = 4; // Не ответил
                }
            }
            end_round();
        } else if (current_state == 1) {
            for (int i = 0; i < current_losers_indices.size(); i++) {
                int idx = current_losers_indices[i];
                if (players_list[idx].minigame_choice == -1) {
                    players_list[idx].minigame_choice = 4; // Не сделал выбор в мини-игры
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

    // Запускаем сбор и регистрацию игроков в Telegram
    if (use_real_players) {
        call_start_registration();
    }

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

    // Если играем с ботами, генерируем фейков
    if (!use_real_players) {
        if (players_list.empty()) {
            for (int i = 0; i < 5 && i < profiles_db.size(); i++) {
                Player p;
                p.telegram_id = profiles_db[i].telegram_id;
                p.name = profiles_db[i].name;
                p.avatar_path = profiles_db[i].avatar_path;
                p.score = 0;
                p.is_alive = true;
                p.selected_answer = -1;
                p.minigame_choice = -1;
                players_list.push_back(p);
            }
        }
    }

    // Если реальные игроки не подключились, не даем запустить пустую игру
    if (players_list.empty()) {
        UtilityFunctions::print("Cannot start game: No players registered!");
        return;
    }

    // Переходим к игре
    start_game();
}

void GameManager::start_game() {
    current_q_idx = 0;
    
    CanvasItem* lobby_ui = get_node<CanvasItem>("LobbyUI");
    if (lobby_ui) lobby_ui->set_visible(false);

    if (!questions_list.empty()) display_question();
}

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
        p.telegram_id = row[0].strip_edges(); 
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
        godot::Question q;
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

    godot::Question current_q = questions_list[current_q_idx];
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

    if (use_real_players) {
        call_send_new_question(current_q);
    }
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

    if (use_real_players) {
        call_correct_answer_was(correct_idx);
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

    // Рассылаем мини-игры игрокам в Telegram
    if (use_real_players) {
        for (int i = 0; i < players_list.size(); i++) {
            if (players_list[i].is_alive) {
                int64_t chat_id = std::stoll(players_list[i].telegram_id.utf8().get_data());
                
                // Проверяем, проиграл ли игрок в этом раунде
                bool is_loser = (std::find(current_losers_indices.begin(), current_losers_indices.end(), i) != current_losers_indices.end());
                if (is_loser) {
                    // +1 потому что в proto типы мини-игр начинаются с 1
                    int minigame_type = current_minigame_idx + 1;
                    call_assign_minigame(chat_id, minigame_type);
                } else {
                    // Игрок угадал ответ, даем ему расслабиться (GAME_UNSPECIFIED)
                    call_assign_minigame(chat_id, 0); 
                }
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

    // Уведомляем каждого участника мини-игры о его судьбе
    if (use_real_players) {
        for (int idx : current_losers_indices) {
            int64_t chat_id = std::stoll(players_list[idx].telegram_id.utf8().get_data());
            if (!players_list[idx].is_alive) {
                call_send_youre_dead(chat_id);
            } else {
                call_send_youre_alive(chat_id);
            }
        }
    }

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

    if (use_real_players) {
        call_show_winners();
    }
}

// =========================================================================
// gRPC Вспомогательные методы отправки и получения данных
// =========================================================================

void GameManager::fetch_player_answers() {
    if (!stub_) return;
    google::protobuf::Empty request;
    ::PlayerList response;
    grpc::ClientContext context;
    
    auto status = stub_->receivePlayerAnswers(&context, request, &response);
    if (!status.ok()) {
        UtilityFunctions::print("gRPC receivePlayerAnswers failed: ", status.error_message().c_str());
        return;
    }

    for (const auto& pair : response.players()) {
        String tg_id = String::num_int64(pair.first);
        int ans = pair.second.answer(); 

        for (int i = 0; i < players_list.size(); i++) {
            if (players_list[i].telegram_id == tg_id) {
                if (current_state == 0 && players_list[i].is_alive) {
                    players_list[i].selected_answer = ans;
                } else if (current_state == 1 && ans != 4) { 
                    players_list[i].minigame_choice = ans;
                }
                break;
            }
        }
    }
}

void GameManager::fetch_registered_players() {
    if (!stub_) return;
    
    google::protobuf::Empty request;
    ::PlayerList response;
    grpc::ClientContext context;

    auto status = stub_->receiveRegisteredPlayers(&context, request, &response);
    if (!status.ok()) {
        UtilityFunctions::print("gRPC receiveRegisteredPlayers failed: ", status.error_message().c_str());
        return;
    }

    players_list.clear();

    for (const auto& pair : response.players()) {
        String tg_id = String::num_int64(pair.first);
        
        Player p;
        p.telegram_id = tg_id;
        p.name = "Unknown (" + tg_id + ")";
        p.avatar_path = "res://avatars/default.png"; 
        p.score = 0;
        p.is_alive = true;
        p.selected_answer = -1;
        p.minigame_choice = -1;

        for (int i = 0; i < profiles_db.size(); i++) {
            if (profiles_db[i].telegram_id == tg_id) {
                p.name = profiles_db[i].name;
                p.avatar_path = profiles_db[i].avatar_path;
                break;
            }
        }
        
        players_list.push_back(p);
    }

    update_lobby_ui();
}

void GameManager::call_start_registration() {
    if (!stub_) return;
    google::protobuf::Empty request;
    google::protobuf::Empty response;
    grpc::ClientContext context;
    
    auto status = stub_->startRegistration(&context, request, &response);
    if (!status.ok()) {
        UtilityFunctions::print("gRPC startRegistration failed: ", status.error_message().c_str());
    }
}

void GameManager::call_send_new_question(const godot::Question& q) {
    if (!stub_) return;
    ::Question request;
    if (q.answers.size() > 0) request.set_a(q.answers[0].utf8().get_data());
    if (q.answers.size() > 1) request.set_b(q.answers[1].utf8().get_data());
    if (q.answers.size() > 2) request.set_c(q.answers[2].utf8().get_data());
    if (q.answers.size() > 3) request.set_d(q.answers[3].utf8().get_data());

    google::protobuf::Empty response;
    grpc::ClientContext context;
    
    auto status = stub_->sendNewQuestion(&context, request, &response);
    if (!status.ok()) {
        UtilityFunctions::print("gRPC sendNewQuestion failed: ", status.error_message().c_str());
    }
}

void GameManager::call_correct_answer_was(int correct_idx) {
    if (!stub_) return;
    ::CorrectAnswer request;
    request.set_correct_answer(static_cast<::Answer>(correct_idx));
    
    google::protobuf::Empty response;
    grpc::ClientContext context;
    
    auto status = stub_->correctAnswerWas(&context, request, &response);
    if (!status.ok()) {
        UtilityFunctions::print("gRPC correctAnswerWas failed: ", status.error_message().c_str());
    }
}

void GameManager::call_assign_minigame(int64_t chat_id, int minigame_type) {
    if (!stub_) return;
    ::MinigameRequest request;
    request.set_chat_id(chat_id);
    request.set_game_type(static_cast<::MinigameType>(minigame_type));

    google::protobuf::Empty response;
    grpc::ClientContext context;
    
    auto status = stub_->assignMinigame(&context, request, &response);
    if (!status.ok()) {
        UtilityFunctions::print("gRPC assignMinigame failed: ", status.error_message().c_str());
    }
}

void GameManager::call_send_youre_dead(int64_t chat_id) {
    if (!stub_) return;
    ::MinigameRequest request;
    request.set_chat_id(chat_id);

    google::protobuf::Empty response;
    grpc::ClientContext context;
    
    auto status = stub_->sendYoureDead(&context, request, &response);
    if (!status.ok()) {
        UtilityFunctions::print("gRPC sendYoureDead failed: ", status.error_message().c_str());
    }
}

void GameManager::call_send_youre_alive(int64_t chat_id) {
    if (!stub_) return;
    ::MinigameRequest request;
    request.set_chat_id(chat_id);

    google::protobuf::Empty response;
    grpc::ClientContext context;
    
    auto status = stub_->sendYoureAlive(&context, request, &response);
    if (!status.ok()) {
        UtilityFunctions::print("gRPC sendYoureAlive failed: ", status.error_message().c_str());
    }
}

void GameManager::call_show_winners() {
    if (!stub_) return;
    ::PlayerList request;
    for (const auto& p : players_list) {
        if (p.is_alive) {
            int64_t chat_id = std::stoll(p.telegram_id.utf8().get_data());
            ::PlayerInfo info;
            info.set_chat_id(chat_id);
            (*request.mutable_players())[chat_id] = info;
        }
    }

    google::protobuf::Empty response;
    grpc::ClientContext context;
    
    auto status = stub_->showWinners(&context, request, &response);
    if (!status.ok()) {
        UtilityFunctions::print("gRPC showWinners failed: ", status.error_message().c_str());
    }
}