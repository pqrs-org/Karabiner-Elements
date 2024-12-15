#pragma once

#include "impl.hpp"
#include <mapbox/eternal.hpp>

namespace krbn {
namespace momentary_switch_event_details {
namespace consumer_key_code {
constexpr std::pair<const mapbox::eternal::string, const pqrs::hid::usage::value_t> name_value_pairs[] = {
    // High priority aliases
    {"dictation", pqrs::hid::usage::consumer::voice_command},

    {"power", pqrs::hid::usage::consumer::power},

    {"menu", pqrs::hid::usage::consumer::menu}, // Touch ID key on Magic Keyboard
    {"menu_pick", pqrs::hid::usage::consumer::menu_pick},
    {"menu_up", pqrs::hid::usage::consumer::menu_up},
    {"menu_down", pqrs::hid::usage::consumer::menu_down},
    {"menu_left", pqrs::hid::usage::consumer::menu_left},
    {"menu_right", pqrs::hid::usage::consumer::menu_right},
    {"menu_escape", pqrs::hid::usage::consumer::menu_escape},
    {"menu_value_increase", pqrs::hid::usage::consumer::menu_value_increase},
    {"menu_value_decrease", pqrs::hid::usage::consumer::menu_value_decrease},

    {"data_on_screen", pqrs::hid::usage::consumer::data_on_screen},
    {"closed_caption", pqrs::hid::usage::consumer::closed_caption},
    {"closed_caption_select", pqrs::hid::usage::consumer::closed_caption_select},
    {"vcr_or_tv", pqrs::hid::usage::consumer::vcr_or_tv},
    {"broadcast_mode", pqrs::hid::usage::consumer::broadcast_mode},
    {"snapshot", pqrs::hid::usage::consumer::snapshot},
    {"still", pqrs::hid::usage::consumer::still},
    {"picture_in_picture_toggle", pqrs::hid::usage::consumer::picture_in_picture_toggle},
    {"picture_in_picture_swap", pqrs::hid::usage::consumer::picture_in_picture_swap},
    {"red_menu_button", pqrs::hid::usage::consumer::red_menu_button},
    {"green_menu_button", pqrs::hid::usage::consumer::green_menu_button},
    {"blue_menu_button", pqrs::hid::usage::consumer::blue_menu_button},
    {"yellow_menu_button", pqrs::hid::usage::consumer::yellow_menu_button},
    {"aspect", pqrs::hid::usage::consumer::aspect},
    {"three_dimensional_mode_select", pqrs::hid::usage::consumer::three_dimensional_mode_select},
    {"display_brightness_increment", pqrs::hid::usage::consumer::display_brightness_increment},
    {"display_brightness_decrement", pqrs::hid::usage::consumer::display_brightness_decrement},

    {"fast_forward", pqrs::hid::usage::consumer::fast_forward},
    {"rewind", pqrs::hid::usage::consumer::rewind},
    {"scan_next_track", pqrs::hid::usage::consumer::scan_next_track},
    {"scan_previous_track", pqrs::hid::usage::consumer::scan_previous_track},
    {"stop", pqrs::hid::usage::consumer::stop},
    {"eject", pqrs::hid::usage::consumer::eject},
    {"play_or_pause", pqrs::hid::usage::consumer::play_or_pause},
    {"voice_command", pqrs::hid::usage::consumer::voice_command},
    {"mute", pqrs::hid::usage::consumer::mute},
    {"bass_boost", pqrs::hid::usage::consumer::bass_boost},
    {"loudness", pqrs::hid::usage::consumer::loudness},
    {"volume_increment", pqrs::hid::usage::consumer::volume_increment},
    {"volume_decrement", pqrs::hid::usage::consumer::volume_decrement},
    {"bass_increment", pqrs::hid::usage::consumer::bass_increment},
    {"bass_decrement", pqrs::hid::usage::consumer::bass_decrement},

    // Application launch buttons
    {"al_consumer_control_configuration", pqrs::hid::usage::consumer::al_consumer_control_configuration},
    {"al_word_processor", pqrs::hid::usage::consumer::al_word_processor},
    {"al_text_editor", pqrs::hid::usage::consumer::al_text_editor},
    {"al_spreadsheet", pqrs::hid::usage::consumer::al_spreadsheet},
    {"al_graphics_editor", pqrs::hid::usage::consumer::al_graphics_editor},
    {"al_presentation_app", pqrs::hid::usage::consumer::al_presentation_app},
    {"al_database_app", pqrs::hid::usage::consumer::al_database_app},
    {"al_email_reader", pqrs::hid::usage::consumer::al_email_reader},
    {"al_newsreader", pqrs::hid::usage::consumer::al_newsreader},
    {"al_voicemail", pqrs::hid::usage::consumer::al_voicemail},
    {"al_contacts_or_address_book", pqrs::hid::usage::consumer::al_contacts_or_address_book},
    {"al_Calendar_Or_Schedule", pqrs::hid::usage::consumer::al_Calendar_Or_Schedule},
    {"al_task_or_project_manager", pqrs::hid::usage::consumer::al_task_or_project_manager},
    {"al_log_or_journal_or_timecard", pqrs::hid::usage::consumer::al_log_or_journal_or_timecard},
    {"al_checkbook_or_finance", pqrs::hid::usage::consumer::al_checkbook_or_finance},
    {"al_calculator", pqrs::hid::usage::consumer::al_calculator},
    {"al_a_or_v_capture_or_playback", pqrs::hid::usage::consumer::al_a_or_v_capture_or_playback},
    {"al_local_machine_browser", pqrs::hid::usage::consumer::al_local_machine_browser},
    {"al_lan_or_wan_browser", pqrs::hid::usage::consumer::al_lan_or_wan_browser},
    {"al_internet_browser", pqrs::hid::usage::consumer::al_internet_browser},
    {"al_remote_networking_or_isp_connect", pqrs::hid::usage::consumer::al_remote_networking_or_isp_connect},
    {"al_network_conference", pqrs::hid::usage::consumer::al_network_conference},
    {"al_network_chat", pqrs::hid::usage::consumer::al_network_chat},
    {"al_telephony_or_dialer", pqrs::hid::usage::consumer::al_telephony_or_dialer},
    {"al_logon", pqrs::hid::usage::consumer::al_logon},
    {"al_logoff", pqrs::hid::usage::consumer::al_logoff},
    {"al_logon_or_logoff", pqrs::hid::usage::consumer::al_logon_or_logoff},
    {"al_terminal_lock_or_screensaver", pqrs::hid::usage::consumer::al_terminal_lock_or_screensaver}, // Lock key on Magic Keyboard without Touch ID
    {"al_control_panel", pqrs::hid::usage::consumer::al_control_panel},
    {"al_command_line_processor_or_run", pqrs::hid::usage::consumer::al_command_line_processor_or_run},
    {"al_process_or_task_manager", pqrs::hid::usage::consumer::al_process_or_task_manager},
    {"al_select_task_or_application", pqrs::hid::usage::consumer::al_select_task_or_application},
    {"al_next_task_or_application", pqrs::hid::usage::consumer::al_next_task_or_application},
    {"al_previous_task_or_application", pqrs::hid::usage::consumer::al_previous_task_or_application},
    {"al_preemptive_halt_task_or_application", pqrs::hid::usage::consumer::al_preemptive_halt_task_or_application},
    {"al_integrated_help_center", pqrs::hid::usage::consumer::al_integrated_help_center},
    {"al_documents", pqrs::hid::usage::consumer::al_documents},
    {"al_thesaurus", pqrs::hid::usage::consumer::al_thesaurus},
    {"al_dictionary", pqrs::hid::usage::consumer::al_dictionary},
    {"al_desktop", pqrs::hid::usage::consumer::al_desktop},
    {"al_spell_check", pqrs::hid::usage::consumer::al_spell_check},
    {"al_grammer_check", pqrs::hid::usage::consumer::al_grammer_check},
    {"al_wireless_status", pqrs::hid::usage::consumer::al_wireless_status},
    {"al_keyboard_layout", pqrs::hid::usage::consumer::al_keyboard_layout},
    {"al_virus_protection", pqrs::hid::usage::consumer::al_virus_protection},
    {"al_encryption", pqrs::hid::usage::consumer::al_encryption},
    {"al_screen_saver", pqrs::hid::usage::consumer::al_screen_saver},
    {"al_alarms", pqrs::hid::usage::consumer::al_alarms},
    {"al_clock", pqrs::hid::usage::consumer::al_clock},
    {"al_file_browser", pqrs::hid::usage::consumer::al_file_browser},
    {"al_power_status", pqrs::hid::usage::consumer::al_power_status},
    {"al_image_browser", pqrs::hid::usage::consumer::al_image_browser},
    {"al_audio_browser", pqrs::hid::usage::consumer::al_audio_browser},
    {"al_movie_browser", pqrs::hid::usage::consumer::al_movie_browser},
    {"al_digital_rights_manager", pqrs::hid::usage::consumer::al_digital_rights_manager},
    {"al_digital_wallet", pqrs::hid::usage::consumer::al_digital_wallet},
    {"al_instant_messaging", pqrs::hid::usage::consumer::al_instant_messaging},
    {"al_oem_feature_browser", pqrs::hid::usage::consumer::al_oem_feature_browser},
    {"al_oem_help", pqrs::hid::usage::consumer::al_oem_help},
    {"al_online_community", pqrs::hid::usage::consumer::al_online_community},
    {"al_entertainment_content_browser", pqrs::hid::usage::consumer::al_entertainment_content_browser},
    {"al_online_shopping_browswer", pqrs::hid::usage::consumer::al_online_shopping_browswer},
    {"al_smart_card_information_or_help", pqrs::hid::usage::consumer::al_smart_card_information_or_help},
    {"al_market_monitor_or_finance_browser", pqrs::hid::usage::consumer::al_market_monitor_or_finance_browser},
    {"al_customized_corporate_news_browser", pqrs::hid::usage::consumer::al_customized_corporate_news_browser},
    {"al_online_activity_browswer", pqrs::hid::usage::consumer::al_online_activity_browswer},
    {"al_research_or_search_browswer", pqrs::hid::usage::consumer::al_research_or_search_browswer},
    {"al_audio_player", pqrs::hid::usage::consumer::al_audio_player},
    {"al_message_status", pqrs::hid::usage::consumer::al_message_status},
    {"al_contact_sync", pqrs::hid::usage::consumer::al_contact_sync},
    {"al_navigation", pqrs::hid::usage::consumer::al_navigation},
    {"al_contextaware_desktop_assistant", pqrs::hid::usage::consumer::al_contextaware_desktop_assistant},

    // Generic gui application controls
    {"ac_search", pqrs::hid::usage::consumer::ac_search},
    {"ac_home", pqrs::hid::usage::consumer::ac_home},
    {"ac_back", pqrs::hid::usage::consumer::ac_back},
    {"ac_forward", pqrs::hid::usage::consumer::ac_forward},
    {"ac_refresh", pqrs::hid::usage::consumer::ac_refresh},
    {"ac_bookmarks", pqrs::hid::usage::consumer::ac_bookmarks},
    {"ac_zoom_out", pqrs::hid::usage::consumer::ac_zoom_out},
    {"ac_zoom_in", pqrs::hid::usage::consumer::ac_zoom_in},
    // Do not include ac_pan since it is used as mouse wheel, not button.

    // Aliases
    {"fastforward", pqrs::hid::usage::consumer::fast_forward},
};

constexpr auto name_value_map = mapbox::eternal::hash_map<mapbox::eternal::string, pqrs::hid::usage::value_t>(name_value_pairs);

inline bool target(pqrs::hid::usage_page::value_t usage_page,
                   pqrs::hid::usage::value_t usage) {
  if (usage_page == pqrs::hid::usage_page::consumer) {
    return impl::find_pair(name_value_pairs, usage) != std::end(name_value_pairs);
  }

  return false;
}

inline std::string make_name(pqrs::hid::usage::value_t usage) {
  return impl::make_name(name_value_pairs, usage);
}

inline pqrs::hid::usage_pair make_usage_pair(const std::string& key,
                                             const nlohmann::json& json) {
  return impl::make_usage_pair(name_value_map,
                               pqrs::hid::usage_page::consumer,
                               key,
                               json);
}
} // namespace consumer_key_code
} // namespace momentary_switch_event_details
} // namespace krbn
