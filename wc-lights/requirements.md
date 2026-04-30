WC Lights device that switches lights in WC and Bath rooms.

When user open the door the endstop switch shows that door is open and lights is ON. after that user close the door and we assume user is inside (depending on configurable night time (00:00-06:00 default value) it should be dimmed to configurable night brightness value (default: 30%)). When user open the door and close it again - Lights off. If user open the door and leave it open for a configurable value (defaults: WC-10s, Bath-20s) in seconds next door cloising will off the lights.

Additional RGB light strip (5V) should indicate that user is inside the WC (configuarble color, default: RED). And do a flashlight configurable (30s as default) time after visiting WC or Bath with configurable color (default yellow)

Configurable values should be for both rooms (WC and Bath) independently.
All configurations should be visible in HomeAssistant
Color should be presseted

Device should follow best HomeAssistant practice
Button for reset wifi
WiFI rssi sensor
WC Door Open/Closed sensor
Bath Door Open/Closed sensor
WC time user spent inside
Bath time user spent inside
WC Max time user inside
Bath Max time use inside
WC User Inside senosr
Bath User Inside senosr
WC On/Off and Brightness
Bath On/Off and Brightness
WC endstop NO NC switch settings
Bath endstop NO NC switch settings

Devide all controls by category WC or Bath for HomeAssistant accordintly
Reset button to reset all counters to 0 (duplicated with GPIO)
When captive portal is on or something wrong with WiFi GPOI logic should work independently of wifi

## Common Constraints
* Aviod magic numbers and hardcoded strings
* C/C++ code in additional .h file

## YAML Constraints
* Use substitution for easy configurability