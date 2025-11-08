#include <glib-2.0/glib.h>
#include <playerctl/playerctl.h>

#include <algorithm>
#include <array>
#include <iostream>
#include <string>
#include <vector>

#include "config.hpp"

std::array<char const*, SUPPORTED_PLAYERS_LENGTH> const supported_players{
    SUPPORTED_PLAYERS};

auto find_and_replace_parenthesis(std::string const& title,
                                  std::string const& needle) noexcept
    -> std::string {
  auto position{title.find(needle)};
  if (position != std::string::npos) {
    if (position > 0 && title[position - 1] == ' ') {
      position = position - 1;
    }

    return title.substr(0, position);
  }

  return title;
}

auto edit_title(std::string const& title) noexcept -> std::string {
  std::string new_title{find_and_replace_parenthesis(title, "(ft")};
  new_title = find_and_replace_parenthesis(new_title, "(feat");
  return new_title;
}

auto truncate_string(std::string const& string, std::size_t width) noexcept
    -> std::string {
  if (string.length() > width) {
    std::string copy{string.begin(), string.begin() +
                                         static_cast<std::ptrdiff_t>(width) -
                                         static_cast<std::ptrdiff_t>(3)};
    return copy + "\u2026";
  }

  return {string};
}

auto get_string_value(PlayerctlPlayer* player,
                      gchar*(obtain_function)(PlayerctlPlayer*, GError**),
                      std::string const& message) noexcept -> std::string {
  GError* error{nullptr};
  gchar* value{obtain_function(player, &error)};

  if (error != nullptr) {
    std::cerr << message << error->message << '\n';
    g_error_free(error);
    return "";
  }

  if (value == nullptr) {
    return "";
  }

  return value;
}

auto string_to_lowercase(std::string& string) noexcept -> void {
  std::ranges::transform(
      string.begin(), string.end(), string.begin(),
      [](unsigned char letter) { return std::tolower(letter); });
}

auto show_metadata(PlayerctlPlayer* player) noexcept -> void {
  gchar* player_name_value{nullptr};
  gchar* status_value{nullptr};
  g_object_get(player, "player_name", &player_name_value, "status",
               &status_value, NULL);

  std::string player_name{player_name_value};
  std::string status{status_value};
  string_to_lowercase(status);
  std::string full_title{get_string_value(player, playerctl_player_get_title,
                                          "Error obtaining title! ")};

  if (full_title.empty()) {
    return;
  }

  std::string artist{get_string_value(player, playerctl_player_get_artist,
                                      "Error obtaining artist! ")};
  std::string title{edit_title(full_title)};
  std::string album{get_string_value(player, playerctl_player_get_album,
                                     "Error obtaining album! ")};

  auto free_space_title{
      std::max(static_cast<std::size_t>(0), TITLE_LENGTH - artist.length())};
  auto free_space_artist{
      std::max(static_cast<std::size_t>(0), ARTIST_LENGTH - title.length())};

  std::string truncated_title{
      truncate_string(title, ARTIST_LENGTH + free_space_title)};
  std::string truncated_album{
      truncate_string(artist, TITLE_LENGTH + free_space_artist)};

  std::cout << R"({"text": ")";
  std::cout << truncated_title << " - " << truncated_album;
  std::cout << R"(", "tooltip": ")";
  std::cout << player_name << " (" << status << "): " << title << " - "
            << artist << " - " << album;
  std::cout << R"("})" << '\n';
  std::cout << std::flush;
}

auto on_metadata(PlayerctlPlayer* player, GVariant* metadata) noexcept -> void {
  show_metadata(player);
}

auto initialize_manager() noexcept -> PlayerctlPlayerManager* {
  GError* error{nullptr};
  PlayerctlPlayerManager* manager{playerctl_player_manager_new(&error)};

  if (error != nullptr) {
    g_assert(manager == nullptr);
    std::cerr << "Error creating playerctl manager! " << error->message << '\n';
    g_error_free(error);
    return nullptr;
  }

  g_assert(manager != nullptr);
  return manager;
}

auto initialize_player(PlayerctlPlayerManager* manager,
                       PlayerctlPlayerName* player_name) noexcept -> void {
  GError* error{nullptr};
  PlayerctlPlayer* player{playerctl_player_new_from_name(player_name, &error)};

  if (error != nullptr) {
    g_assert(player == nullptr);
    std::cerr << "Error creating playerctl player! " << error->message << '\n';
    g_error_free(error);
    return;
  }

  g_assert(player != nullptr);
  g_signal_connect(player, "metadata", G_CALLBACK(&on_metadata), manager);
  playerctl_player_manager_manage_player(manager, player);
}

auto get_players(PlayerctlPlayerManager* manager) noexcept -> GList* {
  GError* error{nullptr};
  GList* players{playerctl_list_players(&error)};

  if (error != nullptr) {
    g_assert(players == nullptr);
    std::cerr << "Error obtaining player names! " << error->message << '\n';
    g_error_free(error);
    return nullptr;
  }

  g_assert(players != nullptr);
  return players;
}

auto on_name_appeared(PlayerctlPlayerManager* manager,
                      PlayerctlPlayerName* player_name) noexcept -> void {
  if (std::ranges::find(supported_players.begin(), supported_players.end(),
                        std::string{player_name->name}) !=
      supported_players.end()) {
    initialize_player(manager, player_name);
  }
}

auto on_player_vanished(PlayerctlPlayerManager* manager,
                        PlayerctlPlayer* player) noexcept -> void {
  GList* players{nullptr};
  g_object_get(manager, "players", &players, NULL);
  if (players != nullptr) {
    auto* player{static_cast<PlayerctlPlayer*>(players->data)};
    show_metadata(player);
  }
}

auto main(int argc, char* argv[]) noexcept -> int {
  PlayerctlPlayerManager* manager{initialize_manager()};

  if (manager == nullptr) {
    return 1;
  }

  g_signal_connect(manager, "name-appeared", G_CALLBACK(on_name_appeared),
                   NULL);
  g_signal_connect(manager, "player-vanished", G_CALLBACK(on_player_vanished),
                   NULL);

  GList* player_name_list{nullptr};
  g_object_get(manager, "player_names", &player_name_list, NULL);
  while (player_name_list != nullptr) {
    auto* player_name{
        static_cast<PlayerctlPlayerName*>(player_name_list->data)};
    on_name_appeared(manager, player_name);
    player_name_list = player_name_list->next;
  }

  GMainLoop* main{g_main_loop_new(nullptr, 0)};
  g_main_loop_run(main);
  g_main_loop_unref(main);

  return 0;
}
