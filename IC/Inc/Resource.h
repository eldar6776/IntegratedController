/*
----------------------------------------------------------------------
File        : Resource.h
Content     : Main resource header file of weather forecast demo
---------------------------END-OF-HEADER------------------------------
*/

#ifndef RESOURCE_H
#define RESOURCE_H

#include <stdlib.h>
#include "GUI.h"

#ifndef GUI_CONST_STORAGE
  #define GUI_CONST_STORAGE const
#endif
extern GUI_CONST_STORAGE GUI_FONT GUI_FontVerdana16_LAT;
extern GUI_CONST_STORAGE GUI_FONT GUI_FontVerdana20_LAT;
extern GUI_CONST_STORAGE GUI_FONT GUI_FontVerdana32_LAT;
  
extern GUI_CONST_STORAGE GUI_BITMAP bmanimation_candle_frame_1;
extern GUI_CONST_STORAGE GUI_BITMAP bmanimation_candle_frame_2;
extern GUI_CONST_STORAGE GUI_BITMAP bmanimation_candle_frame_3;
extern GUI_CONST_STORAGE GUI_BITMAP bmanimation_candle_frame_4;
extern GUI_CONST_STORAGE GUI_BITMAP bmanimation_welcome_frame_05;
extern GUI_CONST_STORAGE GUI_BITMAP bmanimation_welcome_frame_10;
extern GUI_CONST_STORAGE GUI_BITMAP bmanimation_welcome_frame_15;
extern GUI_CONST_STORAGE GUI_BITMAP bmanimation_welcome_frame_20;
extern GUI_CONST_STORAGE GUI_BITMAP bmanimation_welcome_frame_25;
extern GUI_CONST_STORAGE GUI_BITMAP bmanimation_welcome_frame_30;
extern GUI_CONST_STORAGE GUI_BITMAP bmanimation_welcome_frame_35;
extern GUI_CONST_STORAGE GUI_BITMAP bmanimation_welcome_frame_40;
extern GUI_CONST_STORAGE GUI_BITMAP bmanimation_welcome_frame_45;
extern GUI_CONST_STORAGE GUI_BITMAP bmanimation_welcome_frame_50;
extern GUI_CONST_STORAGE GUI_BITMAP bmanimation_welcome_frame_55;
extern GUI_CONST_STORAGE GUI_BITMAP bmanimation_welcome_frame_60;
extern GUI_CONST_STORAGE GUI_BITMAP bmanimation_welcome_frame_65;
extern GUI_CONST_STORAGE GUI_BITMAP bmanimation_welcome_frame_70;
extern GUI_CONST_STORAGE GUI_BITMAP bmanimation_welcome_frame_75;
extern GUI_CONST_STORAGE GUI_BITMAP bmanimation_welcome_frame_80;
extern GUI_CONST_STORAGE GUI_BITMAP bmanimation_welcome_frame_85;
extern GUI_CONST_STORAGE GUI_BITMAP bmanimation_welcome_frame_90;
extern GUI_CONST_STORAGE GUI_BITMAP bmanimation_welcome_frame_95;
extern GUI_CONST_STORAGE GUI_BITMAP bmanimation_welcome_frame_100;
extern GUI_CONST_STORAGE GUI_BITMAP bmanimation_welcome_frame_final;
    
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_lights_ceiling_led_fixture_off;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_lights_ceiling_led_fixture_on;  
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_lights_chandelier_off;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_lights_chandelier_on;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_lights_hanging_off;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_lights_hanging_on;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_lights_led_off;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_lights_led_on;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_lights_spot_console_off;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_lights_spot_console_on;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_lights_spot_single_off;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_lights_spot_single_on;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_lights_stairs_off;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_lights_stairs_on;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_lights_wall_off;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_lights_wall_on;

extern GUI_CONST_STORAGE GUI_BITMAP bmicons_menu_gate;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_menu_clean;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_menu_theme;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_menu_timers;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_menu_all_off;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_menu_outdoor_lihts_off;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_menu_outdoor_lihts_on;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_menu_defroster_off;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_menu_defroster_on;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_menu_ventilator_off;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_menu_ventilator_on;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_menu_language_bhsc; 
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_menu_language_eng;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_menu_language_ger;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_menu_language_fra;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_menu_language_ita;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_menu_language_spa;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_menu_language_rus;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_menu_language_ukr;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_menu_language_pol;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_menu_language_cze;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_menu_language_slo;

extern GUI_CONST_STORAGE GUI_BITMAP bmicons_settings;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_toggle_on_50_squared;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_toogle_off_50_squared;

extern GUI_CONST_STORAGE GUI_BITMAP bmicons_button_save_50_squared;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_button_select_40_sqaured;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_alarm_20;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_heating_20;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_cooling_20;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_cooling_20_activ;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_heating_20_activ;

extern GUI_CONST_STORAGE GUI_BITMAP bmicons_gate_garage_door_closed;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_gate_garage_door_closing;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_gate_garage_door_opening;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_gate_garage_door_open;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_gate_garage_door_partial_open;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_gate_pedestrian_closed;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_gate_pedestrian_open;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_gate_ramp_closed;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_gate_ramp_closing;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_gate_ramp_opening;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_gate_ramp_open;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_gate_ramp_partial_open;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_gate_swing_gate_closed;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_gate_swing_gate_closing;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_gate_swing_gate_opening;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_gate_swing_gate_open;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_gate_swing_gate_partial_open;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_gate_sliding_gate_closed;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_gate_sliding_gate_closing;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_gate_sliding_gate_partial_open;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_gate_sliding_gate_opening;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_gate_sliding_gate_open;

extern GUI_CONST_STORAGE GUI_BITMAP bmSijalicaOn;
extern GUI_CONST_STORAGE GUI_BITMAP bmSijalicaOff;
extern GUI_CONST_STORAGE GUI_BITMAP bmTermometar;
extern GUI_CONST_STORAGE GUI_BITMAP bmHome;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_button_up;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_button_up_50_squared;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_button_down;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_button_down_50_squared; 
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_button_right;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_button_right_50_squared;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_button_left;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_button_left_50_squared;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_button_fast_reverse;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_button_fast_reverse_50_squared;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_button_fast_forward;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_button_fast_forward_50_squared;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_toggle_on;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_toogle_off;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_date_time;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_button_ok;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_button_reset;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_button_cancel;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_button_cancel_50_squared;

extern GUI_CONST_STORAGE GUI_BITMAP bmCLEAN;
extern GUI_CONST_STORAGE GUI_BITMAP bmblindMedium;
extern GUI_CONST_STORAGE GUI_BITMAP bmbaflag;
extern GUI_CONST_STORAGE GUI_BITMAP bmengflag;
extern GUI_CONST_STORAGE GUI_BITMAP bmnext;
extern GUI_CONST_STORAGE GUI_BITMAP bmalHamliQR;
extern GUI_CONST_STORAGE GUI_BITMAP bmprevious;

extern GUI_CONST_STORAGE GUI_BITMAP bmwifi;
extern GUI_CONST_STORAGE GUI_BITMAP bmmobilePhone;
extern GUI_CONST_STORAGE GUI_BITMAP bmcolorSpectrum;
extern GUI_CONST_STORAGE GUI_BITMAP bmblackWhiteGradient;

extern GUI_CONST_STORAGE GUI_BITMAP bmicons_security_sos;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_language_bos;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_language_eng;

extern GUI_CONST_STORAGE GUI_BITMAP bmicons_scene_homecoming;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_scene_dinner;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_scene_gathering;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_scene_leaving;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_scene_morning;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_scene_movie;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_scene_reading;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_scene_relaxing;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_scene_security;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_scene_sleep;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_scene_wizzard;

extern const unsigned long thstat_size;
extern const unsigned char thstat[];

#endif // RESOURCE_H
/************************ (C) COPYRIGHT JUBERA D.O.O Sarajevo ************************/
