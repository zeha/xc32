#!/bin/bash

set -xe

USERNAME="langtools@microchip.com"
PASSWD="ezss-kztp-thln-koxn"

ZIP="signed_xc32_binaries.zip"
DMG="${1}.dmg"
APP="${1}.app"

notarize()
{
    output=$(xcrun altool --notarize-app \
             --primary-bundle-id "${APP}" \
             --username "${USERNAME}" \
             --password "${PASSWD}" \
             --file "$1" 2>&1)
    echo "${output}"

    request_id=$(echo "$output" | grep "RequestUUID")
    if [ "$?" != "0" ]; then
        # Something failed
        echo "Something failed when trying to upload '$1'. The output is '$output'"
        exit 1
    fi

    request_id=$(echo "$request_id" | sed 's/^.*RequestUUID = //')

    output=$(xcrun altool --notarization-info "${request_id}"\
            --username "${USERNAME}" \
            --password "${PASSWD}" 2>&1)
    status=$(echo "$output" | grep "Status")
    if [ "$?" != "0" ]; then
        # Something failed
        echo "Something failed when trying to get notarization info for requestid: '${request_id}'. The output is '${output}'"
        exit 1
    fi

    echo "${status}"

    # Check for in progress or complete.
    while [ "$(echo "$status" | grep -ci 'in progress')" == "1" ]
    do
        # Still in progress
        echo -n "."
        sleep 60
        output=$(xcrun altool --notarization-info "${request_id}" \
                              --username "${USERNAME}" \
                              --password "${PASSWD}" 2>&1)
        status=$(echo "$output" | grep "Status")
        if [ "$?" != "0" ]; then
            # Something failed
            echo "Something failed when trying to get notarization info for requestid: '${request_id}'. The output is '${output}'"
            exit 1
        fi
    done

    if [ "$(echo "$status" | grep -ci 'invalid')" -gt 0 ]; then
        echo "Notarization Failed as invalid: '$output'"
        exit 1
    fi

    # Notarization passed.
    # Do stapler step for dmg only
    if [[ $1 == *.dmg ]]; then
        xcrun stapler staple "$1"
    fi
}

notarize ${ZIP}
notarize ${DMG}
