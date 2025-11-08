project(
    'player-manager',
    'cpp',
    version: '0.1.0',
    default_options: ['cpp_std=c++23'],
)

dependencies = [dependency('playerctl')]

conf_data = configuration_data()

supported_players = get_option('supported_players')
if supported_players.length() > 0
    conf_data.set('SUPPORTED_PLAYERS', '"' + '", "'.join(supported_players) + '"')
else
    conf_data.set('SUPPORTED_PLAYERS', '')
endif

conf_data.set('SUPPORTED_PLAYERS_LENGTH', supported_players.length())
conf_data.set('TITLE_LENGTH', get_option('title_length'))
conf_data.set('ARTIST_LENGTH', get_option('artist_length'))

configure_file(output: 'config.hpp', configuration: conf_data)

source_files = files('main.cpp')

executable('player-manager', source_files, dependencies: dependencies)