#!/bin/bash
# This script uninstalls Moonraker HMI service
set -eu

SYSTEMDDIR="/etc/systemd/system"

remove_all(){
  echo -e "Stopping services"

  services_list=($(sudo systemctl list-units -t service --full | grep moonraker-hmi | awk '{print $1}'))
  echo -e "${services_list[@]}"
  for service in "${services_list[@]}"
  do
    echo -e "${service}"
    echo -e "Removing $service ..."
    sudo systemctl stop $service
    sudo systemctl disable $service
    sudo rm -f $SYSTEMDDIR/$service
    echo -e "Done!"
  done

  rm -rf "${HOME}/klipper_logs/hmi*"


  sudo systemctl daemon-reload
  sudo systemctl reset-failed

}

remove_all
