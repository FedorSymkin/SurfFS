#!/bin/bash
site_host=$(echo "$1" | sed 's/^.*\/\///g')
site_ip=$(gethostip -d "$site_host")
if [ $? -ne 0 ]; then
        exit 1
fi
mount -t surffs "$1" -o ip=$site_ip "$2"
