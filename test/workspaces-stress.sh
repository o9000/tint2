#!/bin/bash

# List normal windows
# wmctrl -l | awk '{if ($4 != "Desktop") print $1}'
#
# Change the number of desktops
# xprop -root -f _NET_NUMBER_OF_DESKTOPS 32c -set _NET_NUMBER_OF_DESKTOPS 2
#
# Move window to desktop
# xprop -id 0x03600007 -f _NET_WM_DESKTOP 32c -set _NET_WM_DESKTOP 0
#
# Move window to all desktops
# xprop -id 0x03600007 -f _NET_WM_DESKTOP 32c -set _NET_WM_DESKTOP 4294967295

while true
do
  # change the number of desktops to a random value
  num_desktops=$(( $RANDOM % 8 + 1 ))
  xprop -root -f _NET_NUMBER_OF_DESKTOPS 32c -set _NET_NUMBER_OF_DESKTOPS $num_desktops
  max_desktop=$(( $num_desktops - 1 ))
  desktops=$(echo 4294967295; seq 0 $max_desktop)
  for run in 1 2 3
  do
    # start and stop calculators
    if (( $RANDOM % 5 == 0 ))
    then
      killall gnome-calculator 1>/dev/null 2>/dev/null
      sleep 0.1
    else
      (gnome-calculator 1>/dev/null 2>/dev/null &)
    fi
    # change the current desktop to a random value
    for change in 1 2 3
    do
      desktop=$(shuf -n 1 -e $(seq 0 $max_desktop))
      xprop -root -f _NET_CURRENT_DESKTOP 32c -set _NET_CURRENT_DESKTOP $desktop
      sleep 0.1
    done
    # move windows around
    for win in $(wmctrl -l | awk '!/Terminal/ {if ($4 != "Desktop") print $1}')
    do
      desktop=$(shuf -n 1 -e $desktops)
      xprop -id $win -f _NET_WM_DESKTOP 32c -set _NET_WM_DESKTOP $desktop
    done
    sleep 0.1
  done
  sleep 0.1
done
