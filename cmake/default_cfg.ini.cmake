[main]
version                             = @PACKAGE_VERSION@
invert_screenshots_colors           = false

[plugins_load_at_startup]
Oculars                             = true
Satellites                          = true
SolarSystemEditor                   = true
Exoplanets                          = true
MeteorShowers                       = true
Novae                               = true
FOV                                 = true

[video]
fullscreen                          = true
screen_number                       = 0
screen_w                            = 1024
screen_h                            = 768
screen_x                            = 0
screen_y                            = 0
minimum_fps                         = 18
maximum_fps                         = 10000
#viewport_effect                     = sphericMirrorDistorter
viewport_effect                     = none
#vsync                               = true

[projection]
type                                = ProjectionStereographic
viewportMask                        = none
flip_horz                           = false
flip_vert                           = false

[spheric_mirror]
projector_gamma                     = 0.3
projector_position_x                = 0
projector_position_y                = 4
projector_position_z                = 0
mirror_position_x                   = 0
mirror_position_y                   = 5
mirror_position_z                   = 0
mirror_radius                       = 0.37
dome_radius                         = 5.0
image_distance_div_height           = 2.67
projector_delta                     = 10.6
projector_alpha                     = 0
projector_phi                       = 0
flip_horz                           = false
flip_vert                           = false
distorter_max_fov                   = 180
texture_triangle_base_length        = 8
flag_use_ext_framebuffer_object     = false

[localization]
sky_culture                         = western
sky_locale                          = system
app_locale                          = system
time_display_format                 = system_default
date_display_format                 = yyyymmdd

[search]
flag_search_online                  = true
simbad_server_url                   = http://simbad.u-strasbg.fr/

[stars]
relative_scale                      = 1.0
absolute_scale                      = 1.0
star_twinkle_amount                 = 0.2
flag_star_twinkle                   = true

#Johannes:
#I recommend setting mag_converter_max_fov to 180, so that the sky gets not so
#crowded when zoomed out
mag_converter_max_fov               = 90

#Johannes:
# mag_converter_min_fov of 0.1 is good for showing even stars of magnitude=17.95.
# If you do not use so faint stars, just reduce it
mag_converter_min_fov               = 0.001

#and finally the custom star colors.
#you can define your own star colors from bv_color_-0.50 up to bv_color_+3.50
#bv_color_-0.10 = 0,0,1
#bv_color_+0.50 = 1,0,0
#bv_color_+3.00 = 0,1,0

labels_amount                       = 3.0
init_bortle_scale                   = 2

[custom_selected_info]
flag_show_absolutemagnitude         = false
flag_show_altaz                     = false
flag_show_catalognumber             = false
flag_show_distance                  = false
flag_show_extra1                    = false
flag_show_extra2                    = false
flag_show_extra3                    = false
flag_show_hourangle                 = false
flag_show_magnitude                 = false
flag_show_name                      = false
flag_show_radecj2000                = false
flag_show_radecofdate               = false
flag_show_size                      = false

[gui]
flag_show_flip_buttons              = false
flag_show_nebulae_background_button = false
screen_font_size                    = 13
gui_font_size                       = 13
#Name of custom font file for some languages, like Thai.
#This file should be stored into data/ folder.
#base_font_file                      = DejaVuSans.ttf
#base_font_name                      = DejaVu Sans
flag_font_selection                 = false
mouse_cursor_timeout                = 10
flag_mouse_cursor_timeout           = false
selected_object_info                = all
auto_hide_horizontal_toolbar        = true
auto_hide_vertical_toolbar          = true
flag_enable_kinetic_scrolling       = true
# These values are used on non-Windows systems supporting GPSD
gpsd_hostname                       = localhost
gpsd_port                           = 2947
# These values are used on Windows only.
gps_interface                       = COM3
gps_baudrate                        = 4800

[color]
default_color                       = 0.5,0.5,0.7
daylight_text_color                 = 0.0,0.0,0.0
# Ecliptic J2000 group: red tones
ecliptical_J2000_color              = 0.4,0.1,0.1
ecliptic_J2000_color                = 0.7,0.2,0.2
equinox_J2000_points_color          = 0.7,0.2,0.2
solstice_J2000_points_color         = 0.7,0.2,0.2
ecliptic_J2000_poles_color          = 0.7,0.2,0.2
# Ecliptic of date group: orange tones
ecliptical_color                    = 0.6,0.3,0.1
ecliptic_color                      = 0.9,0.6,0.2
equinox_points_color                = 0.9,0.6,0.2
solstice_points_color               = 0.9,0.6,0.2
ecliptic_poles_color                = 0.9,0.6,0.2
precession_circles_color            = 0.9,0.6,0.2
# Equatorial J2000 group: blue tones (standard atlas coordinates)
equator_J2000_color                 = 0.2,0.2,0.6
equatorial_J2000_color              = 0.1,0.1,0.5
celestial_J2000_poles_color         = 0.2,0.2,0.6
# Equatorial of date group: brighter blue
equator_color                       = 0.3,0.5,1.0
equatorial_color                    = 0.2,0.3,0.8
celestial_poles_color               = 0.3,0.5,1.0
circumpolar_circles_color           = 0.3,0.5,1.0
# Galaxy: brownish, not too strong.
galactic_color                      = 0.3,0.2,0.1
galactic_equator_color              = 0.5,0.3,0.1
galactic_poles_color                = 0.5,0.3,0.1
# Supergalactic group: dark grey, should be less apparent than galactic.
supergalactic_color                 = 0.2,0.2,0.2
supergalactic_equator_color         = 0.4,0.4,0.4
supergalactic_poles_color           = 0.4,0.4,0.4
# Horizon and altazimuthal grid: greenish.
azimuthal_color                     = 0.0,0.3,0.2
horizon_color                       = 0.2,0.6,0.2
meridian_color                      = 0.2,0.6,0.2
prime_vertical_color                = 0.2,0.5,0.2
zenith_nadir_color                  = 0.2,0.6,0.2
cardinal_color                      = 0.8,0.2,0.1
# A mix of equatorial (blueish) and ecliptical (reddish)...
colures_color                       = 0.5,0.0,0.5
oc_longitude_color                  = 0.6,0.2,0.4
antisolar_point_color               = 0.9,0.3,0.5
apex_points_color                   = 0.8,0.2,0.3

# Constellations
const_lines_color                   = 0.2,0.2,0.6
const_names_color                   = 0.4,0.6,0.9
const_boundary_color                = 0.3,0.1,0.1
#DSO
dso_label_color                     = 0.2,0.6,0.7
dso_circle_color                    = 1.0,0.7,0.2
dso_galaxy_color                    = 1.0,0.2,0.2
dso_nebula_color                    = 0.1,1.0,0.1
dso_dark_nebula_color               = 0.3,0.3,0.3
dso_cluster_color                   = 1.0,1.0,0.1

planet_names_color                  = 0.5,0.5,0.7
planet_orbits_color                 = 0.7,0.2,0.2
planet_pointers_color               = 1.0,0.3,0.3
object_trails_color                 = 1.0,0.7,0.0
asteroid_orbits_color               = 0.7,0.5,0.5
comet_orbits_color                  = 0.7,0.8,0.8
sso_orbits_color                    = 0.7,0.2,0.2
script_console_keyword_color        = 1.0,0.0,1.0
script_console_module_color         = 0.0,1.0,1.0
script_console_comment_color        = 1.0,1.0,0.0
script_console_function_color       = 0.0,1.0,0.0
script_console_constant_color       = 1.0,0.5,0.5

[tui]
flag_show_gravity_ui                = false
flag_show_tui_datetime              = false
flag_show_tui_short_obj_info        = false
tui_font_size                       = 15
admin_shutdown_cmd                  = ""

[navigation]
auto_zoom_out_resets_direction      = false
preset_sky_time                     = 2451514.250011573
startup_time_mode                   = Actual
today_time                          = 22:00
flag_manual_zoom                    = false
flag_enable_zoom_keys               = true
flag_enable_move_keys               = true
flag_enable_mouse_navigation        = true
init_fov                            = 60
init_view_pos                       = 1,1e-05,0.2
auto_move_duration                  = 1.4
mouse_zoom                          = 10
move_speed                          = 0.0004
zoom_speed                          = 0.00035
viewing_mode                        = horizon

[landscape]
flag_landscape                      = true
flag_fog                            = true
flag_atmosphere                     = true
flag_landscape_sets_location        = false
atmosphere_fade_duration            = 0.5
# This is for people who require some minimum visibility for the landscapes
minimal_brightness                  = 0.10
flag_minimal_brightness             = false
# This allows use of a minimum value even given in the respective landscape.ini
flag_landscape_sets_minimal_brightness = true
flag_enable_illumination_layer      = true
flag_enable_labels                  = true
label_font_size                     = 18
label_color                         = 0.2,0.8,0.2

[viewing]
flag_constellation_drawing          = false
flag_constellation_name             = false
flag_constellation_art              = false
flag_constellation_boundaries       = false
flag_constellation_isolate_selected = false
flag_azimuthal_grid                 = false
flag_equatorial_grid                = false
flag_equatorial_J2000_grid          = false
flag_ecliptic_grid                  = false
flag_ecliptic_J2000_grid            = false
flag_galactic_grid                  = false
flag_galactic_equator_line          = false
flag_equator_line                   = false
flag_equator_J2000_line             = false
flag_ecliptic_line                  = false
flag_ecliptic__J2000_line           = false
flag_meridian_line                  = false
flag_longitude_line                 = false
flag_horizon_line                   = false
flag_cardinal_points                = true
flag_gravity_labels                 = false
flag_moon_scaled                    = false
moon_scale                          = 4
flag_minorbodies_scaled             = false
minorbodies_scale                   = 10
constellation_art_intensity         = 0.45
constellation_art_fade_duration     = 1.5
# GZ I found this unused, 2015-04.
#flag_chart                          = false
flag_night                          = false
light_pollution_luminance           = 0.0
use_luminance_adaptation            = true
sky_brightness_label_threshold      = 250.0

[astro]
flag_stars                          = true
flag_star_name                      = true
flag_planets                        = true
flag_planets_hints                  = false
flag_planets_orbits                 = false
flag_light_travel_time              = true
flag_object_trails                  = false
flag_nebula                         = true
flag_nebula_name                    = false
flag_nebula_display_no_texture      = false
flag_nutation                       = true
extinction_mode_below_horizon       = mirror
max_mag_nebula_name                 = 8
nebula_scale                        = 1
flag_nebula_hints_proportional      = false
flag_milky_way                      = true
milky_way_intensity                 = 1
milky_way_saturation                = 1
flag_bright_nebulae                 = false
meteor_zhr                          = 10
labels_amount                       = 3.0
nebula_hints_amount                 = 3.0
flag_star_magnitude_limit           = false
star_magnitude_limit                = 6.5
flag_planet_magnitude_limit         = false
planet_magnitude_limit              = 6.5
flag_nebula_magnitude_limit         = false
nebula_magnitude_limit              = 8.5
flag_use_de430                      = false
flag_use_de431                      = false
de430_path                          = ""
de431_path                          = ""

[init_location]
location                            = auto
landscape_name                      = guereins

[files]
#removable_media_path                = /mount/point

[scripts]
flag_script_allow_write_absolute_path = false
#flag_script_allow_ui                  = false

#[proxy]
#host_name                           = proxy.org
#port                                = 8080
#user                                = michael_knight
#passwo                              = xxxxx
