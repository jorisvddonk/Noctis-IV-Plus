#!/bin/bash

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

CONF_FILE="$SCRIPT_DIR/dosbox.conf"
MOUNT_DIR="$SCRIPT_DIR"

echo
echo "+------------------------------+"
echo "| Noctis IV / IV Plus Launcher |"
echo "+------------------------------+"
echo

function pause() {
    local MESSAGE="${1:-"Press any key to continue . . ."}"
    read -n 1 -s -r -p "$MESSAGE"
    echo
}

function prompt_exit() {
    pause "Press any key to exit . . ."
    exit
}

if command -v dosbox-x > /dev/null 2>&1; then
    echo "DOSBox-X installation found!"
else
    echo "DOSBox-X is not installed!"
    echo
    echo "ERROR: No DOSBox-X installation found! Unable to launch!"
    echo "Install it with: sudo snap install dosbox-x"
    echo
    prompt_exit
fi

echo

if [[ -f "$CONF_FILE" ]]; then
    echo "Using conf file: \"$CONF_FILE\""
else
    echo "Could not find conf file: \"$CONF_FILE\""
    echo
    echo "ERROR: Configuration file not found! Unable to launch!"
    echo "Download here: https://github.com/jorisvddonk/Noctis-IV-Plus/raw/refs/heads/master/dosbox.conf"
    echo
    prompt_exit
fi

if ! [[ -f "$MOUNT_DIR/modules/NOCTIS.EXE" ]]; then
    echo
    echo "ERROR: Noctis IV / IV Plus not found! Unable to launch!"
    echo "Download Noctis IV here: https://80.style/packs/zip/hsp/noctis_iv-noctis_iv_download_JmsLdos_onlyK"
    echo "Download Noctis IV Plus from here: https://github.com/jorisvddonk/Noctis-IV-Plus/releases/latest"
    echo
    prompt_exit
fi



echo
echo "Launching Noctis IV..."
dosbox-x -c "mount n \"$MOUNT_DIR\"" -c "n:" -c "cd modules" -c "NOCTIS.EXE" -conf "$CONF_FILE" -exit &
