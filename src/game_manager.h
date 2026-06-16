#ifndef GAME_MANAGER_H
#define GAME_MANAGER_H

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/texture_rect.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/classes/grid_container.hpp>
#include <godot_cpp/classes/texture_progress_bar.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/array.hpp>
#include <vector>
#include <memory>
#include <grpcpp/grpcpp.h>
#include "murder_trivia.pb.h"
#include "murder_trivia.grpc.pb.h"

namespace godot {

// Структура для хранения одного вопроса из CSV
struct Question {
    String text;
    std::vector<String> answers;
    int correct_answer_idx;
};

// Профиль игрока, загружаемый из players.csv при старте игры.
// Служит базой данных, чтобы понимать, какую картинку давать игроку по его Telegram ID.
struct PlayerProfile {
    String telegram_id;
    String name;
    String avatar_path;
};

// Текущий игрок, который физически находится в матче
struct Player {
    String telegram_id;      // Уникальный ID из телеграма (ключ для поиска)
    String name;             // Никнейм
    String avatar_path;      // Путь к картинке (res://avatars/...)
    int score;               // Количество правильных ответов
    bool is_alive;           // true = живой, false = проиграл в мини-игре (стал призраком)
    int selected_answer;     // Индекс выбранного ответа на текущий вопрос (-1 = еще не ответил)
    int minigame_choice;     // Индекс выбора в текущей мини-игре (-1 = еще не выбрал)
};

class GameManager : public Node {
    GDCLASS(GameManager, Node)

private:
    std::vector<godot::Question> questions_list;   // Список всех вопросов
    std::vector<PlayerProfile> profiles_db; // База всех зарегистрированных игроков
    std::vector<Player> players_list;       // Игроки в текущем лобби/матче
    std::vector<int> current_losers_indices; // Индексы игроков, которые ошиблись и попали в мини-игру

    int current_q_idx;         // Номер текущего вопроса
    double time_left;          // Оставшееся время таймера (в секундах)
    double max_time_for_state; // Максимальное время для текущего стейта (нужно для полоски таймера)
    bool is_timer_running;     // Запущен ли таймер сейчас
    
    // Состояние игры:
    // 0 = Вопросы
    // 1 = Ожидание выбора в мини-игре
    // 2 = Показ результатов мини-игры (взрывы/яд)
    // 3 = Лобби (Ожидание игроков перед стартом)
    int current_state; 
    
    // Текущая мини игра: 0 = Провода, 1 = Бокалы, 2 = Минное поле
    int current_minigame_idx; 
    
    // =========================================================
    // ФЛАГ ПЕРЕКЛЮЧЕНИЯ ИГРОКОВ И БОТОВ
    // false -> игра играет сама в себя (тестовый режим)
    // true  -> игра ждет данные от бота Telegram (боевой режим)
    // =========================================================
    bool use_real_players; 

    // gRPC клиент для связи с Python-ботом
    std::unique_ptr<::BotRPC::Stub> stub_;

    // Внутренние методы логики
    void load_questions_from_csv();
    void load_profiles_from_csv();
    void display_question();
    void end_round();
    void show_losers();
    void process_minigame();
    void next_round();
    void show_winner();
    void prevent_total_wipeout(std::vector<int>& died_now, Label* status_label, GridContainer* grid);
    void run_minigame_wires(std::vector<int>& died_now, Label* status_label, GridContainer* grid);
    void run_minigame_poison(std::vector<int>& died_now, Label* status_label, GridContainer* grid);
    void run_minigame_minefield(std::vector<int>& died_now, Label* status_label, GridContainer* grid);

    // Вспомогательные gRPC методы отправки и получения данных
    void fetch_player_answers();
    void fetch_registered_players();
    void call_start_registration();
    void call_send_new_question(const godot::Question& q);
    void call_correct_answer_was(int correct_idx);
    void call_assign_minigame(int64_t chat_id, int minigame_type);
    void call_send_youre_dead(int64_t chat_id);
    void call_send_youre_alive(int64_t chat_id);
    void call_show_winners();

protected:
    static void _bind_methods();

public:
    GameManager();
    ~GameManager();

    void _ready() override;
    void _process(double delta) override;

    // Методы управления матчем
    void start_game();
    void show_lobby();
    void update_lobby_ui();
    void _on_start_button_pressed(); // Вызывается кнопкой "НАЧАТЬ ИГРУ" из интерфейса
};

}
#endif